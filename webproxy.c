#include "http_parser.h"
#include "tools.h"
#include "webproxy.h"

#include <errno.h>
#include <pthread.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_LEN 16384 
#define BAD_REQUEST "HTTP/1.0 400 Bad Request\r\n\r\n"
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
	int client_sock;
	while (1) {
		client_sock = ConnectClient(proxy_sock);
		make_async(client_sock);
		proxy_info *p_info;
		p_info = make_proxy_info(client_sock);
		pthread_t thread;
		int rc = pthread_create(&thread, NULL, &proxy, p_info);
	}
}

static void *proxy(void *vproxy_info) {
	proxy_info *p_info = (proxy_info *)vproxy_info;
	parse_info info;
	memset(&info, 0, sizeof(parse_info));
	int remote_sock = ReadRequest(p_info->client_sock, &info);
	make_async(remote_sock);
	HandleConnection(p_info->client_sock, remote_sock);
	close(p_info->client_sock);
	close(remote_sock);
	free_proxy_info(p_info);
	return NULL;
}


int ReadRequest(int client_sock, parse_info *info) {
	char buffer[BUF_LEN] = {'\0'};
	http_parser *parser = malloc(sizeof(http_parser));
	http_parser_init(parser, HTTP_REQUEST);
	parser->data = (void *)info;
	int remote_sock = HandleRewrite(client_sock, buffer, parser, info);
	return remote_sock;
}

int HandleRewrite(int client_sock, char buffer[], http_parser *parser,
				  parse_info *info) {
	int nread, nparsed;
	struct pollfd fd[1];
	memset(fd, 0, sizeof(struct pollfd));
	fd[0].fd = client_sock;
	fd[0].events |= POLLIN;
	poll(fd, 1, -1);
	nread = read(client_sock, buffer, BUF_LEN - 1);
	if (nread < 0) {
		fprintf(stderr, "read error: %d\n", errno);
	}
	info->buffer = buffer;
	info->buf_length = nread;
	nparsed = http_parser_execute(parser, &settings, buffer, nread);
	if (nread != nparsed) {
		return -1;
	}
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	int remote_sock = ConnectRemote(info, &sin);
	if (remote_sock < 0) {	// Bad request
		write(client_sock, BAD_REQUEST, 28);
		return -1;
	}
	memset(buffer, '\0', BUF_LEN);
	strcpy(buffer, info->request);
	info->request_length = strlen(info->buffer);
	write(remote_sock, info->buffer, info->request_length);
	return remote_sock;
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
