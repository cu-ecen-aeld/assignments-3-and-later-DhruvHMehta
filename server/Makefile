CC=gcc
CROSS_COMPILE=
CFLAGS= -g -Wall -Werror
LDFLAGS= -pthread

all: aesdsocket

default: aesdsocket

aesdsocket : aesdsocket.o
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(LDFLAGS) -o aesdsocket aesdsocket.o

aesdsocket.o : aesdsocket.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(LDFLAGS) -c aesdsocket.c

clean :
	rm -f aesdsocket *.o
