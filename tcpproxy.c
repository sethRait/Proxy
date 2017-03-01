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

#define BUFFER_SIZE 256

int main(int argc, char *argv[]) {
	ProxyParams proxy_params;
	memset(&proxy_params, 0, sizeof(ProxyParams));
	if (CheckInput(&proxy_params, argc, argv) != 0) {
		printf("\nUsage: tcpproxy remote_host remote_port proxy_server_port\n");
		return 0;
	}
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	// Connect to remote server
	int remote_sock = ConnectRemote(proxy_params.remote_host,
								    proxy_params.remote_port, &sin);
	// Wait for and connect to incoming connections
	int client_sock = SetupListen(proxy_params.proxy_server_port);
	HandleConnection(client_sock, remote_sock);
	return 0;
}

void HandleConnection(int client, int remote) {
	char buf[BUFFER_SIZE] = {'0'};
	int buf_pos = 0, num_read = 0;
	num_read = read(client, buf, BUFFER_SIZE);	// Read request from client
	if (num_read < 0) {
		fprintf(stderr, "\nError reading from socket\n");
		return;
	}
	printf("\nread: %s", buf);
	write(remote, buf, num_read);	// Write request from client to remote
	memset(buf, 0, BUFFER_SIZE);	
	num_read = read(remote, buf, BUFFER_SIZE - 1);  // Read response from remote
	if (num_read < 0) {
		fprintf(stderr, "\nError reading from remote socket\n");
		return;
	}
	printf("\nremote returned: %s", buf);
	return;
}

int ConnectClient(int s) {
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	int sinlen = sizeof(sin);
	int client_sock = accept(s, (struct sockaddr *)&sin, &sinlen);
	if (client_sock < 0) {
		return -1;
	}
	printf("\nClient connection accepted from %s\n", inet_ntoa(sin.sin_addr));
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
	printf("\nConnected to remote host %s", host);
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
