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
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "packet.h"
#include "util.h"

// CPE EnvColors definitions
#define ENV_COLOUR_DEFAULT ((uint32_t)UINT32_MAX)

typedef struct scheduledtick_s {
	size_t x, y, z;
	uint64_t time;
} scheduledtick_t;

typedef enum {
	weather_clear,
	weather_rain,
	weather_snow,
} weathertype_t;

typedef struct map_s {
	char *name;

	size_t width, depth, height;
	uint8_t *blocks;

	bool generating;
	bool modified;

	size_t num_ticks;
	size_t ticks_size;
	scheduledtick_t *ticks;

	rgb_t colours[env_colour_count];
	int weather;
} map_t;

map_t *map_create(const char *name, size_t width, size_t depth, size_t height);
void map_destroy(map_t *map);

void map_set(map_t *map, size_t x, size_t y, size_t z, uint8_t block);
uint8_t map_get(map_t *map, size_t x, size_t y, size_t z);
size_t map_get_top(map_t *map, size_t x, size_t z);
size_t map_get_top_lit(map_t *map, size_t x, size_t z);

void map_set_colour(map_t *map, envcolourtype_t type, rgb_t *rgb);
void map_set_weather(map_t *map, weathertype_t type);

void map_tick(map_t *map);
void map_add_tick(map_t *map, size_t x, size_t y, size_t z, uint64_t num_ticks_until);

void map_save(map_t *map);
map_t *map_load(const char *name);

static inline bool map_pos_valid(map_t *map, size_t x, size_t y, size_t z) {
	return x < map->width && y < map->depth && z < map->height;
}

static inline size_t map_get_block_index(map_t *map, size_t x, size_t y, size_t z) {
	return (y * map->height + z) * map->width + x;
}

static inline void map_index_to_pos(map_t *map, size_t index, size_t *x, size_t *y, size_t *z) {
	*y = index / (map->width * map->depth);
	index -= *y * map->width * map->depth;
	*z = index / map->width;
	*x = index % map->width;
}
