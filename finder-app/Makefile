CC=gcc
CROSS_COMPILE=

all: writer

writer : writer.o
	$(CROSS_COMPILE)$(CC) -o writer writer.o

writer.o : writer.c
	$(CROSS_COMPILE)$(CC) -c writer.c

clean :
	rm -f writer *.o
