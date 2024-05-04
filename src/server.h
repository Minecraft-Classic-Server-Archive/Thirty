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

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "sockets.h"

typedef struct client_s client_t;
typedef struct map_s map_t;
typedef struct rng_s rng_t;
typedef struct namelist_s namelist_t;

typedef struct server_s {
	socket_t socket_fd;
	uint16_t port;

	uint64_t tick;

	client_t *clients;
	size_t num_clients;

	map_t *map;
	rng_t *global_rng;

	char salt[17];
	double last_heartbeat;

	namelist_t *ops;
	namelist_t *banned_users;
	namelist_t *banned_ips;
	namelist_t *whitelist;
} server_t;

bool server_init(void);
void server_tick(void);
void server_shutdown(void);

void server_heartbeat(void);

void server_broadcast(const char *msg, ...) __attribute__((format(printf, 1, 2)));

extern server_t server;
