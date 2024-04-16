#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "sockets.h"

typedef struct client_s client_t;
typedef struct map_s map_t;
typedef struct rng_s rng_t;

typedef struct server_s {
	socket_t socket_fd;
	uint16_t port;

	uint64_t tick;

	client_t *clients;
	size_t num_clients;

	map_t *map;
	rng_t *global_rng;
} server_t;

bool server_init(void);
void server_tick(void);
void server_shutdown(void);

void server_broadcast(const char *msg, ...) __attribute__((format(printf, 1, 2)));

extern server_t server;
