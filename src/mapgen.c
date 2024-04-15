#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "map.h"
#include "rng.h"
#include "perlin.h"
#include "util.h"
#include "blocks.h"

/* used for flood fill */
typedef struct fastintstack_s {
	unsigned int *values;
	unsigned int capacity, size;
} fastintstack_t;

static fastintstack_t *fastintstack_create(unsigned int capacity);
static unsigned int fastintstack_pop(fastintstack_t *);
static void fastintstack_push(fastintstack_t *, unsigned int);
static void fastintstack_destroy(fastintstack_t *);

typedef struct genstate_s {
	rng_t *rng;
	int *heightmap;
} genstate_t;

static void gen_heightmap(map_t *map, genstate_t *state);
static void gen_strata(map_t *map, genstate_t *state);
static void gen_caves(map_t *map, genstate_t *state);
static void gen_ore(map_t *map, genstate_t *state, int8_t block, float abundance);
static void gen_water(map_t *map, genstate_t *state);
static void gen_lava(map_t *map, genstate_t *state);
static void gen_surface(map_t *map, genstate_t *state);
static void gen_plants(map_t *map, genstate_t *state);

static void fill_oblate_spherioid(map_t *map, int centreX, int centreY, int centreZ, double radius, uint8_t block);
static void flood_fill(map_t *map, unsigned int startIndex, uint8_t block);

static bool mapgen_space_for_tree(struct map_s *map, int x, int y, int z, int height);
static void mapgen_grow_tree(struct map_s *map, genstate_t *state, int x, int y, int z, int height);

void mapgen_classic(map_t *map) {
	genstate_t state;
	state.rng = rng_create(0);
	state.heightmap = calloc(map->width * map->height, sizeof(int));

	gen_heightmap(map, &state);
	gen_strata(map, &state);
	gen_caves(map, &state);
	gen_ore(map, &state, gold_ore, 0.5f);
	gen_ore(map, &state, iron_ore, 0.7f);
	gen_ore(map, &state, coal_ore, 0.9f);
	gen_water(map, &state);
	gen_lava(map, &state);
	gen_surface(map, &state);
	gen_plants(map, &state);

	rng_destroy(state.rng);
}

void gen_heightmap(map_t *map, genstate_t *state) {
	combinednoise_t *noise1 = combinednoise_create(octavenoise_create(state->rng, 8), octavenoise_create(state->rng, 8));
	combinednoise_t *noise2 = combinednoise_create(octavenoise_create(state->rng, 8), octavenoise_create(state->rng, 8));
	octavenoise_t *noise3 = octavenoise_create(state->rng, 6);

	state->heightmap = calloc(map->width * map->depth * map->height, sizeof(int));

	for (size_t x = 0; x < map->width; x++)
	for (size_t z = 0; z < map->height; z++) {
		double heightLow = combinednoise_compute(noise1, (double)x * 1.3, (double)z * 1.3) / 6 - 4;
		double heightHigh = combinednoise_compute(noise2, (double)x * 1.3, (double)z * 1.3) / 5 + 6;
		double heightResult;

		if (octavenoise_compute(noise3, (double)x, (double)z) / 8 > 0) {
			heightResult = heightLow;
		} else {
			heightResult = util_max(heightLow, heightHigh);
		}

		heightResult /= 2;

		if (heightResult < 0) {
			heightResult *= 0.8;
		}

		state->heightmap[x + z * map->width] = (int) (heightResult + ((double)map->depth / 2.0));
	}

	combinednoise_destroy(noise1);
	combinednoise_destroy(noise2);
	octavenoise_destroy(noise3);
}

