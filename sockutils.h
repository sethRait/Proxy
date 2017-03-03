#ifndef __sockutils_h__
#define __sockutils_h__

#define SENDBUF 1024

/**
 * Create and set up a TCP server to listen on all IP addresses
 * associated with the machine. Listens at the specified port, and
 * listens with the specified allowed backlog.
 *
 * @param port where the server will listen for connections
 * @param backlog max connections waiting for TCP handshake
 * @return valid file descriptor for server, or -1 on error
 */
int make_server(char *port, int backlog);

/**
 * Wait for an incoming client connection.
 *
 * @param server valid file descriptor returned from make_server
 * @return valid file descriptor for client, or -1 on error
 */
int server_accept(int server);

/**
 * Translate a host and port into an addrinfo struct. You must free
 * the returned addrinfo structure when it is no longer needed (free
 * with freeaddrinfo).
 *
 * @param host hostname or ip address
 * @param port port number (in string representation)
 * @return allocated addrinfo ready to be used for making a connection
 *         or NULL on error
 */
struct addrinfo *make_addrinfo(const char *host, const char *port);

/**
 * Free the structure returned by make_addrinfo.
 *
 * @param info
 * @return void
 */
void free_addrinfo(struct addrinfo *info);

/**
 * Make an outgoing connection.
 *
 * @param info address to connect to
 * @return socket of established connection of -1 on error
 */
int host_connect(struct addrinfo *info);

#endif /* __sockutils_h__ */
