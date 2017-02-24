#include "tcpproxy.h"

#include <stdlib.h>
#include <stdio.h>
int CheckInput(ProxyParams* proxy_params, int argc, char *argv[]) {
	if (argc != 4) {
		return 1;
	}
	proxy_params->remote_host = argv[1];
	if ((proxy_params->remote_port = atoi(argv[2])) == 0) {
		return 1;
	}
	if ((proxy_params->proxy_server_port = atoi(argv[3])) == 0) {
		return 1;
	}
	return 0;
}

