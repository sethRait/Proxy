CC=gcc
HTTP_PARSER = http-parser
CFLAGS= -lpthread -g -Wall -I$(HTTP_PARSER)
VPATH = $(HTTP_PARSER)

$(HTTP_PARSER)/libhttp_parser.a:
	cd $(HTTP_PARSER); $(MAKE) package

webproxy: webproxy.o $(HTTP_PARSER)/libhttp_parser.a tools.o

clean:
	rm *.o
