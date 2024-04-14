#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "server.h"

server_t server;

bool server_init() {
	server.port = (uint16_t)25565;

	int err;

	err = (server.socket_fd = socket(AF_INET, SOCK_STREAM, 0));

	if (err == -1) {
		perror("socket");
		return false;
	}

	struct sockaddr_in server_addr = { 0 };
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons((uint16_t)server.port);

	int yes = 1;
	err = setsockopt(server.socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	if (err == -1) {
		perror("setsockopt(SO_REUSEADDR)");
		return false;
	}

	err = bind(server.socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (err == -1) {
		perror("bind");
		return false;
	}

	// TODO: magic number
	err = listen(server.socket_fd, 10);
	if (err == -1) {
		perror("listen");
		return false;
	}

	ioctl(server.socket_fd, FIONBIO, &yes);

	return true;
}

void server_shutdown() {
	close(server.socket_fd);
}
