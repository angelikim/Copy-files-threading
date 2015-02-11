.PHONY: clean all

CC = g++
CFLAGS = -Wall -g3 -pedantic 
LFLAGS= -pthread

all:client server 

client: client.o   library.o
	$(CC) $^ $(LFLAGS) -o $@  

client.o: client.cpp library.cpp
	$(CC) $(CFLAGS) $^ -c

server: server.o library.o
	$(CC) $^ $(LFLAGS) -o $@  

server.o: server.cpp
	$(CC) $(CFLAGS) $^ -c

library.o: library.cpp
	$(CC) $(CFLAGS) $^ -c

distclean:
	$(RM) client server library

clean:
	$(RM) *.o
