#include <stdlib.h>
#include <string.h>
#include "map.h"
#include "mapgen.h"
#include "buffer.h"
#include "server.h"
#include "client.h"
#include "packet.h"

map_t *map_create(size_t width, size_t depth, size_t height) {
	map_t *map = malloc(sizeof(*map));
	map->width = width;
	map->depth = depth;
	map->height = height;
	map->blocks = malloc(width * depth * height);
	memset(map->blocks, 0, width * depth * height);

	mapgen_flat(map);

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

	for (size_t i = 0; i < server.num_clients; i++) {
		client_t *client = &server.clients[i];
		buffer_write_uint8(client->out_buffer, packet_set_block_server);
		buffer_write_uint16be(client->out_buffer, x);
		buffer_write_uint16be(client->out_buffer, y);
		buffer_write_uint16be(client->out_buffer, z);
		buffer_write_uint8(client->out_buffer, block);
		client_flush(client);
	}
}

uint8_t map_get(map_t *map, size_t x, size_t y, size_t z) {
	return map->blocks[map_get_block_index(map, x, y, z)];
}
