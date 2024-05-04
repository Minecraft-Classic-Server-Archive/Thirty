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
#include <pthread.h>
#include "sockets.h"
#include "cpe.h"

struct buffer_s;

enum {
	mapsend_none,
	mapsend_running,
	mapsend_success,
	mapsend_sent,
	mapsend_failure
};

typedef struct client_s {
	socket_t socket_fd;
	bool connected;
	size_t idx;
	bool is_op;

	struct buffer_s *in_buffer;
	struct buffer_s *out_buffer;
	pthread_mutex_t out_mutex;

	int mapsend_state;
	struct buffer_s *mapgz_buffer;

	double last_ping;
	double ping;
	uint16_t ping_key;

	char name[64];

	bool spawned;
	float x, y, z;
	float yaw, pitch;

	size_t num_extensions;
	cpeext_t *extensions;

	int customblocks_support;

	bool ws_can_switch, using_websocket;
	unsigned int ws_state, ws_opcode, ws_frame_len, ws_frame_read, ws_mask_read;
	uint8_t ws_mask[4];
	struct buffer_s *ws_frame;
	struct buffer_s *ws_out_buffer;
} client_t;

void client_init(client_t *client, int fd, size_t idx);
void client_destroy(client_t *client);
void client_tick(client_t *client);
void client_flush(client_t *client);
void client_flush_buffer(client_t *client, struct buffer_s *buffer);
void client_disconnect(client_t *client, const char *msg);

bool client_supports_extension(client_t *client, const char *name, int version);

typedef struct {
	client_t *client;
	uint8_t *data;
	size_t width, height, depth;
} mapsend_t;

void *mapsend_thread_start(void *data);
void *mapsend_fast_thread_start(void *data);
