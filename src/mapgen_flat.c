#include "mapgen.h"
#include "map.h"
#include "blocks.h"

void mapgen_flat(map_t *map) {
	unsigned int waterLevel = (map->depth / 2) - 1;

	for (size_t x = 0; x < map->width; x++)
	for (size_t y = 0; y <= waterLevel; y++)
	for (size_t z = 0; z < map->height; z++) {
		uint8_t block;

		if (y == waterLevel) {
			block = grass;
		}
		else if (y >= waterLevel - 4) {
			block = dirt;
		}
		else {
			block = stone;
		}

		map_set(map, x, y, z, block);
	}
}
