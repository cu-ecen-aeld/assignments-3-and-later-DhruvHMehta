CC=gcc
CROSS_COMPILE=
CFLAGS= -g -Wall -Werror

all: aesdsocket

default: aesdsocket

aesdsocket : aesdsocket.o
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o aesdsocket aesdsocket.o

aesdsocket.o : aesdsocket.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c aesdsocket.c

clean :
	rm -f aesdsocket *.o
