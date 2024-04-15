#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct map_s {
	size_t width, depth, height;
	uint8_t *blocks;
} map_t;

map_t *map_create(size_t width, size_t depth, size_t height);
void map_destroy(map_t *map);

void map_set(map_t *map, size_t x, size_t y, size_t z, uint8_t block);
uint8_t map_get(map_t *map, size_t x, size_t y, size_t z);