void gen_strata(map_t *map, genstate_t *state) {
	octavenoise_t *noise = octavenoise_create(state->rng, 8);

	for (size_t x = 0; x < map->width; x++)
	for (size_t z = 0; z < map->height; z++) {
		size_t dirtThickness = (int) octavenoise_compute(noise, (double)x, (double)z) / 24 - 4;
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
}

static void gen_caves(map_t *map, genstate_t *state) {
	unsigned int numCaves = (map->width * map->depth * map->height) / 8192;
	
	for (unsigned int i = 0; i < numCaves; i++) {
		double caveX = rng_next2(state->rng, 0, (int)map->width);
		double caveY = rng_next2(state->rng, 0, (int)map->depth);
		double caveZ = rng_next2(state->rng, 0, (int)map->height);

		int caveLength = (int) (rng_next_float(state->rng) * rng_next_float(state->rng) * 200.0f);

		double theta = rng_next_float(state->rng) * M_PI * 2;
		double deltaTheta = 0.0;

		double phi = rng_next_float(state->rng) * M_PI * 2;
		double deltaPhi = 0.0;

		double caveRadius = rng_next_float(state->rng) * rng_next_float(state->rng);

		for (double len = 0; len < (double)caveLength; len += 1.0f) {
			caveX += sin(theta) * cos(phi);
			caveY += cos(theta) * cos(phi);
			caveZ += sin(phi);

			theta = theta + deltaTheta * 0.2f;
			deltaTheta = deltaTheta * 0.9f + rng_next_float(state->rng) - rng_next_float(state->rng);
			phi = phi / 2.0f + deltaPhi / 4.0f;
			deltaPhi = deltaPhi * 0.75f + rng_next_float(state->rng) - rng_next_float(state->rng);

			if (rng_next_float(state->rng) >= 0.25f) {
				int centreX = (int) (caveX + (double)(rng_next(state->rng, 4) - 2) * 0.2);
				int centreY = (int) (caveY + (double)(rng_next(state->rng, 4) - 2) * 0.2);
				int centreZ = (int) (caveZ + (double)(rng_next(state->rng, 4) - 2) * 0.2);

				double radius = ((double)map->height - centreY) / (double)map->height;
				radius = 1.2 + (radius * 3.5 + 1) * caveRadius;
				radius = radius * sin(len * M_PI / caveLength);

				fill_oblate_spherioid(map, centreX, centreY, centreZ, radius, air);
			}
		}
	}
}

void gen_ore(map_t *map, genstate_t *state, int8_t block, float abundance) {
	int numVeins = (int) (((double) (map->width * map->depth * map->height) * abundance) / 16384.0f);

	for (int i = 0; i < numVeins; i++) {
		double veinX = rng_next2(state->rng, 0, (int)map->width);
		double veinY = rng_next2(state->rng, 0, (int)map->depth);
		double veinZ = rng_next2(state->rng, 0, (int)map->height);

		double veinLength = rng_next_float(state->rng) * rng_next_float(state->rng) * 75.0f * abundance;

		double theta = rng_next_float(state->rng) * M_PI * 2;
		double deltaTheta = 0;
		double phi = rng_next_float(state->rng) * M_PI * 2;
		double deltaPhi = 0;

		for (float len = 0; len < veinLength; len += 1.0f) {
			veinX = veinX + sin(theta) * cos(phi);
			veinY = veinY + cos(theta) * cos(phi);
			veinZ = veinZ + cos(theta);

			theta = deltaTheta * 0.2;
			deltaTheta = (deltaTheta * 0.9) + rng_next_float(state->rng) - rng_next_float(state->rng);
			phi = phi / 2.0 + deltaPhi / 4.0;
			deltaPhi = (deltaPhi * 0.9) + rng_next_float(state->rng) - rng_next_float(state->rng);

			double radius = abundance * sin(len * M_PI / veinLength) + 1;

			fill_oblate_spherioid(map, (int) veinX, (int) veinY, (int) veinZ, radius, block);
		}
	}
}

void gen_water(map_t *map, genstate_t *state) {
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
		int x = rng_next2(state->rng, 0, (int)map->width);
		int z = rng_next2(state->rng, 0, (int)map->height);
		int y = waterLevel - rng_next2(state->rng, 0, 2);

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
		bool sandChance = octavenoise_compute(noise1, x, z) > 8;
		bool gravelChance = octavenoise_compute(noise2, x, z) > 12;

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

void gen_plants(map_t *map, genstate_t *state)
{
	int numFlowers = (int)(map->width * map->height) / 3000;
	int numShrooms = (int)(map->width * map->depth * map->height) / 2000;
	int numTrees = (int)(map->width * map->height) / 4000;

	for (int i = 0; i < numFlowers; i++) {
		uint8_t flowerType = rng_next_boolean(state->rng) ? dandelion : rose;

		int patchX = rng_next2(state->rng, 0, (int)map->width);
		int patchZ = rng_next2(state->rng, 0, (int)map->height);

		for (int j = 0; j < 10; j++) {
			int flowerX = patchX;
			int flowerZ = patchZ;

			for (int k = 0; k < 5; k++) {
				flowerX += rng_next(state->rng, 6) - rng_next(state->rng, 6);
				flowerZ += rng_next(state->rng, 6) - rng_next(state->rng, 6);

				if (map_pos_valid(map, flowerX, 0, flowerZ)) {
					int flowerY = state->heightmap[flowerX + flowerZ * map->width] + 1;
					uint8_t below = map_get(map, flowerX, flowerY - 1, flowerZ);

					if (map_get(map, flowerX, flowerY, flowerZ) == air && below == grass) {
						map_set(map, flowerX, flowerY, flowerZ, flowerType);
					}
				}
			}
		}
	}

	for (int i = 0; i < numShrooms; i++) {
		uint8_t shroomType = rng_next_boolean(state->rng) ? brown_mushroom : red_mushroom;

		unsigned int patchX = rng_next2(state->rng, 0, (int)map->width);
		unsigned int patchY = rng_next2(state->rng, 1, (int)map->depth);
		unsigned int patchZ = rng_next2(state->rng, 0, (int)map->height);

		for (int j = 0; j < 20; j++) {
			unsigned int shroomX = patchX;
			unsigned int shroomY = patchY;
			unsigned int shroomZ = patchZ;

			for (int k = 0; k < 5; k++) {
				shroomX += rng_next(state->rng, 6) - rng_next(state->rng, 6);
				shroomZ += rng_next(state->rng, 6) - rng_next(state->rng, 6);

				if (map_pos_valid(map, shroomX, 0, shroomZ) && (int)shroomY < state->heightmap[shroomX + shroomZ * map->width] - 1) {
					uint8_t below = map_get(map, shroomX, shroomY - 1, shroomZ);

					if (map_get(map, shroomX, shroomY, shroomZ) == air && below == stone) {
						map_set(map, shroomX, shroomY, shroomZ, shroomType);
					}
				}
			}
		}
	}

	for (int i = 0; i < numTrees; i++) {
		int patchX = rng_next2(state->rng, 0, (int)map->width);
		int patchZ = rng_next2(state->rng, 0, (int)map->height);

		for (int j = 0; j < 20; j++) {
			int treeX = patchX;
			int treeZ = patchZ;

			for (int k = 0; k < 20; k++) {
				treeX += rng_next(state->rng, 6) - rng_next(state->rng, 6);
				treeZ += rng_next(state->rng, 6) - rng_next(state->rng, 6);

				if (map_pos_valid(map, treeX, 0, treeZ) && rng_next_float(state->rng) <= 0.25f) {
					int treeY = state->heightmap[treeX + treeZ * map->width] + 1;
					int treeHeight = rng_next2(state->rng, 1, 3) + 4;

					if (mapgen_space_for_tree(map, treeX, treeY, treeZ, treeHeight)) {
						mapgen_grow_tree(map, state, treeX, treeY, treeZ, treeHeight);
					}
				}
			}
		}
	}

}

void fill_oblate_spherioid(map_t *map, int centreX, int centreY, int centreZ, double radius, uint8_t block) {
	int xStart = (int)floor(util_max(centreX - radius, 0));
	int xEnd = (int)floor(util_min(centreX + radius, map->width - 1));
	int yStart = (int)floor(util_max(centreY - radius, 0));
	int yEnd = (int)floor(util_min(centreY + radius, map->depth - 1));
	int zStart = (int)floor(util_max(centreZ - radius, 0));
	int zEnd = (int)floor(util_min(centreZ + radius, map->height - 1));

	for (double x = xStart; x < xEnd; x++)
	for (double y = yStart; y < yEnd; y++)
	for (double z = zStart; z < zEnd; z++) {
		double dx = x - centreX;
		double dy = y - centreY;
		double dz = z - centreZ;

		int ix = (int)x;
		int iy = (int)y;
		int iz = (int)z;

		if ((dx * dx + 2 * dy * dy + dz * dz) < (radius * radius) && map_pos_valid(map, ix, iy, iz)) {
			if (map_get(map, ix, iy, iz) == stone) {
				map_set(map, ix, iy, iz, block);
			}
		}
	}
}

void flood_fill(map_t *map, unsigned int startIndex, uint8_t block) {
	unsigned int oneY = map->width * map->height;
	unsigned int blocklen = map->width * map->depth * map->height;

	fastintstack_t *stack = fastintstack_create(4);
	fastintstack_push(stack, startIndex);

	while (stack->size > 0) {
		unsigned int index = fastintstack_pop(stack);

		if (index >= blocklen) continue;

		if (map->blocks[index] != air) continue;
		map->blocks[index] = block;

		unsigned int x = index % map->width;
		unsigned int y = index / oneY;
		unsigned int z = (index / map->width) % map->height;

		if (x > 0) fastintstack_push(stack, index - 1);
		if (x < map->width - 1) fastintstack_push(stack, index + 1);
		if (z > 0) fastintstack_push(stack, index - map->width);
		if (z < map->height - 1) fastintstack_push(stack, index + map->width);
		if (y > 0) fastintstack_push(stack, index - oneY);
	}

	fastintstack_destroy(stack);
}

fastintstack_t *fastintstack_create(unsigned int capacity) {
	fastintstack_t *s = malloc(sizeof(fastintstack_t));
	s->size = 0;
	s->capacity = capacity;
	s->values = calloc(capacity, sizeof(unsigned int));
	return s;
}

unsigned int fastintstack_pop(fastintstack_t *s) {
	return s->values[--s->size];
}

void fastintstack_push(fastintstack_t *s, unsigned int v) {
	if (s->size == s->capacity) {
		unsigned int newcap = s->capacity * 2;
		unsigned int *arr = calloc(newcap, sizeof(unsigned int));
		memcpy(arr, s->values, s->size * sizeof(unsigned int));
		free(s->values);
		s->values = arr;
		s->capacity = newcap;
	}

	s->values[s->size++] = v;
}

void fastintstack_destroy(fastintstack_t *s) {
	free(s->values);
	free(s);
}

bool mapgen_space_for_tree(struct map_s *map, int x, int y, int z, int height) {
	if (!map_pos_valid(map, x, y, z) || !map_pos_valid(map, x, y - 1, z)) {
		return false;
	}

	uint8_t below = map_get(map, x, y - 1, z);
	if (below != dirt && below != grass) {
		return false;
	}

	uint8_t here = map_get(map, x, y, z);
	if (here == sapling) {
		map_set(map, x, y, z, air);
	}

	for (int xx = x - 1; xx <= x + 1; xx++)
		for (int yy = y; yy < y + height; yy++)
			for (int zz = z - 1; zz <= z + 1; zz++) {
				if (!map_pos_valid(map, xx, yy, zz)) {
					return false;
				}

				if (map_get(map, xx, yy, zz) != air) {
					return false;
				}
			}

	int canopyY = y + (height - 4);

	for (int xx = x - 2; xx <= x + 2; xx++)
		for (int yy = canopyY; yy < y + height; yy++)
			for (int zz = z - 2; zz <= z + 2; zz++) {
				if (!map_pos_valid(map, xx, yy, zz)) {
					return false;
				}

				if (map_get(map, xx, yy, zz) != air) {
					return false;
				}
			}

	if (here == sapling) {
		map_set(map, x, y, z, sapling);
	}

	return true;
}

void mapgen_grow_tree(map_t *map, genstate_t *state, int x, int y, int z, int height) {
	map_set(map, x, y, z, wood);

	int max0 = y + height;
	int max1 = max0 - 1;
	int max2 = max0 - 2;
	int max3 = max0 - 3;

	/* bottom */
	for (int xx = -2; xx <= 2; xx++)
		for (int zz = -2; zz <= 2; zz++) {
			int ax = x + xx;
			int az = z + zz;

			if (abs(xx) == 2 && abs(zz) == 2) {
				if (rng_next_boolean(state->rng)) map_set(map, ax, max3, az, leaves);
				if (rng_next_boolean(state->rng)) map_set(map, ax, max2, az, leaves);
			} else {
				map_set(map, ax, max3, az, leaves);
				map_set(map, ax, max2, az, leaves);
			}
		}

	/* top */
	for (int xx = -1; xx <= 1; xx++)
		for (int zz = -1; zz <= 1; zz++) {
			int ax = x + xx;
			int az = z + zz;

			if (xx == 0 || zz == 0) {
				map_set(map, ax, max1, az, leaves);
				map_set(map, ax, max0, az, leaves);
			} else {
				if (rng_next_boolean(state->rng)) map_set(map, ax, max1, az, leaves);
			}
		}

	/* trunk */
	for (int yy = y; yy < max0; yy++) {
		map_set(map, x, yy, z, wood);
	}
}

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
