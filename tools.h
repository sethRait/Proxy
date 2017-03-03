// Gather and check commandline arguments to the program.  Returns 0 on success,
// returns 1 otherwise.
int CheckInput(ProxyParams* proxy_params, int argc, char *argv[]);

// Helper method which sets up a socket on the given port to listen for incoming
//connections.
int SetupListen(int port);

// Connects a client to the proxy server and returns the client's socket fd
int ConnectClient(int s);

// Connects a remote server to the proxy server and returns the remote server's
// socket fd
int ConnectRemote(char *host, int port, struct sockaddr_in *sa);
