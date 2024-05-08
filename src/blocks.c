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

#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include "blocks.h"
#include "map.h"
#include "mapgen.h"
#include "rng.h"
#include "server.h"

static void blocktick_gravity(map_t *map, size_t x, size_t y, size_t z, uint8_t block);
static void blocktick_flow(map_t *map, size_t x, size_t y, size_t z, uint8_t block);
static void blocktick_grass_die(map_t *map, size_t x, size_t y, size_t z, uint8_t block);
static void blocktick_grass_grow(map_t *map, size_t x, size_t y, size_t z, uint8_t block);
static void blocktick_tree_grow(map_t *map, size_t x, size_t y, size_t z, uint8_t block);

blockinfo_t blockinfo[num_blocks];

void blocks_init(void) {
	memset(blockinfo, 0, sizeof(blockinfo));
	for (size_t i = 0; i < num_blocks; i++) {
		blockinfo[i].solid = true;
		blockinfo[i].block_light = true;
	}

	blockinfo[air].solid = false;
	blockinfo[air].block_light = false;
	blockinfo[grass].random_tickfunc = blocktick_grass_die;
	blockinfo[dirt].random_tickfunc = blocktick_grass_grow;
	blockinfo[sapling].solid = false;
	blockinfo[sapling].block_light = false;
	blockinfo[sapling].random_tickfunc = blocktick_tree_grow;
	blockinfo[sand].tickfunc = blocktick_gravity;
	blockinfo[gravel].tickfunc = blocktick_gravity;
	blockinfo[bedrock].op_only = true;
	blockinfo[water].solid = false;
	blockinfo[water].tickfunc = blocktick_flow;
	blockinfo[water].ticktime = 4;
	blockinfo[water].op_only = true;
	blockinfo[water_still].op_only = true;
	blockinfo[lava].solid = false;
	blockinfo[lava].tickfunc = blocktick_flow;
	blockinfo[lava].ticktime = 8;
	blockinfo[lava].op_only = true;
	blockinfo[lava_still].op_only = true;
	blockinfo[glass].block_light = false;
	blockinfo[rose].solid = false;
	blockinfo[rose].block_light = false;
	blockinfo[dandelion].solid = false;
	blockinfo[dandelion].block_light = false;
	blockinfo[brown_mushroom].solid = false;
	blockinfo[brown_mushroom].block_light = false;
	blockinfo[red_mushroom].solid = false;
	blockinfo[red_mushroom].block_light = false;
	blockinfo[rope].solid = false;
	blockinfo[rope].block_light = false;
	blockinfo[fire].solid = false;
	blockinfo[fire].block_light = false;
	blockinfo[snow].solid = false;
}

void blocktick_gravity(map_t *map, size_t x, size_t y, size_t z, uint8_t block) {
	size_t yy = y;

	while (!blockinfo[map_get(map, x, yy - 1, z)].solid) {
		if (yy == 0) break;
		yy--;
	}

	if (yy != y) {
		map_set(map, x, y, z, air);
		map_set(map, x, yy, z, block);
	}
}

void blocktick_flow(map_t *map, size_t x, size_t y, size_t z, uint8_t block) {
	if (!blockinfo[map_get(map, x - 1, y, z)].solid) {
		map_set(map, x - 1, y, z, block);
	}
	if (!blockinfo[map_get(map, x + 1, y, z)].solid) {
		map_set(map, x + 1, y, z, block);
	}
	if (!blockinfo[map_get(map, x, y, z - 1)].solid) {
		map_set(map, x, y, z - 1, block);
	}
	if (!blockinfo[map_get(map, x, y, z + 1)].solid) {
		map_set(map, x, y, z + 1, block);
	}
	if (!blockinfo[map_get(map, x, y - 1, z)].solid) {
		map_set(map, x, y - 1, z, block);
	}
}


void blocktick_grass_die(map_t *map, size_t x, size_t y, size_t z, uint8_t block) {
	if (block != grass) {
		return;
	}

	size_t top = map_get_top_lit(map, x, z);
	if (top > y) {
		map_set(map, x, y, z, dirt);
	}
}

void blocktick_grass_grow(map_t *map, size_t x, size_t y, size_t z, uint8_t block) {
	if (block != dirt) {
		return;
	}

	size_t top = map_get_top_lit(map, x, z);
	if (top == y) {
		map_set(map, x, y, z, grass);
	}
}

void blocktick_tree_grow(map_t *map, size_t x, size_t y, size_t z, uint8_t block) {
	if (block != sapling) {
		return;
	}

	size_t top = map_get_top_lit(map, x, z);
	if (top > y) {
		return;
	}

	int treeHeight = rng_next2(server.global_rng, 1, 3) + 4;
	if (mapgen_space_for_tree(map, x, y, z, treeHeight)) {
		mapgen_grow_tree(map, server.global_rng, x, y, z, treeHeight);
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
