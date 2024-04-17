// classicserver, a ClassiCube (Minecraft Classic) server
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

#pragma once
#include <stdint.h>

#define CPE_CUSTOMBLOCKS_LEVEL 1

typedef struct map_s map_t;

typedef void (*blocktickfunc_t)(map_t *map, size_t x, size_t y, size_t z, uint8_t block);

enum {
	air,
	stone,
	grass,
	dirt,
	cobblestone,
	wood_planks,
	sapling,
	bedrock,
	water,
	water_still,
	lava,
	lava_still,
	sand,
	gravel,
	gold_ore,
	iron_ore,
	coal_ore,
	wood,
	leaves,
	sponge,
	glass,
	red_wool,
	orange_wool,
	yellow_wool,
	lime_wool,
	green_wool,
	aquagreen_wool,
	cyan_wool,
	blue_wool,
	purple_wool,
	indigo_wool,
	violet_wool,
	magenta_wool,
	pink_wool,
	black_wool,
	grey_wool,
	white_wool,
	dandelion,
	rose,
	brown_mushroom,
	red_mushroom,
	gold_block,
	iron_block,
	double_slab,
	slab,
	bricks,
	tnt,
	bookshelf,
	mossy_cobblestoe,
	obsidian,

	// CPE blocks.
	cobblestone_slab,
	rope,
	sandstone,
	snow,
	fire,
	light_pink_wool,
	forest_green_wool,
	brown_wool,
	deep_blue,
	turquoise,
	ice,
	ceramic_tile,
	magma,
	pillar,
	crate,
	stone_brick,

	num_blocks
};

typedef struct blockinfo_s {
	bool solid;

	blocktickfunc_t tickfunc;
	uint64_t ticktime;
} blockinfo_t;

extern blockinfo_t blockinfo[num_blocks];

void blocks_init(void);

uint8_t block_get_fallback(uint8_t block);
