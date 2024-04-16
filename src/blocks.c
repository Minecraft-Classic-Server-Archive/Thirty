#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include "blocks.h"
#include "map.h"

static void blocktick_gravity(map_t *map, size_t x, size_t y, size_t z, uint8_t block);

blockinfo_t blockinfo[num_blocks];

void blocks_init(void) {
	memset(blockinfo, 0, sizeof(blockinfo));

	blockinfo[sand].tickfunc = blocktick_gravity;
	blockinfo[gravel].tickfunc = blocktick_gravity;
}

void blocktick_gravity(map_t *map, size_t x, size_t y, size_t z, uint8_t block) {
	size_t yy = y;

	while (map_get(map, x, yy - 1, z) == air) {
		if (yy == 0) break;
		yy--;
	}

	if (yy != y) {
		map_set(map, x, y, z, air);
		map_set(map, x, yy, z, block);
	}
}

uint8_t block_get_fallback(uint8_t block) {
	switch (block) {
		case cobblestone_slab:
			return slab;
		case rope:
			return brown_mushroom;
		case sandstone:
			return sand;
		case snow:
			return air;
		case fire:
			return lava;
		case light_pink_wool:
			return pink_wool;
		case forest_green_wool:
			return green_wool;
		case brown_wool:
			return dirt;
		case deep_blue:
			return blue_wool;
		case turquoise:
			return cyan_wool;
		case ice:
			return glass;
		case ceramic_tile:
			return iron_block;
		case magma:
			return obsidian;
		case pillar:
			return white_wool;
		case crate:
			return wood_planks;
		case stone_brick:
			return stone;

		default: return block;
	}
}
