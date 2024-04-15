#pragma once
#include <stdint.h>

#define CPE_CUSTOMBLOCKS_LEVEL 1

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

uint8_t block_get_fallback(uint8_t block);
