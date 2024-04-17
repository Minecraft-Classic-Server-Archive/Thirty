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

#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include "mapgen.h"
#include "map.h"
#include "rng.h"
#include "perlin.h"
#include "blocks.h"

// Experimental world generator I originally wrote for "real" Minecraft.
// Works better with large (infinite...) worlds. But even then the terrain ends up pretty rough.

typedef struct {
	rng_t *rng;
	unsigned int *heightmap;
	combinednoise_t *basenoise1, *basenoise2;
	octavenoise_t *basenoise3;
	combinednoise_t *overhangheight;
	octavenoise_t *overhangthreshold;
	octavenoise_t *overhangamplify;
	octavenoise_t *overhangsample;
	octavenoise_t *dirtthickness;
	octavenoise_t *beachnoise;
} genstate_t;

static void gen_noise(map_t *map, genstate_t *state);
static void gen_surface(map_t *map, genstate_t *state);

void mapgen_seantest(map_t *map) {
	genstate_t state;
	state.rng = rng_create((int)time(NULL));
	state.heightmap = calloc(map->width * map->height, sizeof(unsigned int));
	state.basenoise1 = combinednoise_create(octavenoise_create(state.rng, 8), octavenoise_create(state.rng, 8));
	state.basenoise2 = combinednoise_create(octavenoise_create(state.rng, 8), octavenoise_create(state.rng, 8));
	state.basenoise3 = octavenoise_create(state.rng, 16);
	state.overhangheight = combinednoise_create(octavenoise_create(state.rng, 8), octavenoise_create(state.rng, 8));
	state.overhangsample = octavenoise_create(state.rng, 8);
	state.overhangthreshold = octavenoise_create(state.rng, 4);
	state.overhangamplify = octavenoise_create(state.rng, 8);
	state.dirtthickness = octavenoise_create(state.rng, 8);
	state.beachnoise = octavenoise_create(state.rng, 6);

	gen_noise(map, &state);
	gen_surface(map, &state);
	gen_caves(map, state.rng);
	gen_ore(map, state.rng, gold_ore, 0.5f);
	gen_ore(map, state.rng, iron_ore, 0.7f);
	gen_ore(map, state.rng, coal_ore, 0.9f);
	gen_plants(map, state.rng, state.heightmap);

	octavenoise_destroy(state.beachnoise);
	octavenoise_destroy(state.dirtthickness);
	octavenoise_destroy(state.overhangamplify);
	octavenoise_destroy(state.overhangthreshold);
	octavenoise_destroy(state.overhangsample);
	combinednoise_destroy(state.overhangheight);
	octavenoise_destroy(state.basenoise3);
	combinednoise_destroy(state.basenoise2);
	combinednoise_destroy(state.basenoise1);
	free(state.heightmap);
	rng_destroy(state.rng);
}

void gen_noise(map_t *map, genstate_t *state) {
	int waterLevel = ((int)map->depth / 2) - 1;

	for (size_t x = 0; x < map->width; x++)
	for (size_t z = 0; z < map->height; z++) {
		double h1 = combinednoise_compute2d(state->basenoise1, (double)x / 2.3, (double)z / 2.3) / 3.0 - 2.0;
		double h2 = combinednoise_compute2d(state->basenoise2, (double)x / 1.3, (double)z / 1.3) / 2.0 - 6.0;
		double hr;
		if (octavenoise_compute2d(state->basenoise3, (double)x, (double)z) / 8.0 > 0) {
			hr = h1;
		}
		else {
			hr = h2;
		}

		unsigned int hm = 0;

		double bh = waterLevel + hr;
		double th = waterLevel + (combinednoise_compute2d(state->overhangheight, (double)x / 4.1, (double)z / 4.1) / 2.5) + (octavenoise_compute2d(state->overhangamplify, (double)x / 3.7, (double)z / 3.7) / 2.0);
		double tt = (octavenoise_compute2d(state->overhangthreshold, (double)x / 4.1, (double)z / 4.1) / 3.0) - (octavenoise_compute2d(state->overhangamplify, (double)x, (double)z) / 3.0);

		for (size_t y = 0; y < map->depth; y++) {
			double ts = octavenoise_compute3d(state->overhangsample, (double)x / 3.3, (double)y, (double)z / 3.3) / 3.0;
			if ((double)y <= bh || ((double)y <= th && ts > tt)) {
				map_set(map, x, y, z, stone);
				if (y > hm) hm = y;
			}
			else if ((double)y <= waterLevel) {
				map_set(map, x, y, z, water);
				if (y > hm) hm = y;
			}
		}

		state->heightmap[x + z * map->width] = hm;
	}
}

void gen_surface(map_t *map, genstate_t *state) {
	int waterLevel = ((int)map->depth / 2) - 1;

	for (size_t x = 0; x < map->width; x++)
	for (size_t z = 0; z < map->height; z++) {
		int d = 0;
		bool underwater = false;
		int bh = rng_next(state->rng, 3);

		for (ssize_t y = (ssize_t)map->depth - 1; y >= 0; y--) {
			uint8_t t = map_get(map, x, y, z);
			if (t == air) {
				d = 0;
				underwater = false;
				continue;
			}

			if (t == water || t == water_still) {
				underwater = true;
				continue;
			}

			int dirt_thickness = (int) octavenoise_compute2d(state->dirtthickness, (double)x, (double)z) / 24 + 4;

			uint8_t newtype;
			if (d < 3 && octavenoise_compute2d(state->beachnoise, (double)x / 1.3, (double)z / 1.3) / 6.0 > 1.0 && y >= waterLevel - 1 && y <= waterLevel + 1) {
				newtype = sand;
			}
			else if (d == 0 && !underwater) {
				newtype = grass;
			}
			else if (d < dirt_thickness) {
				if ((underwater && octavenoise_compute3d(state->dirtthickness, (double)x, (double)y, (double)z) > 9.0)) {
					newtype = sand;
				}
				else {
					newtype = dirt;
				}
			}
			else if (y <= bh) {
				newtype = bedrock;
			}
			else {
				newtype = stone;
			}

			map_set(map, x, y, z, newtype);
			d++;
		}
	}
}
