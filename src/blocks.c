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

static bool can_liquid_flow_to(map_t *map, size_t x, size_t y, size_t z);

static void blocktick_gravity(map_t *map, size_t x, size_t y, size_t z, uint8_t block);
static void blocktick_flow(map_t *map, size_t x, size_t y, size_t z, uint8_t block);
static void blocktick_grass_die(map_t *map, size_t x, size_t y, size_t z, uint8_t block);
static void blocktick_grass_grow(map_t *map, size_t x, size_t y, size_t z, uint8_t block);
static void blocktick_tree_grow(map_t *map, size_t x, size_t y, size_t z, uint8_t block);

static void blockplace_sponge(map_t *map, size_t x, size_t y, size_t z, uint8_t block);
static void blockbreak_sponge(map_t *map, size_t x, size_t y, size_t z, uint8_t block);
static void blockplace_liquid(map_t *map, size_t x, size_t y, size_t z, uint8_t block);

blockinfo_t blockinfo[num_blocks];

void blocks_init(void) {
	memset(blockinfo, 0, sizeof(blockinfo));
	for (size_t i = 0; i < num_blocks; i++) {
		blockinfo[i].solid = true;
		blockinfo[i].block_light = true;
		blockinfo[i].colour = 0xFF00FF;
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
	blockinfo[bedrock].op_only_place = true;
	blockinfo[bedrock].op_only_break = true;
	blockinfo[water].solid = false;
	blockinfo[water].liquid = true;
	blockinfo[water].tickfunc = blocktick_flow;
	blockinfo[water].ticktime = 4;
	blockinfo[water].op_only_place = true;
	blockinfo[water].placefunc = blockplace_liquid;
	blockinfo[water_still].liquid = true;
	blockinfo[water_still].op_only_place = true;
	blockinfo[water].placefunc = blockplace_liquid;
	blockinfo[lava].solid = false;
	blockinfo[lava].liquid = true;
	blockinfo[lava].tickfunc = blocktick_flow;
	blockinfo[lava].ticktime = 8;
	blockinfo[lava].op_only_place = true;
	blockinfo[water].placefunc = blockplace_liquid;
	blockinfo[lava_still].liquid = true;
	blockinfo[lava_still].op_only_place = true;
	blockinfo[water].placefunc = blockplace_liquid;
	blockinfo[leaves].block_light = false;
	blockinfo[sponge].placefunc = blockplace_sponge;
	blockinfo[sponge].breakfunc = blockbreak_sponge;
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

	blockinfo[air].colour = 0x000000U;
	blockinfo[stone].colour = 0x7f7f7fU;
	blockinfo[grass].colour = 0x86b556U;
	blockinfo[dirt].colour = 0x966c4aU;
	blockinfo[cobblestone].colour = 0x515151U;
	blockinfo[wood_planks].colour = 0xbc9862U;
	blockinfo[sapling].colour = 0x168700U;
	blockinfo[bedrock].colour = 0x333333U;
	blockinfo[water].colour = 0x0000FFU;
	blockinfo[water_still].colour = 0x0000FFU;
	blockinfo[lava].colour = 0xFF8000U;
	blockinfo[lava_still].colour = 0xFF8000U;
	blockinfo[sand].colour = 0xE7DBAFU;
	blockinfo[gravel].colour = 0x9A8285U;
	blockinfo[gold_ore].colour = 0xFCEE4BU;
	blockinfo[iron_ore].colour = 0xE2C0AAU;
	blockinfo[coal_ore].colour = 0x373737U;
	blockinfo[wood].colour = 0x6A5534U;
	blockinfo[leaves].colour = 0x5cfc3cU;
	blockinfo[sponge].colour = 0xCECE46U;
	blockinfo[glass].colour = 0xC0F5FEU;
	blockinfo[red_wool].colour = 0xE73434U;
	blockinfo[orange_wool].colour = 0xE58C34U;
	blockinfo[yellow_wool].colour = 0xEEEE36U;
	blockinfo[lime_wool].colour = 0x9CFF3AU;
	blockinfo[green_wool].colour = 0x34E834U;
	blockinfo[aquagreen_wool].colour = 0x34E58CU;
	blockinfo[cyan_wool].colour = 0x34E8E8U;
	blockinfo[blue_wool].colour = 0x69A5E0U;
	blockinfo[purple_wool].colour = 0x7B7BE4U;
	blockinfo[indigo_wool].colour = 0x8F35EBU;
	blockinfo[violet_wool].colour = 0xA144CDU;
	blockinfo[magenta_wool].colour = 0xFF3AFFU;
	blockinfo[pink_wool].colour = 0xE8348EU;
	blockinfo[black_wool].colour = 0x525252U;
	blockinfo[grey_wool].colour = 0x9B9B9BU;
	blockinfo[white_wool].colour = 0xFFFFFFU;
	blockinfo[dandelion].colour = 0xCCD302U;
	blockinfo[rose].colour = 0xD10609U;
	blockinfo[brown_mushroom].colour = 0x916D55U;
	blockinfo[red_mushroom].colour = 0xFE2A2AU;
	blockinfo[gold_block].colour = 0xFFFE73U;
	blockinfo[iron_block].colour = 0xFF0000U;
	blockinfo[double_slab].colour = 0xCDCDCDU;
	blockinfo[slab].colour = 0xCDCDCDU;
	blockinfo[bricks].colour = 0xDB441AU;
	blockinfo[tnt].colour = 0xDB441AU;
	blockinfo[bookshelf].colour = 0xBC9862U;
	blockinfo[mossy_cobblestoe].colour = 0x214B21U;
	blockinfo[obsidian].colour = 0x3C3056U;
}

bool can_liquid_flow_to(map_t *map, size_t x, size_t y, size_t z) {
	const uint8_t block = map_get(map, x, y, z);
	if (blockinfo[block].solid) {
		return false;
	}

	for (size_t xx = x - 2; xx <= x + 2; xx++)
		for (size_t yy = y - 2; yy <= y + 2; yy++)
			for (size_t zz = z - 2; zz <= z + 2; zz++) {
				if (map_get(map, xx, yy, zz) == sponge) {
					return false;
				}
			}

	return true;
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
	if (can_liquid_flow_to(map, x - 1, y, z)) {
		map_set(map, x - 1, y, z, block);
	}
	if (can_liquid_flow_to(map, x + 1, y, z)) {
		map_set(map, x + 1, y, z, block);
	}
	if (can_liquid_flow_to(map, x, y, z - 1)) {
		map_set(map, x, y, z - 1, block);
	}
	if (can_liquid_flow_to(map, x, y, z + 1)) {
		map_set(map, x, y, z + 1, block);
	}
	if (can_liquid_flow_to(map, x, y - 1, z)) {
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

void blockplace_sponge(map_t *map, size_t x, size_t y, size_t z, uint8_t block) {
	(void) block;

	for (size_t xx = x - 2; xx <= x + 2; xx++)
	for (size_t yy = y - 2; yy <= y + 2; yy++)
	for (size_t zz = z - 2; zz <= z + 2; zz++) {
		if (blockinfo[map_get(map, xx, yy, zz)].liquid) {
			map_set(map, xx, yy, zz, air);
		}
	}
}

void blockbreak_sponge(map_t *map, size_t x, size_t y, size_t z, uint8_t block) {
	(void) block;

	for (size_t xx = x - 3; xx <= x + 3; xx++)
	for (size_t yy = y - 3; yy <= y + 3; yy++)
	for (size_t zz = z - 3; zz <= z + 3; zz++) {
		const uint8_t block = map_get(map, xx, yy, zz);
		if (blockinfo[block].liquid) {
			map_add_tick(map, xx, yy, zz, blockinfo[block].ticktime);
		}
	}
}

void blockplace_liquid(map_t *map, size_t x, size_t y, size_t z, uint8_t block) {
	(void) block;
	
	for (size_t xx = x - 2; xx <= x + 2; xx++)
	for (size_t yy = y - 2; yy <= y + 2; yy++)
	for (size_t zz = z - 2; zz <= z + 2; zz++) {
		if (map_get(map, xx, yy, zz) == sponge) {
			map_set(map, x, y, z, air);
		}
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
