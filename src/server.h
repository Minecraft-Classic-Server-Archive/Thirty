#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct client_s client_t;
typedef struct map_s map_t;

typedef struct server_s {
	int socket_fd;
	uint16_t port;

	uint64_t tick;

	client_t *clients;
	size_t num_clients;

	map_t *map;
} server_t;

bool server_init();
void server_tick();
void server_shutdown();

extern server_t server;