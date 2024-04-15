#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include "server.h"
#include "client.h"
#include "map.h"

void server_accept();

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

	printf("Server is listening on port %u\n", server.port);

	server.map = map_create(64, 64, 64);

	return true;
}

void server_shutdown() {
	map_destroy(server.map);
	close(server.socket_fd);
}

void server_tick() {
	server_accept();

	for (size_t i = 0; i < server.num_clients; i++) {
		client_tick(&server.clients[i]);
	}

	server.tick++;
}

void server_accept() {
	struct sockaddr_storage client_addr;
	socklen_t addr_size = sizeof(client_addr);

	int acceptfd = accept(server.socket_fd, (struct sockaddr *)&client_addr, &addr_size);
	if (acceptfd == -1) {
		int e = errno;
		if (e == EAGAIN || e == EWOULDBLOCK) {
			// No connections.
			return;
		}

		perror("accept");
		return;
	}

	int yes = 1;
	ioctl(acceptfd, FIONBIO, &yes);

	struct sockaddr_in *sin = (struct sockaddr_in *)&client_addr;

	uint8_t *ip = (uint8_t *)&sin->sin_addr.s_addr;
	printf("Incoming connection from %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);

	size_t conn_idx = server.num_clients++;
	server.clients = realloc(server.clients, server.num_clients * sizeof(*server.clients));
	client_t *client = &server.clients[conn_idx];
	client_init(client, acceptfd, conn_idx);
}