#include "webproxy.h"
#include "tools.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int make_async(int s) {
    int n;
    if((n = fcntl(s, F_GETFL)) == -1 || fcntl(s, F_SETFL, n | O_NONBLOCK) == -1)
		return -1;
    n = 1;
    if(setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &n, sizeof(n)) == -1)
		return -1;
    return 0;
}

int CheckInput(int argc, char *argv[]) {
	if (argc != 2) {
		return -1;
	}
	if (!(atoi(argv[1]) <= 65535 && atoi(argv[1]) > 1024)) {
		fprintf(stderr, "Port must be between 1025 and 65535\n");
		return -1;
	}
	proxy_server_port = atoi(argv[1]);
	return proxy_server_port;
}

int ConnectClient(int s) {
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	socklen_t sinlen = sizeof(sin);
	int client_sock = accept(s, (struct sockaddr *)&sin, &sinlen);
	if (client_sock < 0) {
		return -1;
	}
	return client_sock;
}

int ConnectRemote(parse_info *info, struct sockaddr_in *sa) {
	struct hostent *h;
	int s;
	char *host = malloc(info->irl_offset);
	strncpy(host, info->host, info->irl_offset - info->protocol_offset);
	h = gethostbyname(host);
	if (!h || h->h_length != sizeof(struct in_addr)) {
		fprintf(stderr, "%s: no such host\n", host);
		return -1;
	}

	s = socket(AF_INET, SOCK_STREAM, 0);
	sa->sin_family = AF_INET;
	sa->sin_port = htons(0);	// OS can choose port
	sa->sin_addr.s_addr = htonl(INADDR_ANY);	// OS can choose IP
	if (bind(s, (struct sockaddr *)sa, sizeof(*sa)) < 0) {
		perror("bind");
		close(s);
		return -1;
	}

	// Set the destination address
	sa->sin_port = htons(80);	// For now, just use port 80 for HTTP
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
	return s;
}

void GetOffsets(parse_info *parse_struct, const char *s, size_t length) {
	if (strncmp(s, "https://", 8) == 0) {
		parse_struct->protocol_offset = 8;
	}
	else if(strncmp(s, "http://", 7) ==0) {
		parse_struct->protocol_offset = 7;
	}
	for (int i = parse_struct->protocol_offset; i < length; i++) {
		if (s[i] == '/') {	// Count the '/' occurrences.
			parse_struct->irl_offset = i;
			break;
		}
	}
	parse_struct->host = s + parse_struct->protocol_offset;
}
