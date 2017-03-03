#include <poll.h>
#include <netinet/in.h>
#include <sys/socket.h>

typedef struct ProxyParams {
	char* remote_host;
	int remote_port;
	int proxy_server_port;
} ProxyParams;

// Helper method which sets up a socket on the given port to listen for incoming
//connections.
int SetupListen(int port);

// Connects a client to the proxy server and returns the client's socket fd
int ConnectClient(int s);

// Connects a remote server to the proxy server and returns the remote server's
// socket fd
int ConnectRemote(char *host, int port, struct sockaddr_in *sa);

// Handle state for the connection and run the proxy loop
void HandleConnection(int client_sock, int remote_sock);

// perform reads and writes between the remote client and remote server
void TransferData(struct pollfd *fds, char *to_server, char *to_client);

// Connect incoming connections
void ConnectionLoop(ProxyParams *proxy_params); 
