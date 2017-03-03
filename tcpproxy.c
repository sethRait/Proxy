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

#define BUFFER_SIZE 256

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
	int connection_status;
	TransferData(fds, to_server, to_client);
}

void TransferData(struct pollfd *fds, char *to_server, char *to_client) {
	int server_offset = 0;
	int client_offset = 0;
	while(1) {
		poll(fds, 2, -1);
		// If there is data to read on the client socket and room in the buffer
		if (fds[0].revents & POLLIN && server_offset < BUFFER_SIZE) {
			server_offset += read(fds[0].fd, to_server + server_offset, 
						 	  	  BUFFER_SIZE - server_offset - 1);
		}
		// If the client can accept data and there is data to be sent
		while (fds[0].revents & POLLOUT && client_offset > 0) {
			client_offset -= write(fds[0].fd, to_client, client_offset);
		}
		// If there is data to read on the server socket and room in the buffer
		if (fds[1].revents & POLLIN && client_offset < BUFFER_SIZE) {
			client_offset += read(fds[1].fd, to_client + client_offset,
						  	  	  BUFFER_SIZE - client_offset - 1);
		}
		// If the server can accept data and there is data to be sent
		while (fds[1].revents & POLLOUT && server_offset > 0) {
			server_offset -= write(fds[1].fd, to_server, server_offset);
		}
		if (server_offset < 0 || client_offset < 0) {
			printf("Client disconnected\n");
			return;
		}
	}
}

int ConnectClient(int s) {
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	int sinlen = sizeof(sin);
	int client_sock = accept(s, (struct sockaddr *)&sin, &sinlen);
	if (client_sock < 0) {
		return -1;
	}
	return client_sock;
}

int ConnectRemote(char *host, int port, struct sockaddr_in *sa) {
	struct hostent *h;
	int s;

	h = gethostbyname(host);
	if (!h || h->h_length != sizeof(struct in_addr)) {
		fprintf(stderr, "%s: no such host\n", host);
		return -1;
	}

	s = socket(AF_INET, SOCK_STREAM, 0);
	memset(sa, 0, sizeof(sa));
	sa->sin_family = AF_INET;
	sa->sin_port = htons(0);	// OS can choose port
	sa->sin_addr.s_addr = htonl(INADDR_ANY);	// OS can choose IP
	if (bind(s, (struct sockaddr *)sa, sizeof(*sa)) < 0) {
		perror("bind");
		close(s);
		return -1;
	}

	// Set the destination address
	sa->sin_port = htons(port);
	sa->sin_addr = *(struct in_addr *)h->h_addr;

	// Connect to the server
	if (connect(s, (struct sockaddr *)sa, sizeof(*sa)) < 0) {
		perror(host);
		close(s);
		return -1;
	}
	return s;
}

int SetupListen(int port) {
	int s, n;
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		perror("socket");
		return -1;
	}
	// Use even when port is already in use
	n = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&n, sizeof(n)) < 0) {
		perror("SO_REUSEADDR");
		close(s);
		return -1;
	}
	// Set the flags
	fcntl(s, F_SETFD, 1);
	if (bind(s, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		fprintf(stderr, "TCP port %d: %s\n", port, strerror(errno));
		close(s);
		return -1;
	}
	if (listen(s, 5) < 0) {
		perror("listen");
		close(s);
		return -1;
	}
	return ConnectClient(s);
}
