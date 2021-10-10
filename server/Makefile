CC=gcc
CROSS_COMPILE=
CFLAGS= -g -Wall -Werror
LDFLAGS= -lrt -pthread

all: aesdsocket

default: aesdsocket

aesdsocket : aesdsocket.o
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o aesdsocket aesdsocket.o $(LDFLAGS)

aesdsocket.o : aesdsocket.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c aesdsocket.c $(LDFLAGS)

clean :
	rm -f aesdsocket *.o
