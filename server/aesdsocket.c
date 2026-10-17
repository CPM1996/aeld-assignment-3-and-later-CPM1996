#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <syslog.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include "aesdsocket_linkedList.h"

#define SOCKETDATA_PATH "/var/tmp/aesdsocketdata"
#define AESDSOCKET_PORT_STR "9000"

typedef struct
{
    LinkedListElement linkedListElem;
    pthread_t thread;
    int connSocketFd;
    struct in_addr clientAddr;
} CONN_INFO_T;

int globalServerRunning = 1;

int sharedFd = -1;
pthread_mutex_t fdMutex;
LinkedList threadsList;


void signalHandler(int)
{
    syslog(LOG_INFO, "Caught signal, exiting");
    globalServerRunning = 0;
}

int joinCallback(LinkedListElement *elem, void *param)
{
    (void) param;

    return pthread_join(((CONN_INFO_T *)elem)->thread, NULL);
}

int attendConn(CONN_INFO_T *connInfo);
int processPacket(int connSocketFd, char *buf, int len);
int createConnTask(int connSocketFd, struct in_addr clientAddr);
void *timestampTask(void * param);

int main(const int argc, const char *argv[])
{
    struct sigaction sigAction = {0};
    int isDaemon = 0;

    char sLogBuff[128];
    int ret;
    
    
    int socketfd;
    struct addrinfo hints = {0};
    struct addrinfo *pAddrInfo = NULL;
    // struct sockaddr_in bindSAddrIn = {0};
    
    int connSocketFd;
    struct sockaddr_in connSockAddr;
    socklen_t conSockAddrLen = sizeof(connSockAddr);

    CONN_INFO_T *auxConnInfo;
    pthread_t timestampThread;

    if(argc > 2)
    {
        printf("Error: only one argument is accepted\n");
        return -1;
    }

    if((argc == 2) && (!strcmp(argv[1], "-d")))
    {
        isDaemon = 1;
    }

    openlog("aesdsocket", 0, LOG_USER);

    sigAction.sa_handler = signalHandler;
    // sigAction.sa_flags = SA_RESTART;

    sigaction(SIGINT, &sigAction, NULL);
    sigaction(SIGTERM, &sigAction, NULL);

    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd < 0)
    {
        snprintf(sLogBuff, sizeof(sLogBuff), "Error while creating socket: %s", strerror(errno));
        syslog(LOG_ERR, sLogBuff);
        return -1;
    }

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo(NULL, AESDSOCKET_PORT_STR, &hints, &pAddrInfo);
    if(ret != 0)
    {
        snprintf(sLogBuff, sizeof(sLogBuff), "Error on getaddrinfo(): %s", gai_strerror(ret));
        syslog(LOG_ERR, sLogBuff);
        close(socketfd);
        return -1;
    }

    ret = bind(socketfd, (struct sockaddr *) pAddrInfo->ai_addr, pAddrInfo->ai_addrlen);
    freeaddrinfo(pAddrInfo);
    if(ret != 0)
    {
        snprintf(sLogBuff, sizeof(sLogBuff), "Error while binding socket: %s", strerror(errno));
        syslog(LOG_ERR, sLogBuff);

        close(socketfd);
        return -1;
    }

    ret = listen(socketfd, 16);
    if(ret != 0)
    {
        snprintf(sLogBuff, sizeof(sLogBuff), "Error on listen(): %s", strerror(errno));
        syslog(LOG_ERR, sLogBuff);

        close(socketfd);
        return -1;
    }

    if(isDaemon)
    {
        ret = fork();
        if(ret <0)
        {
            snprintf(sLogBuff, sizeof(sLogBuff), "Error on listen(): %s", strerror(errno));
            syslog(LOG_ERR, sLogBuff);

            close(socketfd);
            return -1;
        }
        else if(ret > 0) // Parent process
        {
            return 0;
        }
    }

    sharedFd = open(SOCKETDATA_PATH, O_RDWR | O_CREAT | O_APPEND, S_IRWXU);
    if(sharedFd < 0)
    {
        snprintf(sLogBuff, sizeof(sLogBuff), "Error on open(): %s", strerror(errno));
        syslog(LOG_ERR, sLogBuff);

        close(socketfd);
        return -1;
    }

    pthread_mutex_init(&fdMutex, NULL);
    initLinkedList(&threadsList);

    if(pthread_create(&timestampThread, NULL, timestampTask, NULL))
    {
        snprintf(sLogBuff, sizeof(sLogBuff), "Error on pthread_create(): %s", strerror(errno));
        syslog(LOG_ERR, sLogBuff);

        close(socketfd);
        return -1;
    }

    while(globalServerRunning)
    {
        conSockAddrLen = sizeof(connSockAddr);

        connSocketFd = accept(socketfd, (struct sockaddr *) &connSockAddr, &conSockAddrLen);
        if(connSocketFd < 0)
        {
            if(errno != EINTR)
            {
                snprintf(sLogBuff, sizeof(sLogBuff), "Error on accept(): %s", strerror(errno));
                syslog(LOG_ERR, sLogBuff);
            }
            break;
        }

        createConnTask(connSocketFd, connSockAddr.sin_addr);
        // attendConn(connSocketFd);
    }

    travelList((LinkedList *) &threadsList, joinCallback, NULL);

    pthread_join(timestampThread, NULL);

    pthread_mutex_destroy(&fdMutex);

    if(sharedFd != -1)
    {
        close(sharedFd);
    }

    remove(SOCKETDATA_PATH);
    close(socketfd);

    return 0;
}

