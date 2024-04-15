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
