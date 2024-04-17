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
#include "rng.h"

static void rng_set_seed(rng_t *, int);

rng_t *rng_create(int seed) {
	rng_t *r = malloc(sizeof(rng_t));
	r->value = 0x5DEECE66DL;
	r->mask = (1LL << 48) - 1;
	r->seed = 0;

	rng_set_seed(r, seed);

	return r;
}

void rng_set_seed(rng_t * rng, int seed) {
	rng->seed = (seed ^ rng->value) & rng->mask;
}

int rng_next(rng_t *rng, int n) {
	if ((n & -n) == n) {
		rng->seed = (rng->seed * rng->value + 0xBLL) & rng->mask;
		long long raw = (long long)((unsigned long long)rng->seed >> (48 - 31));
		return (int)((n * raw) >> 31);
	}

	int bits, val;
	do {
		rng->seed = (rng->seed * rng->value + 0xBL) & rng->mask;
		bits = (int)((unsigned long long)rng->seed >> (48 - 31));
		val = bits % n;
	} while (bits - val + (n - 1) < 0);

	return val;
}

int rng_next2(rng_t *rng, int min, int max) {
	return min + rng_next(rng, max - min);
}

float rng_next_float(rng_t *rng) {
	rng->seed = (rng->seed * rng->value + 0xBL) & rng->mask;
	int raw = (int)((unsigned long long)rng->seed >> (48 - 24));
	return (float)raw / ((float)(1 << 24));
}

bool rng_next_boolean(rng_t *rng) {
	return rng_next_float(rng) > 0.5f;
}

void rng_destroy(rng_t *rng) {
	free(rng);
}