int createConnTask(int connSocketFd, struct in_addr clientAddr)
{
    CONN_INFO_T *connInfo;

    connInfo = malloc(sizeof(CONN_INFO_T));
    if(connInfo == NULL)
    {
        return -1;
    }

    connInfo->connSocketFd = connSocketFd;
    connInfo->clientAddr = clientAddr;
    insertElement(&threadsList, (LinkedListElement *) connInfo);

    return pthread_create(&connInfo->thread, NULL, (void *(*)(void *)) attendConn, connInfo);
}

int attendConn(CONN_INFO_T *connInfo)
{
    int recvSize;
    int recvCompleted = 0;
    char dataBuffer[256];
    int connSocketFd = connInfo->connSocketFd;

    char *fullDataBuffer = NULL;
    int fullDataBufferSize = 0, fullDataBufferLen = 0;
    int i;
    char *src, *dst;

    snprintf(dataBuffer, sizeof(dataBuffer), "Accepted connection from %s",
            inet_ntoa(connInfo->clientAddr));
    syslog(LOG_INFO, dataBuffer);

    while(!recvCompleted)
    {
        recvSize = recv(connSocketFd, dataBuffer, sizeof(dataBuffer), SA_RESTART);
        if((recvSize < 0) && (errno != EINTR))
        {
            recvCompleted = 1;
            snprintf(dataBuffer, sizeof(dataBuffer), "Error on recv(): %s", strerror(errno));
            syslog(LOG_ERR, dataBuffer);
        }
        else if(recvSize == 0)
        {
            recvCompleted = 1;
        }
        else
        {
            if(fullDataBufferLen + recvSize > fullDataBufferSize)
            {
                fullDataBufferSize += (recvSize > 1024) ? recvSize+1 : 1024;
                fullDataBuffer = realloc(fullDataBuffer, fullDataBufferSize);
                if(fullDataBuffer == NULL)
                {
                    snprintf(dataBuffer, sizeof(dataBuffer), "Error on realloc(): %s", strerror(errno));
                    syslog(LOG_ERR, dataBuffer);
                    // deleteElement((LinkedList *) threadsList, (LinkedListElement *) connInfo);
                    return -1;
                }
            }

            for(i = 0, src = dataBuffer, dst = &fullDataBuffer[fullDataBufferLen];
                i < recvSize;
                i++, fullDataBufferLen++, src++, dst++)
            {
                *dst = *src;
                if(*dst == '\n')
                {
                    processPacket(connSocketFd, fullDataBuffer, fullDataBufferLen+1);
                    
                    fullDataBufferLen = 0;
                    dst = fullDataBuffer;
                }
            }
        }
    }

    free(fullDataBuffer);
    close(connSocketFd);

    snprintf(dataBuffer, sizeof(dataBuffer), "Closed connection from %s",
        inet_ntoa(connInfo->clientAddr));
    syslog(LOG_INFO, dataBuffer);

    // deleteElement((LinkedList *) threadsList, (LinkedListElement *) connInfo);

    return 0;
}

int processPacket(int connSocketFd, char *buf, int len)
{
    char readBuff[256];
    int readChars;
    int ret;

    pthread_mutex_lock(&fdMutex);

    lseek(sharedFd, 0, SEEK_END);
    ret = write(sharedFd, buf, len);
    if((ret < len) && (errno != EINTR))
    {
        pthread_mutex_unlock(&fdMutex);
        return -1;
    }

    lseek(sharedFd, 0, SEEK_SET);
    
    errno = 0;
    do
    {
        readChars = read(sharedFd, readBuff, sizeof(readBuff));
        if((readChars > 0))
        {
            send(connSocketFd, readBuff, readChars, 0);
        }

        if(errno == EINTR)
        {
            readChars = 1;
            errno = 0;
        }
    }while(readChars > 0);

    pthread_mutex_unlock(&fdMutex);
}

void * timestampTask(void * param)
{
    (void) param;

    struct timespec currentTime;
    struct timespec waitTime;
    struct tm localTime;

    char buf[32];
    int len;
    int ret;

    waitTime.tv_sec = 10;
    waitTime.tv_nsec = 0;
    while(globalServerRunning)
    {
        clock_gettime(CLOCK_REALTIME, &currentTime);
        localtime_r(&currentTime.tv_sec, &localTime);

        strftime(buf, sizeof(buf), "timestamp:%a, %d %b %Y %T %z", &localTime);
        len = strlen(buf);
        strncat(buf, "\n", sizeof(buf)-1-len);

        pthread_mutex_lock(&fdMutex);
        lseek(sharedFd, 0, SEEK_END);
        ret = write(sharedFd, buf, len);
        if((ret < len) && (errno != EINTR))
        {
            pthread_mutex_unlock(&fdMutex);
            return NULL;
        }
        pthread_mutex_unlock(&fdMutex);

        nanosleep(&waitTime, NULL);
    }

    return NULL;
}