#include "http_parser.h"

#include <netinet/in.h>
#include <poll.h>
#include <stdlib.h>

typedef struct {
	int client_sock;
} proxy_info;

static proxy_info *make_proxy_info(int client_sock) {
	proxy_info* info = malloc(sizeof(proxy_info));
	info->client_sock = client_sock;
	return info;
}
static void free_proxy_info(proxy_info *info) {
	free(info);
}

static void *proxy(void *vproxy_info);

// Handle state for the connection and run the proxy loop
void HandleConnection(int client_sock, int remote_sock);

// Read the client's HTTP request and connect to the remote server
int ReadRequest(int client_sock, parse_info *info);
	
// perform reads and writes between the remote client and remote server
void TransferData(struct pollfd *fds, char *to_server, char *to_client);

// Connect incoming connections
void ConnectionLoop(); 

// Manage the rewrite of the request buffer.  Returns the socket of the server
// requested by the client.
int HandleRewrite(int client_sock, char buffer[], http_parser *parser,
				  parse_info *info);

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
	parse_info *info = ((parse_info *)(parser->data));
	SetHost(info, s, length);
	SetPort(info, s, length);
	return RewriteRequest(info, s, length);
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
