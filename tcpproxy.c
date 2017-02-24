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

int main(int argc, char *argv[]) {
	char* remote_host;
	int remote_port;
	int proxy_server_port;
	ProxyParams proxy_params;
	memset(&proxy_params, 0, sizeof(ProxyParams));
	if (CheckInput(&proxy_params, argc, argv) != 0) {
		printf("\nUsage: tcpproxy remote_host remote_port proxy_server_port\n");
		return 0;
	}
	int s = ServerLoop();
	if (s < 0) {
		fprintf(stderr, "OH NO!");
		return 1;
	}
	struct sockaddr_in sin;
	// server loop
	while (1) {
		int sinlen = sizeof(sin);
		int cs = accept(s, (struct sockaddr *)&sin, &sinlen);
		if (cs < 0) {
			fprintf(stderr, "OH NO AGAN");
			return 1;
		}
		printf("\nConnection accepted from %s\n", inet_ntoa(sin.sin_addr));
	}
	return 0;
}

int ServerLoop() {
	int port = 12321;
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
	return s;
}
