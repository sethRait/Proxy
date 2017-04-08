CC=gcc
HTTP_PARSER = http-parser
CFLAGS = -g -Wall -I$(HTTP_PARSER)
LDFLAGS = -lpthread
VPATH = $(HTTP_PARSER)

$(HTTP_PARSER)/libhttp_parser.a:
	cd $(HTTP_PARSER); $(MAKE) package

webproxy: webproxy.o $(HTTP_PARSER)/libhttp_parser.a tools.o $(LDFLAGS)

clean:
	rm *.o
