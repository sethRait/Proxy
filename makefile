CC=gcc
CFLAGS=-I.
tcpproxy: tcpproxy.o tools.o
	$(CC) -o tcpproxy tcpproxy.o tools.o -I.
