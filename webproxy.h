#include <netinet/in.h>
#include <poll.h>

// Handle state for the connection and run the proxy loop
void HandleConnection(int client_sock, int remote_sock);

// perform reads and writes between the remote client and remote server
void TransferData(struct pollfd *fds, char *to_server, char *to_client);

// Connect incoming connections
void ConnectionLoop(); 
