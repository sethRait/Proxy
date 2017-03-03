#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "utils.h"
#include "sockutils.h"

int make_server(char *port, int backlog) {
    int sock = -1, n = 1;
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perr("socket");
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n)) < 0)
        perr("setsockopt SO_REUSEADDR");
    fcntl(sock, F_SETFD, 1); /* keep server file descriptor private */
    if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        perr("bind");
    if(listen(sock, backlog) < 0)
        perr("listen");

    return sock;
    
  err:
    if(sock >= 0)
        close(sock);
    return -1;
}

int server_accept(int server) {
    struct sockaddr addr;
    socklen_t len = sizeof(addr);
    int sock;

    if((sock = accept(server, &addr, &len)) < 0)
        perr("accept");
    return sock;
    
  err:
    return -1;
}

struct addrinfo *make_addrinfo(const char *host, const char *port) {
    struct addrinfo hints, *info;
    int ret;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;  /* TCP (stream-oriented)          */
    hints.ai_family   = AF_INET;      /* IPv4                           */
    hints.ai_flags    = AI_CANONNAME; /* Include canonical name in info */

    if((ret = getaddrinfo(host, port, &hints, &info)) != 0)
        err("Could not resolve address %s:%s (%s)\n",
            host, port, gai_strerror(ret));

    return info;
  err:
    return NULL;
}

void free_addrinfo(struct addrinfo *info) {
    freeaddrinfo(info);
}

int host_connect(struct addrinfo *info) {
    int fd, size = SENDBUF;

    if(info == NULL)
        err("Invalid address");

    if((fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol)) < 0)
        perr("socket");
    if(setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) < 0)
        perr("setsockopt");
    if(connect(fd, info->ai_addr, info->ai_addrlen) != 0)
        err("Could not connect to host %s:%d (%s)",
	    info->ai_canonname,
            ntohs(((struct sockaddr_in *)info->ai_addr)->sin_port),
            strerror(errno));

    return fd;
  err:
    return -1;
}
