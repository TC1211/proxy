CC = gcc

CFLAGS = -Wall -g

proxy: proxy.o
	$(CC) -Wall -pthread -o proxy proxy.o 

clean: 
	$(RM) main *o *~
