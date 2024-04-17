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

#pragma once

#include <stdbool.h>

typedef struct rng_s {
	long long seed;
	long long value;
	long long mask;
} rng_t;

rng_t *rng_create(int seed);
int rng_next(rng_t *rng, int n);
int rng_next2(rng_t *rng, int min, int max);
float rng_next_float(rng_t *rng);
bool rng_next_boolean(rng_t *rng);
void rng_destroy(rng_t *rng);
