#include "blocks.h"

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
