#include "http_parser.h"
#include <netinet/in.h>
#include <poll.h>

// Handle state for the connection and run the proxy loop
void HandleConnection(int client_sock, int remote_sock);

// Read the client's HTTP request and connect to the remote server
int ReadRequest(int client_sock);
	
// perform reads and writes between the remote client and remote server
void TransferData(struct pollfd *fds, char *to_server, char *to_client);

// Connect incoming connections
void ConnectionLoop(); 

void DoTheStuff(int client_sock, char buffer[], http_parser *parser, parse_info *parsed);
