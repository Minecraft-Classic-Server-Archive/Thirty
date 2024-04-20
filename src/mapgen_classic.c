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

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "mapgen.h"
#include "rng.h"
#include "map.h"
#include "blocks.h"
#include "perlin.h"
#include "util.h"
#include "config.h"

// Minecraft Classic world generator.
// Partially derived from ClassiCube client under BSD 3-clause (c) 2014 - 2022, UnknownShadow200

typedef struct genstate_s {
	rng_t *rng;
	unsigned int *heightmap;
} genstate_t;

static void gen_heightmap(map_t *map, genstate_t *state);
static void gen_strata(map_t *map, genstate_t *state);
static void gen_lava(map_t *map, genstate_t *state);
static void gen_surface(map_t *map, genstate_t *state);

void mapgen_classic(map_t *map) {
	genstate_t state;
	state.rng = rng_create(config.map.random_seed ? (int)time(NULL) : config.map.seed);
	state.heightmap = calloc(map->width * map->height, sizeof(unsigned int));

	gen_heightmap(map, &state);
	gen_strata(map, &state);
	gen_caves(map, state.rng, true, air);
	gen_ore(map, state.rng, gold_ore, 0.5f);
	gen_ore(map, state.rng, iron_ore, 0.7f);
	gen_ore(map, state.rng, coal_ore, 0.9f);
	gen_water(map, state.rng);
	gen_lava(map, &state);
	gen_surface(map, &state);
	gen_plants(map, state.rng, state.heightmap);

	free(state.heightmap);
	rng_destroy(state.rng);
}

void gen_heightmap(map_t *map, genstate_t *state) {
	combinednoise_t *noise1 = combinednoise_create(octavenoise_create(state->rng, 8), octavenoise_create(state->rng, 8));
	combinednoise_t *noise2 = combinednoise_create(octavenoise_create(state->rng, 8), octavenoise_create(state->rng, 8));
	octavenoise_t *noise3 = octavenoise_create(state->rng, 6);

	state->heightmap = calloc(map->width * map->depth * map->height, sizeof(int));

	for (size_t x = 0; x < map->width; x++)
	for (size_t z = 0; z < map->height; z++) {
		double heightLow = combinednoise_compute2d(noise1, (double)x * 1.3, (double)z * 1.3) / 6 - 4;
		double heightHigh = combinednoise_compute2d(noise2, (double)x * 1.3, (double)z * 1.3) / 5 + 6;
		double heightResult;

		if (octavenoise_compute2d(noise3, (double)x, (double)z) / 8 > 0) {
			heightResult = heightLow;
		} else {
			heightResult = util_max(heightLow, heightHigh);
		}

		heightResult /= 2;

		if (heightResult < 0) {
			heightResult *= 0.8;
		}

		state->heightmap[x + z * map->width] = (unsigned int) (heightResult + ((double)map->depth / 2.0));
	}

	combinednoise_destroy(noise1);
	combinednoise_destroy(noise2);
	octavenoise_destroy(noise3);
}

void gen_strata(map_t *map, genstate_t *state) {
	octavenoise_t *noise = octavenoise_create(state->rng, 8);

	for (size_t x = 0; x < map->width; x++)
	for (size_t z = 0; z < map->height; z++) {
		size_t dirtThickness = (int) octavenoise_compute2d(noise, (double)x, (double)z) / 24 - 4;
		size_t dirtTransition = state->heightmap[x + z * map->width];
		size_t stoneTransition = dirtTransition + dirtThickness;

		for (size_t y = 0; y < map->depth; y++) {
			uint8_t block = air;

			if (y == 0) {
				block = lava;
			} else if (y <= stoneTransition) {
				block = stone;
			} else if (y <= dirtTransition) {
				block = dirt;
			}

			map_set(map, x, y, z, block);
		}
	}

	octavenoise_destroy(noise);
}

