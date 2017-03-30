#include <netinet/ip.h>
#include <stdio.h>
#include "http_parser.h"

extern int proxy_server_port;

typedef struct {
	const char * host;
	int fqdn_length;
	int irl_offset;
	int protocol_offset;
	char ** headers;
	int content_length_exists;
	int content_length;
}parse_info;

// Makes a socket ready for asynchronous IO.  Returns -1 on error.
int make_async(int s);

// Gather and check commandline arguments to the program.  Returns port number
// on success and -1 otherwise.
int CheckInput(int argc, char *argv[]);

// Helper method which sets up a socket on the given port to listen for incoming
//connections.
int SetupListen(int port);

// Connects a client to the proxy server and returns the client's socket fd
int ConnectClient(int s);

// Helper method for finding the offset in the domain string of the filepath
void GetOffsets(parse_info *parse_struct, const char *s, size_t length);

// Connects a remote server to the proxy server and returns the remote server's
// socket fd
int ConnectRemote(parse_info *info, struct sockaddr_in *sa);

static int message_begin_cb(http_parser *parser) {
	printf("message begin\n");
	return 0;
}

static int message_complete_cb(http_parser *parser) {
	printf("message complete\n");
	return 0;
}


// Set the url field of the parser struct
static int url_cb(http_parser *parser, const char *s, size_t length) {
	parse_info *parse_struct = ((parse_info *)(parser->data));
	parse_struct->fqdn_length = (int)length;
	GetOffsets(parse_struct, s, length);
	return 0;
}

static http_parser_settings settings = {
	.on_message_begin = message_begin_cb,
	.on_header_field = NULL,
	.on_header_value = NULL,
	.on_url = url_cb,
	.on_body = NULL,
	.on_headers_complete = NULL,
	.on_message_complete = message_complete_cb
};
