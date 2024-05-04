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

static void blocktick_gravity(map_t *map, size_t x, size_t y, size_t z, uint8_t block);
static void blocktick_flow(map_t *map, size_t x, size_t y, size_t z, uint8_t block);

blockinfo_t blockinfo[num_blocks];

void blocks_init(void) {
	memset(blockinfo, 0, sizeof(blockinfo));
	for (size_t i = 0; i < num_blocks; i++) {
		blockinfo[i].solid = true;
	}

	blockinfo[air].solid = false;
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
	blockinfo[rose].solid = false;
	blockinfo[dandelion].solid = false;
	blockinfo[brown_mushroom].solid = false;
	blockinfo[red_mushroom].solid = false;
	blockinfo[rope].solid = false;
	blockinfo[fire].solid = false;
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
