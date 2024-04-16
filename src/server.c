#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "server.h"
#include "buffer.h"
#include "client.h"
#include "map.h"
#include "packet.h"
#include "rng.h"

void server_accept(void);

server_t server;

bool server_init(void) {
	server.port = (uint16_t)25565;
	server.global_rng = rng_create((int)time(NULL));

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
#ifdef _WIN32
	err = setsockopt(server.socket_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));
#else
	err = setsockopt(server.socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#endif
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

#ifdef _WIN32
	ioctlsocket(server.socket_fd, FIONBIO, (u_long *)&yes);
#else
	ioctlsocket(server.socket_fd, FIONBIO, &yes);
#endif

	printf("Server is listening on port %u\n", server.port);

	printf("Preparing map...\n");
	server.map = map_create(256, 256, 256);

	return true;
}

void server_shutdown(void) {
	map_destroy(server.map);
	closesocket(server.socket_fd);
	rng_destroy(server.global_rng);
}

void server_tick(void) {
	server_accept();
	map_tick(server.map);

	for (size_t i = 0; i < server.num_clients; i++) {
		client_tick(&server.clients[i]);
	}

	bool removed = false;
	for (size_t i = 0; i < server.num_clients; i++) {
		client_t *client = &server.clients[i];
		if (client->connected) {
			continue;
		}

		client_destroy(client);

		memmove(server.clients + i, server.clients + i + 1, (server.num_clients - i - 1) * sizeof(*server.clients));
		server.num_clients--;
		removed = true;
	}

	if (removed) {
		server.clients = realloc(server.clients, sizeof(*server.clients) * server.num_clients);
	}

	server.tick++;
}

void server_accept(void) {
	struct sockaddr_storage client_addr;
	socklen_t addr_size = sizeof(client_addr);

	socket_t acceptfd = accept(server.socket_fd, (struct sockaddr *)&client_addr, &addr_size);
	if (acceptfd == INVALID_SOCKET) {
		int e = socket_error();
		if (e == EAGAIN || e == SOCKET_EWOULDBLOCK) {
			// No connections.
			return;
		}

		fprintf(stderr, "accept error %d\n", e);
		return;
	}

	int yes = 1;
#ifdef _WIN32
	ioctlsocket(server.socket_fd, FIONBIO, (u_long *)&yes);
#else
	ioctlsocket(server.socket_fd, FIONBIO, &yes);
#endif

	struct sockaddr_in *sin = (struct sockaddr_in *)&client_addr;

	uint8_t *ip = (uint8_t *)&sin->sin_addr.s_addr;
	printf("Incoming connection from %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);

	size_t conn_idx = server.num_clients++;
	server.clients = realloc(server.clients, server.num_clients * sizeof(*server.clients));
	client_t *client = &server.clients[conn_idx];
	client_init(client, acceptfd, conn_idx);
}

void server_broadcast(const char *msg, ...) {
	char buffer[65];

	va_list args;
	va_start(args, msg);
	vsnprintf(buffer, 65, msg, args);
	va_end(args);

	puts(buffer);

	for (size_t i = 0; i < server.num_clients; i++) {
		client_t *client = &server.clients[i];
		buffer_write_uint8(client->out_buffer, packet_message);
		buffer_write_uint8(client->out_buffer, 0xFF);
		buffer_write_mcstr(client->out_buffer, buffer, !client_supports_extension(client, "FullCP437", 1));
		client_flush(client);
	}
}
