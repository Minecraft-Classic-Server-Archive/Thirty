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

#pragma once
#include <stdbool.h>
#include <stdint.h>

/* used for flood fill */
typedef struct fastintstack_s {
	unsigned int *values;
	unsigned int capacity, size;
} fastintstack_t;

typedef struct map_s map_t;
typedef struct rng_s rng_t;

void map_generate(map_t *map, const char *generator_name);

void mapgen_classic(map_t *map);
void mapgen_debug(map_t *map);
void mapgen_flat(map_t *map);
void mapgen_random(map_t *map);
void mapgen_seantest(map_t *map);

void gen_caves(map_t *map, rng_t *rng, bool filter_stone, uint8_t block);
void gen_ore(map_t *map, rng_t *rng, int8_t block, float abundance);
void gen_plants(map_t *map, rng_t *rng, unsigned int *heightmap);
void gen_water(map_t *map, rng_t *rng);

bool mapgen_space_for_tree(struct map_s *map, int x, int y, int z, int height);
void mapgen_grow_tree(struct map_s *map, rng_t *rng, int x, int y, int z, int height);

void fill_oblate_spherioid(map_t *map, int centreX, int centreY, int centreZ, double radius, bool filter_stone, uint8_t block);
void flood_fill(map_t *map, unsigned int startIndex, uint8_t block);

fastintstack_t *fastintstack_create(unsigned int capacity);
unsigned int fastintstack_pop(fastintstack_t *);
void fastintstack_push(fastintstack_t *, unsigned int);
void fastintstack_destroy(fastintstack_t *);
