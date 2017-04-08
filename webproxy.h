#include "http_parser.h"

#include <netinet/in.h>
#include <poll.h>
#include <stdlib.h>

typedef struct {
	int client_sock;
}proxy_info;

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
