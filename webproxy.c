#include "http_parser.h"
#include "tcpproxy.h"
#include "tools.h"

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_LEN 16384 

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
	int proxy_sock = SetupListen(proxy_params->proxy_server_port);
	int remote_sock, client_sock;
	while (1) {
		remote_sock = ConnectRemote(proxy_params->remote_host,
								   proxy_params->remote_port, &sin);
		make_async(remote_sock);
		make_async(client_sock);
		client_sock = ConnectClient(proxy_sock);
		// Connect to remote client
		HandleConnection(client_sock, remote_sock);
		close(client_sock);
		close(remote_sock);
	}
}

void HandleConnection(int client, int server) {
	char to_server[BUF_LEN] = {'\0'};
	char to_client[BUF_LEN] = {'\0'};
	struct pollfd fds[2];
	memset(fds, 0, 2 * sizeof(struct pollfd));
	fds[0].fd = client;
	fds[0].events |= (POLLIN | POLLOUT);
	fds[1].events |= (POLLIN | POLLOUT);
	fds[1].fd = server;
	TransferData(fds, to_server, to_client);
}

void TransferData(struct pollfd *fds, char *to_server, char *to_client) {
	int serv_count = 0;
	int client_count = 0;
	while(1) {
		poll(fds, 2, -1);
		// If there is data to read on the client socket and room in the buffer
		if (fds[0].revents & POLLIN && serv_count < BUF_LEN - 1) {
			serv_count += read(fds[0].fd, to_server + serv_count, 
						 	   BUF_LEN - serv_count - 1);
		}
		// If the client can accept data and there is data to be sent
		while (fds[0].revents & POLLOUT && client_count > 0) {
			client_count -= write(fds[0].fd, to_client, client_count);
		}
		// If there is data to read on the server socket and room in the buffer
		if (fds[1].revents & POLLIN && client_count < BUF_LEN - 1) {
			client_count += read(fds[1].fd, to_client + client_count,
						  	  	 BUF_LEN - client_count - 1);
		}
		// If the server can accept data and there is data to be sent
		while (fds[1].revents & POLLOUT && serv_count > 0) {
			serv_count -= write(fds[1].fd, to_server, serv_count);
		}
		if (serv_count < 0 || client_count < 0) {
			fprintf(stderr, "Client disconnected with status: %d\n", errno);
			return;
		}
		if (serv_count == 0 && client_count == 0 && fds[1].revents & ~POLLOUT) {
			return;
		}
	}
}
