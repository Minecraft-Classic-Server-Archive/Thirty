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
#include <string.h>
#include <stdio.h>
#include "mapgen.h"
#include "map.h"
#include "rng.h"
#include "perlin.h"
#include "util.h"
#include "blocks.h"

void map_generate(map_t *map, const char *generator_name) {
	map->generating = true;

	if (strcmp(generator_name, "classic") == 0) {
		mapgen_classic(map);
	}
	else if (strcmp(generator_name, "seantest") == 0) {
		mapgen_seantest(map);
	}
	else if (strcmp(generator_name, "flat") == 0) {
		mapgen_flat(map);
	}
	else if (strcmp(generator_name, "debug") == 0) {
		mapgen_debug(map);
	}
	else if (strcmp(generator_name, "random") == 0) {
		mapgen_random(map);
	}
	else if (strcmp(generator_name, "growtest") == 0) {
		mapgen_growtest(map);
	}
	else {
		fprintf(stderr, "Invalid generator name '%s', map will be empty\n", generator_name);
	}

	map->generating = false;
}

void fill_oblate_spherioid(map_t *map, int centreX, int centreY, int centreZ, double radius, bool filter_stone, uint8_t block) {
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
			if ((!filter_stone || map_get(map, ix, iy, iz) == stone) && map_get(map, ix, iy, iz) != air) {
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

		if (map->blocks[index] != air && map->blocks[index] != magenta_wool) continue;
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

	for (int xx = x - 1; xx <= x + 1; xx++)
		for (int yy = y; yy < y + height; yy++)
			for (int zz = z - 1; zz <= z + 1; zz++) {
				if (!map_pos_valid(map, xx, yy, zz)) {
					return false;
				}

				if (blockinfo[map_get(map, xx, yy, zz)].solid) {
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

				if (blockinfo[map_get(map, xx, yy, zz)].solid) {
					return false;
				}
			}

	return true;
}

void mapgen_grow_tree(map_t *map, rng_t *rng, int x, int y, int z, int height) {
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
				if (rng_next_boolean(rng)) map_set(map, ax, max3, az, leaves);
				if (rng_next_boolean(rng)) map_set(map, ax, max2, az, leaves);
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
				if (rng_next_boolean(rng)) map_set(map, ax, max1, az, leaves);
			}
		}

	/* trunk */
	for (int yy = y; yy < max0; yy++) {
		map_set(map, x, yy, z, wood);
	}
}
