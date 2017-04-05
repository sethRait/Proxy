#include "http_parser.h"
#include "tools.h"
#include "webproxy.h"

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_LEN 16384 
int proxy_server_port;

int main(int argc, char *argv[]) {
	if ((CheckInput(argc, argv)) < 0) {
		printf("Usage: webproxy proxy_server_port\n");
		return 0;
	}
	ConnectionLoop();
	return 0;
}

void ConnectionLoop() {
	int proxy_sock = SetupListen(proxy_server_port);
	int remote_sock, client_sock;
	while (1) {
		make_async(client_sock);
		client_sock = ConnectClient(proxy_sock);
		remote_sock = ReadRequest(client_sock);
		make_async(remote_sock);
		HandleConnection(client_sock, remote_sock);
		close(client_sock);
		close(remote_sock);
	}
}

int ReadRequest(int client_sock) {
	char buffer[BUF_LEN] = {'\0'};
	parse_info parsed;
	http_parser *parser = malloc(sizeof(http_parser));
	memset(&parsed, 0, sizeof(parse_info));
	http_parser_init(parser, HTTP_REQUEST);
	parser->data = (void *)&parsed;
	DoTheStuff(client_sock, buffer, parser, &parsed);
	return 0;
}

void DoTheStuff(int client_sock, char buffer[], http_parser *parser, parse_info *parsed) {
	int nread, nparsed;
	nread = read(client_sock, buffer, BUF_LEN - 1);
	parsed->buffer = buffer;
	parsed->buf_length = nread;
	nparsed = http_parser_execute(parser, &settings, buffer, nread);
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	int remote_sock = ConnectRemote(parsed, &sin);
	if (remote_sock < 0) {
		fprintf(stderr, "Error connecting remote server\n");
		return;
	}
	int ret = write(remote_sock, parsed->request, nread);
	HandleConnection(client_sock, remote_sock);
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
