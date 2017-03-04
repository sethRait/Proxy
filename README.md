A simple TCP proxy server written in C. In this implementation, the server accepts only one client at a time and the destination (remote host) needs to be specified at runtime.

# Usage
	make
	./tcpproxy <host_name> <remote_port> <local_port>
