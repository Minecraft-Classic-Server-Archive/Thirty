#include "mapgen.h"
#include "map.h"
#include "rng.h"
#include "blocks.h"

void mapgen_debug(map_t *map) {
	for (size_t x = 0; x < map->width; x++)
		for (size_t z = 0; z < map->height; z++) {
			map_set(map, x, 0, z, bedrock);
			map_set(map, x, 1, z, stone);
		}

	for (size_t i = 0; i < num_blocks; i++)
		for (size_t z = 0; z < map->height; z++) {
			map_set(map, i, 2, z, i);
		}
}

void mapgen_random(map_t *map) {
	rng_t *rng = rng_create(0);

	for (size_t i = 0; i < map->width * map->depth * map->height; i++) {
		map->blocks[i] = rng_next(rng, num_blocks);
	}

	rng_destroy(rng);
}
