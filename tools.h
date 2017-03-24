#include <stdio.h>

// Makes a socket ready for asynchronous IO.  Returns -1 on error.
int make_async(int s);
// Gather and check commandline arguments to the program.  Returns 0 on success,
// returns 1 otherwise.
int CheckInput(ProxyParams* proxy_params, int argc, char *argv[]);

// Helper method which sets up a socket on the given port to listen for incoming
//connections.
int SetupListen(int port);

// Connects a client to the proxy server and returns the client's socket fd
int ConnectClient(int s);

// Connects a remote server to the proxy server and returns the remote server's
// socket fd
int ConnectRemote(char *host, int port, struct sockaddr_in *sa);

static int message_begin_cb(http_parser *parser) {
	printf("message begin\n");
	return 0;
}

static int message_complete_cb(http_parser *parser) {
	printf("message complete\n");
	return 0;
}

static int url_cb(http_parser *parser, const char *s, size_t length) {
	printf("url: %.*s\n", (int)length, s);
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
