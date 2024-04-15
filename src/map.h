#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct map_s {
	size_t width, depth, height;
	uint8_t *blocks;
} map_t;

map_t *map_create(size_t width, size_t depth, size_t height);
void map_destroy(map_t *map);

void map_set(map_t *map, size_t x, size_t y, size_t z, uint8_t block);
uint8_t map_get(map_t *map, size_t x, size_t y, size_t z);

static inline bool map_pos_valid(map_t *map, size_t x, size_t y, size_t z) {
	return x < map->width && y < map->depth && z < map->height;
}

static inline size_t map_get_block_index(map_t *map, size_t x, size_t y, size_t z) {
	return (y * map->height + z) * map->width + x;
}

void mapgen_classic(map_t *map);
void mapgen_debug(map_t *map);
void mapgen_random(map_t *map);
