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
#include "blocks.h"

void mapgen_flat(map_t *map) {
	unsigned int waterLevel = (map->depth / 2) - 1;

	for (size_t x = 0; x < map->width; x++)
	for (size_t y = 0; y <= waterLevel; y++)
	for (size_t z = 0; z < map->height; z++) {
		uint8_t block;

		if (y == waterLevel) {
			block = grass;
		}
		else if (y >= waterLevel - 4) {
			block = dirt;
		}
		else {
			block = stone;
		}

		map_set(map, x, y, z, block);
	}
}
