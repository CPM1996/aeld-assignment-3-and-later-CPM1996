all: aesdsocket

default: aesdsocket

aesdsocket : aesdsocket.o aesdsocket_linkedList.o 
	${CROSS_COMPILE}${GCC} -o aesdsocket aesdsocket.o aesdsocket_linkedList.o 

aesdsocket.o : aesdsocket.c aesdsocket_linkedList.h
	${CROSS_COMPILE}${GCC} -c aesdsocket.c

aesdsocket_linkedList.o : aesdsocket_linkedList.c aesdsocket_linkedList.h
	${CROSS_COMPILE}${GCC} -c aesdsocket_linkedList.c

clean :
	rm -f aesdsocket aesdsocket.o
