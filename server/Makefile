all: aesdsocket

default: aesdsocket

CC ?= ${CROSS_COMPILE}gcc
CFLAGS ?= -g -Wall -Werror
TARGET ?= aesdsocket
LDFLAGS ?= -lpthread -lrt

aesdsocket : aesdsocket.o aesdsocket_linkedList.o 
	${CC} ${LDFLAGS} -o aesdsocket aesdsocket.o aesdsocket_linkedList.o 

aesdsocket.o : aesdsocket.c aesdsocket_linkedList.h
	${CC} ${CFLAGS} -c aesdsocket.c

aesdsocket_linkedList.o : aesdsocket_linkedList.c aesdsocket_linkedList.h
	${CC} ${CFLAGS} -c aesdsocket_linkedList.c

clean :
	rm -f aesdsocket aesdsocket.o
