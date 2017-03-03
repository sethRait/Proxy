#include "tcpproxy.h"
#include "tools.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
	ProxyParams proxy_params;
	memset(&proxy_params, 0, sizeof(ProxyParams));
	if (CheckInput(&proxy_params, argc, argv) != 0) {
		printf("\nUsage: tcpproxy remote_host remote_port proxy_server_port\n");
		return 0;
	}
	ConnectionLoop(&proxy_params);
	return 0;
}

void ConnectionLoop(ProxyParams *proxy_params) {
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	// Connect to remote server
	int remote_sock = ConnectRemote(proxy_params->remote_host,
									proxy_params->remote_port, &sin);
	int client_sock;
	while (1) {
		// Connect to remote client
		client_sock = SetupListen(proxy_params->proxy_server_port);
		HandleConnection(client_sock, remote_sock);
		close(client_sock);
	}
}

void HandleConnection(int client, int server) {
	char to_server[BUFFER_SIZE] = {'\0'};
	char to_client[BUFFER_SIZE] = {'\0'};
	struct pollfd fds[2];
	memset(fds, 0, 2 * sizeof(struct pollfd));
	fds[0].fd = client;
	fds[0].events |= (POLLIN | POLLOUT);
	fds[1].events |= (POLLIN | POLLOUT);
	fds[1].fd = server;
	TransferData(fds, to_server, to_client);
}

void TransferData(struct pollfd *fds, char *to_server, char *to_client) {
	int server_offset = 0;
	int client_offset = 0;
	int msg_sent = 0;
	int msg_recieved = 0;
	while(1) {
		poll(fds, 2, -1);
		// If there is data to read on the client socket and room in the buffer
		if (fds[0].revents & POLLIN && server_offset < BUFFER_SIZE - 1) {
			server_offset += read(fds[0].fd, to_server + server_offset, 
						 	  	  BUFFER_SIZE - server_offset - 1);
		}
		// If the client can accept data and there is data to be sent
		while (fds[0].revents & POLLOUT && client_offset > 0) {
			client_offset -= write(fds[0].fd, to_client, client_offset);
			if (client_offset == 0) {
				memset(to_client, '\0', BUFFER_SIZE);
				msg_recieved = 1;
			}
		}
		// If there is data to read on the server socket and room in the buffer
		if (fds[1].revents & POLLIN && client_offset < BUFFER_SIZE - 1) {
			client_offset += read(fds[1].fd, to_client + client_offset,
						  	  	  BUFFER_SIZE - client_offset - 1);
		}
		// If the server can accept data and there is data to be sent
		while (fds[1].revents & POLLOUT && server_offset > 0) {
			server_offset -= write(fds[1].fd, to_server, server_offset);
			if (server_offset == 0) {
				memset(to_server, '\0', BUFFER_SIZE);
				msg_sent = 1;
			}
		}
		if (server_offset < 0 || client_offset < 0) {
			fprintf(stderr, "Client disconnected\n");
			return;
		}
		printf("to_server: %d\nto_client: %d\n", server_offset, client_offset);
		usleep(250000);
		if (server_offset == 0 && client_offset == 0 && msg_recieved == 1) {
			char end[] = "\r\n\r\n";
			write(fds[1].fd, end, 4);
			msg_recieved = 0;
		}
	}
}
