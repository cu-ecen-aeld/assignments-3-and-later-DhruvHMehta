CROSS_COMPILE=gcc

writer : writer.o
	$(CROSS_COMPILE) -o writer writer.o

writer.o : writer.c
	$(CROSS_COMPILE) -c writer.c

clean :
	rm writer writer.o
