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
#include <stdint.h>
#include <stdbool.h>

typedef struct {
	char code;
	uint8_t r, g, b, a;
} textcolour_t;

typedef struct {
	struct {
		char *name;
		char *motd;
		uint16_t port;
		bool public;
		bool offline;
		unsigned max_players;
		bool enable_whitelist;
		bool enable_old_clients;

		char **allowed_web_proxies;
		size_t num_proxies;
	} server;

	struct {
		char *name;
		unsigned width, depth, height;
		char *generator;
		bool random_seed;
		int seed;
		char *image_path;
		int image_interval;
	} map;

	struct {
		char fixed_salt[17];
		bool disable_save;
	} debug;

	textcolour_t *colours;
	size_t num_colours;
} config_t;

void config_init(const char *config_path);
void config_destroy(void);

const textcolour_t *config_find_colour(char code);

extern config_t config;