void gen_caves(map_t *map, rng_t *rng, bool filter_stone, uint8_t block) {
	unsigned int numCaves = (map->width * map->depth * map->height) / 8192;

	for (unsigned int i = 0; i < numCaves; i++) {
		double caveX = rng_next2(rng, 0, (int)map->width);
		double caveY = rng_next2(rng, 0, (int)map->depth);
		double caveZ = rng_next2(rng, 0, (int)map->height);

		int caveLength = (int) (rng_next_float(rng) * rng_next_float(rng) * 200.0f);

		double theta = rng_next_float(rng) * M_PI * 2;
		double deltaTheta = 0.0;

		double phi = rng_next_float(rng) * M_PI * 2;
		double deltaPhi = 0.0;

		double caveRadius = rng_next_float(rng) * rng_next_float(rng);

		for (double len = 0; len < (double)caveLength; len += 1.0f) {
			caveX += sin(theta) * cos(phi);
			caveY += cos(theta) * cos(phi);
			caveZ += sin(phi);

			theta = theta + deltaTheta * 0.2f;
			deltaTheta = deltaTheta * 0.9f + rng_next_float(rng) - rng_next_float(rng);
			phi = phi / 2.0f + deltaPhi / 4.0f;
			deltaPhi = deltaPhi * 0.75f + rng_next_float(rng) - rng_next_float(rng);

			if (rng_next_float(rng) >= 0.25f) {
				int centreX = (int) (caveX + (double)(rng_next(rng, 4) - 2) * 0.2);
				int centreY = (int) (caveY + (double)(rng_next(rng, 4) - 2) * 0.2);
				int centreZ = (int) (caveZ + (double)(rng_next(rng, 4) - 2) * 0.2);

				double radius = ((double)map->height - centreY) / (double)map->height;
				radius = 1.2 + (radius * 3.5 + 1) * caveRadius;
				radius = radius * sin(len * M_PI / caveLength);

				fill_oblate_spherioid(map, centreX, centreY, centreZ, radius, filter_stone, block);
			}
		}
	}
}

void gen_ore(map_t *map, rng_t *rng, int8_t block, float abundance) {
	int numVeins = (int) (((double) (map->width * map->depth * map->height) * abundance) / 16384.0f);

	for (int i = 0; i < numVeins; i++) {
		double veinX = rng_next2(rng, 0, (int)map->width);
		double veinY = rng_next2(rng, 0, (int)map->depth);
		double veinZ = rng_next2(rng, 0, (int)map->height);

		double veinLength = rng_next_float(rng) * rng_next_float(rng) * 75.0f * abundance;

		double theta = rng_next_float(rng) * M_PI * 2;
		double deltaTheta = 0;
		double phi = rng_next_float(rng) * M_PI * 2;
		double deltaPhi = 0;

		for (float len = 0; len < veinLength; len += 1.0f) {
			veinX = veinX + sin(theta) * cos(phi);
			veinY = veinY + cos(theta) * cos(phi);
			veinZ = veinZ + cos(theta);

			theta = deltaTheta * 0.2;
			deltaTheta = (deltaTheta * 0.9) + rng_next_float(rng) - rng_next_float(rng);
			phi = phi / 2.0 + deltaPhi / 4.0;
			deltaPhi = (deltaPhi * 0.9) + rng_next_float(rng) - rng_next_float(rng);

			double radius = abundance * sin(len * M_PI / veinLength) + 1;

			fill_oblate_spherioid(map, (int) veinX, (int) veinY, (int) veinZ, radius, true, block);
		}
	}
}

void gen_water(map_t *map, rng_t *rng) {
	int waterLevel = ((int)map->depth / 2) - 1;
	int numSources = (int)(map->width * map->height) / 800;

	for (size_t x = 0; x < map->width; x++) {
		flood_fill(map, map_get_block_index(map, x, waterLevel, 0), water);
		flood_fill(map, map_get_block_index(map, x, waterLevel, map->height - 1), water);
	}

	for (size_t z = 0; z < map->height; z++) {
		flood_fill(map, map_get_block_index(map, 0, waterLevel, z), water);
		flood_fill(map, map_get_block_index(map, map->width - 1, waterLevel, z), water);
	}

	for (int i = 0; i < numSources; i++) {
		int x = rng_next2(rng, 0, (int)map->width);
		int z = rng_next2(rng, 0, (int)map->height);
		int y = waterLevel - rng_next2(rng, 0, 2);

		flood_fill(map, map_get_block_index(map, x, y, z), water);
	}
}

void gen_lava(map_t *map, genstate_t *state) {
	int waterLevel = ((int)map->depth / 2) - 1;
	int numSources = (int)(map->width * map->height) / 20000;

	for (int i = 0; i < numSources; i++) {
		int x = rng_next2(state->rng, 0, (int)map->width);
		int z = rng_next2(state->rng, 0, (int)map->height);
		int y = (int) ((float)(waterLevel - 3) * rng_next_float(state->rng) * rng_next_float(state->rng));

		flood_fill(map, map_get_block_index(map, x, y, z), lava);
	}
}

