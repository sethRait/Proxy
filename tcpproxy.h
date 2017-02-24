typedef struct ProxyParams {
	char* remote_host;
	int remote_port;
	int proxy_server_port;
} ProxyParams;

int ServerLoop();
