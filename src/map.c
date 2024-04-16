#include <stdlib.h>
#include <string.h>
#include "map.h"
#include "mapgen.h"
#include "buffer.h"
#include "server.h"
#include "client.h"
#include "packet.h"
#include "blocks.h"

map_t *map_create(size_t width, size_t depth, size_t height) {
	map_t *map = malloc(sizeof(*map));
	map->width = width;
	map->depth = depth;
	map->height = height;
	map->blocks = malloc(width * depth * height);
	map->generating = true;
	map->num_ticks = 0;
	map->ticks = NULL;

	memset(map->blocks, 0, width * depth * height);

	mapgen_flat(map);

	map->generating = false;

	return map;
}

void map_destroy(map_t *map) {
	free(map->blocks);
	free(map);
}

void map_set(map_t *map, size_t x, size_t y, size_t z, uint8_t block) {
	if (!map_pos_valid(map, x, y, z)) {
		return;
	}

	map->blocks[map_get_block_index(map, x, y, z)] = block;

	if (!map->generating) {
		map_add_tick(map, x, y, z, 1);
		map_add_tick(map, x + 1, y, z, 1);
		map_add_tick(map, x - 1, y, z, 1);
		map_add_tick(map, x, y - 1, z, 1);
		map_add_tick(map, x, y + 1, z, 1);
		map_add_tick(map, x, y, z - 1, 1);
		map_add_tick(map, x, y, z + 1, 1);
	}

	for (size_t i = 0; i < server.num_clients; i++) {
		client_t *client = &server.clients[i];
		buffer_write_uint8(client->out_buffer, packet_set_block_server);
		buffer_write_uint16be(client->out_buffer, x);
		buffer_write_uint16be(client->out_buffer, y);
		buffer_write_uint16be(client->out_buffer, z);
		buffer_write_uint8(client->out_buffer, block);
	}
}

uint8_t map_get(map_t *map, size_t x, size_t y, size_t z) {
	return map->blocks[map_get_block_index(map, x, y, z)];
}

void map_tick(map_t *map) {
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
	}

	map->ticks = realloc(map->ticks, sizeof(*map->ticks) * map->num_ticks);
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
