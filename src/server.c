// Thirty, a ClassiCube (Minecraft Classic) server
// Copyright (C) 2024 Sean Baggaley
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
#include "util.h"
#include "mapgen.h"
#include "config.h"
#include "namelist.h"

#ifndef _WIN32
#include <netinet/tcp.h>
#endif

#define HEARTBEAT_INTERVAL (45.0)

void server_accept(void);
void server_generate_salt(char *out, size_t length);

server_t server;

bool server_init(void) {
	server.port = config.server.port;
	server.global_rng = rng_create((int)time(NULL));
	server.last_heartbeat = 0.0;

	if (config.debug.fixed_salt[0] == '\0') {
		server_generate_salt(server.salt, 16);
	}
	else {
		strncpy(server.salt, config.debug.fixed_salt, 16);
	}

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
	server.map = map_load(config.map.name);

	if (server.map == NULL) {
		printf("Failed to load map '%s', generating new...\n", config.map.name);
		server.map = map_create(config.map.name, config.map.width, config.map.depth, config.map.height);

		printf("Generating map...\n");
		double start = get_time_s();
		map_generate(server.map, config.map.generator);
		double duration = get_time_s() - start;
		printf("Map generation took %f seconds\n", duration);

		map_save(server.map);
	}

	server.ops = namelist_create("ops.txt");
	server.banned_users = namelist_create("banned_users.txt");
	server.banned_ips = namelist_create("banned_ips.txt");
	server.whitelist = namelist_create("whitelist.txt");

	server_heartbeat();
	server.last_heartbeat = get_time_s();

	return true;
}

void server_shutdown(void) {
	namelist_destroy(server.whitelist);
	namelist_destroy(server.banned_ips);
	namelist_destroy(server.banned_users);
	namelist_destroy(server.ops);
	map_save(server.map);
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
		if (server.num_clients == 0) {
			free(server.clients);
			server.clients = NULL;
			map_save(server.map);
		}
		else {
			server.clients = realloc(server.clients, sizeof(*server.clients) * server.num_clients);
		}
	}

	if (get_time_s() - server.last_heartbeat > HEARTBEAT_INTERVAL) {
		server_heartbeat();
		server.last_heartbeat = get_time_s();
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
	const char *i_hate_winsock = 1;
	ioctlsocket(acceptfd, FIONBIO, (u_long *)&yes);
	setsockopt(acceptfd, IPPROTO_TCP, TCP_NODELAY, &i_hate_winsock, sizeof(i_hate_winsock));
#else
	ioctlsocket(acceptfd, FIONBIO, &yes);
	setsockopt(acceptfd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
#endif

	struct sockaddr_in *sin = (struct sockaddr_in *)&client_addr;
	uint8_t *ip = (uint8_t *)&sin->sin_addr.s_addr;

	char addrstr[64];
	snprintf(addrstr, sizeof(addrstr), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);

	printf("Incoming connection from %s:%u\n", addrstr, sin->sin_port);

	size_t conn_idx = server.num_clients++;
	server.clients = realloc(server.clients, server.num_clients * sizeof(*server.clients));
	client_t *client = &server.clients[conn_idx];
	client_init(client, acceptfd, conn_idx);
	memcpy(client->address, ip, sizeof(client->address));
	client->port = sin->sin_port;

	if (namelist_contains(server.banned_ips, addrstr)) {
		printf("Client %s is banned!\n", addrstr);
		client_disconnect(client, "You are banned from this server!");
	}
}

void server_broadcast(const char *msg, ...) {
	char buffer[65];

	va_list args;
	va_start(args, msg);
	vsnprintf(buffer, 65, msg, args);
	va_end(args);

	util_print_coloured(buffer);

	for (size_t i = 0; i < server.num_clients; i++) {
		client_t *client = &server.clients[i];
		buffer_write_uint8(client->out_buffer, packet_message);
		buffer_write_uint8(client->out_buffer, 0xFF);
		buffer_write_mcstr(client->out_buffer, buffer, !client_supports_extension(client, "FullCP437", 1));
		client_flush(client);
	}
}

void server_generate_salt(char *out, size_t length) {
	const char *chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	const size_t num_chars = strlen(chars);

	for (size_t i = 0; i < length; i++) {
		out[i] = chars[rng_next(server.global_rng, (int)num_chars)];
	}
}
