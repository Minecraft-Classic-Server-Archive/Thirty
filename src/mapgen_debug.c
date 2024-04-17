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

#include "mapgen.h"
#include "map.h"
#include "rng.h"
#include "blocks.h"

void mapgen_debug(map_t *map) {
	for (size_t x = 0; x < map->width; x++)
	for (size_t z = 0; z < map->height; z++) {
		map_set(map, x, 0, z, bedrock);
		map_set(map, x, 1, z, stone);
	}

	for (size_t x = 0; x < num_blocks; x++)
	for (size_t z = 0; z < map->height; z++) {
		map_set(map, x, 2, z, x);
	}

	for (size_t x = map->width / 4 * 3; x < map->width; x++)
	for (size_t z = 0; z < map->height; z++) {
		if (z < map->height / 2 - 1) {
			map_set(map, x, 1, z, water);
		}
		else if (z > map->height / 2) {
			map_set(map, x, 1, z, lava);
		}
	}
}

void mapgen_random(map_t *map) {
	rng_t *rng = rng_create(0);

	for (size_t i = 0; i < map->width * map->depth * map->height; i++) {
		map->blocks[i] = rng_next(rng, num_blocks);
	}

	rng_destroy(rng);
}