void gen_surface(map_t *map, genstate_t *state) {
	octavenoise_t *noise1 = octavenoise_create(state->rng, 8);
	octavenoise_t *noise2 = octavenoise_create(state->rng, 8);

	for (unsigned int x = 0; x < map->width; x++)
		for (unsigned int z = 0; z < map->height; z++) {
			bool sandChance = octavenoise_compute2d(noise1, x, z) > 8;
			bool gravelChance = octavenoise_compute2d(noise2, x, z) > 12;

			unsigned int y = state->heightmap[x + z * map->width];
			uint8_t above = map_get(map, x, y + 1, z);

			if (above == water && gravelChance) {
				map_set(map, x, y, z, gravel);
			}
			else if (above == air) {
				if (y <= (map->depth / 2) && sandChance) {
					map_set(map, x, y, z, sand);
				}
				else {
					map_set(map, x, y, z, grass);
				}
			}
		}

	octavenoise_destroy(noise1);
	octavenoise_destroy(noise2);
}

void gen_plants(map_t *map, rng_t *rng, unsigned int *heightmap) {
	int numFlowers = (int)(map->width * map->height) / 3000;
	int numShrooms = (int)(map->width * map->depth * map->height) / 2000;
	int numTrees = (int)(map->width * map->height) / 4000;

	for (int i = 0; i < numFlowers; i++) {
		uint8_t flowerType = rng_next_boolean(rng) ? dandelion : rose;

		int patchX = rng_next2(rng, 0, (int)map->width);
		int patchZ = rng_next2(rng, 0, (int)map->height);

		for (int j = 0; j < 10; j++) {
			int flowerX = patchX;
			int flowerZ = patchZ;

			for (int k = 0; k < 5; k++) {
				flowerX += rng_next(rng, 6) - rng_next(rng, 6);
				flowerZ += rng_next(rng, 6) - rng_next(rng, 6);

				if (map_pos_valid(map, flowerX, 0, flowerZ)) {
					int flowerY = heightmap[flowerX + flowerZ * map->width] + 1;
					uint8_t below = map_get(map, flowerX, flowerY - 1, flowerZ);

					if (map_get(map, flowerX, flowerY, flowerZ) == air && below == grass) {
						map_set(map, flowerX, flowerY, flowerZ, flowerType);
					}
				}
			}
		}
	}

	for (int i = 0; i < numShrooms; i++) {
		uint8_t shroomType = rng_next_boolean(rng) ? brown_mushroom : red_mushroom;

		unsigned int patchX = rng_next2(rng, 0, (int)map->width);
		unsigned int patchY = rng_next2(rng, 1, (int)map->depth);
		unsigned int patchZ = rng_next2(rng, 0, (int)map->height);

		for (int j = 0; j < 20; j++) {
			unsigned int shroomX = patchX;
			unsigned int shroomY = patchY;
			unsigned int shroomZ = patchZ;

			for (int k = 0; k < 5; k++) {
				shroomX += rng_next(rng, 6) - rng_next(rng, 6);
				shroomZ += rng_next(rng, 6) - rng_next(rng, 6);

				if (map_pos_valid(map, shroomX, 0, shroomZ) && shroomY < heightmap[shroomX + shroomZ * map->width] - 1) {
					uint8_t below = map_get(map, shroomX, shroomY - 1, shroomZ);

					if (map_get(map, shroomX, shroomY, shroomZ) == air && below == stone) {
						map_set(map, shroomX, shroomY, shroomZ, shroomType);
					}
				}
			}
		}
	}

	for (int i = 0; i < numTrees; i++) {
		int patchX = rng_next2(rng, 0, (int)map->width);
		int patchZ = rng_next2(rng, 0, (int)map->height);

		for (int j = 0; j < 20; j++) {
			int treeX = patchX;
			int treeZ = patchZ;

			for (int k = 0; k < 20; k++) {
				treeX += rng_next(rng, 6) - rng_next(rng, 6);
				treeZ += rng_next(rng, 6) - rng_next(rng, 6);

				if (map_pos_valid(map, treeX, 0, treeZ) && rng_next_float(rng) <= 0.25f) {
					int treeY = heightmap[treeX + treeZ * map->width] + 1;
					int treeHeight = rng_next2(rng, 1, 3) + 4;

					if (mapgen_space_for_tree(map, treeX, treeY, treeZ, treeHeight)) {
						mapgen_grow_tree(map, rng, treeX, treeY, treeZ, treeHeight);
					}
				}
			}
		}
	}
}
