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

#include <stdlib.h>
#include <string.h>
#include "map.h"
#include "mapgen.h"
#include "buffer.h"
#include "server.h"
#include "client.h"
#include "packet.h"
#include "blocks.h"
#include "config.h"
#include "mapimage.h"
#include "rng.h"
#include "util.h"

map_t *map_create(const char *name, size_t width, size_t depth, size_t height) {
	map_t *map = malloc(sizeof(*map));
	map->name = strdup(name);
	map->width = width;
	map->depth = depth;
	map->height = height;
	map->blocks = malloc(width * depth * height);
	map->generating = false;
	map->num_ticks = 0;
	map->ticks = NULL;
	map->modified = true;

	memset(map->blocks, 0, width * depth * height);

	return map;
}

void map_destroy(map_t *map) {
	free(map->name);
	free(map->blocks);
	free(map);
}

void map_set(map_t *map, size_t x, size_t y, size_t z, uint8_t block) {
	if (!map_pos_valid(map, x, y, z) || map_get(map, x, y, z) == block) {
		return;
	}

	const uint8_t old_block = map_get(map, x, y, z);

	map->blocks[map_get_block_index(map, x, y, z)] = block;

	if (!map->generating) {
		if (blockinfo[old_block].breakfunc != NULL) {
			blockinfo[old_block].breakfunc(map, x, y, z, old_block);
		}

		if (blockinfo[block].placefunc != NULL) {
			blockinfo[block].placefunc(map, x, y, z, block);
		}

		uint64_t ticktime = blockinfo[block].ticktime;
		uint64_t dist = ticktime == 0 ? 0 : (((server.tick / ticktime) + 1) * ticktime) - server.tick;
		map_add_tick(map, x, y, z, dist);
		map_add_tick(map, x + 1, y, z, dist);
		map_add_tick(map, x - 1, y, z, dist);
		map_add_tick(map, x, y - 1, z, dist);
		map_add_tick(map, x, y + 1, z, dist);
		map_add_tick(map, x, y, z - 1, dist);
		map_add_tick(map, x, y, z + 1, dist);
	}

	for (size_t i = 0; i < server.num_clients; i++) {
		client_t *client = &server.clients[i];
		buffer_write_uint8(client->out_buffer, packet_set_block_server);
		buffer_write_uint16be(client->out_buffer, x);
		buffer_write_uint16be(client->out_buffer, y);
		buffer_write_uint16be(client->out_buffer, z);
		buffer_write_uint8(client->out_buffer, map_get(map, x, y, z));
	}

	map->modified = true;
}

uint8_t map_get(map_t *map, size_t x, size_t y, size_t z) {
	return map->blocks[map_get_block_index(map, x, y, z)];
}

size_t map_get_top(map_t *map, size_t x, size_t z) {
	size_t yy;

	for (yy = map->depth - 1; yy > 0; yy--) {
		const uint8_t block = map_get(map, x, yy, z);
		if (block != air) {
			break;
		}
	}

	return yy;
}

size_t map_get_top_lit(map_t *map, size_t x, size_t z) {
	size_t yy;

	for (yy = map->depth - 1; yy > 0; yy--) {
		const uint8_t block = map_get(map, x, yy, z);
		if (blockinfo[block].block_light) {
			break;
		}
	}

	return yy;
}

void map_tick(map_t *map) {
	bool resized = false;

	size_t random_ticks = (map->width * map->depth * map->height) / (16 * 16 * 16);
	for (size_t i = 0; i < random_ticks; i++) {
		size_t x = rng_next(server.global_rng, map->width);
		size_t y = rng_next(server.global_rng, map->depth);
		size_t z = rng_next(server.global_rng, map->height);
		uint8_t block = map_get(map, x, y, z);
		if (blockinfo[block].random_tickfunc != NULL) {
			blockinfo[block].random_tickfunc(map, x, y, z, block);
		}
	}

	for (size_t i = 0; i < map->num_ticks; i++) {
		scheduledtick_t *tick = &map->ticks[i];
		if (server.tick < tick->time) {
			continue;
		}

		uint8_t block = map_get(map, tick->x, tick->y, tick->z);
		if (blockinfo[block].tickfunc != NULL) {
			blockinfo[block].tickfunc(map, tick->x, tick->y, tick->z, block);
		}

		memmove(map->ticks + i, map->ticks + i + 1, (map->num_ticks - i - 1) * sizeof(*map->ticks));
		map->num_ticks--;
		resized = true;
	}

	if (resized) {
		if (map->num_ticks == 0) {
			free(map->ticks);
			map->ticks = 0;
		}
		else {
			map->ticks = realloc(map->ticks, sizeof(*map->ticks) * map->num_ticks);
		}
	}

	int image_interval = config.map.image_interval * 20;
	if (image_interval > 0 && server.tick % image_interval == 0) {
		map_save_image_threaded(map, config.map.image_path);
	}
}

void map_add_tick(map_t *map, size_t x, size_t y, size_t z, uint64_t num_ticks_until) {
	if (!map_pos_valid(map, x, y, z)) {
		return;
	}

	uint8_t block = map_get(map, x, y, z);
	if (blockinfo[block].tickfunc == NULL) {
		return;
	}

	size_t idx = map->num_ticks++;
	map->ticks = realloc(map->ticks, sizeof(*map->ticks) * map->num_ticks);
	map->ticks[idx].x = x;
	map->ticks[idx].y = y;
	map->ticks[idx].z = z;
	map->ticks[idx].time = server.tick + num_ticks_until;
}
