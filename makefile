CC=gcc
HTTP_PARSER = /home/seth/Documents/Systems/project2/Proxy/http-parser
CFLAGS= -g -Wall -I$(HTTP_PARSER)
VPATH = $(HTTP_PARSER)

$(HTTP_PARSER)/libhttp_parser.a:
	cd $(HTTP_PARSER); $(MAKE) package

webproxy: webproxy.o $(HTTP_PARSER)/libhttp_parser.a tools.o
	$(CC) -o webproxy webproxy.o tools.o -I.

tcpproxy: tcpproxy.o tools.o
	$(CC) -o tcpproxy tcpproxy.o tools.o -I.
clean:
	rm *.o
