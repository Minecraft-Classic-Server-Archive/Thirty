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
#include "perlin.h"
#include "rng.h"

improvednoise_t *improvednoise_create(rng_t *rng) {
	improvednoise_t *n = malloc(sizeof(improvednoise_t));
	n->num_p = 512;
	n->p = calloc(n->num_p, sizeof(int8_t));

	for (int i = 0; i < 256; i++) {
		n->p[i] = (int8_t) i;
	}

	for (int i = 0; i < 256; i++) {
		int j = rng_next(rng, 256);
		int8_t temp = n->p[i];
		n->p[i] = n->p[j];
		n->p[j] = temp;
	}

	for (int i = 0; i < 256; i++) {
		n->p[i + 256] = n->p[i];
	}

	return n;
}

static inline double fade(double t) {
	return t * t * t * (t * (t * 6 - 15) + 10);
}

static inline double lerp(double t, double a, double b) {
	return a + t * (b - a);
}

static inline double grad(int hash, double x, double y, double z) {
	int h = hash & 15;
	double u = h < 8 ? x : y, v = h < 4 ? y : h == 12 || h == 14 ? x : z;
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}


double improvednoise_compute3d(improvednoise_t *noise, double x, double y, double z) {
	uint8_t *p = noise->p;

	int X = (int) floor(x) & 255, Y = (int) floor(y) & 255, Z = (int) floor(z) & 255;
	x -= floor(x);
	y -= floor(y);
	z -= floor(z);
	double u = fade(x), v = fade(y), w = fade(z);
	int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z, B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;

	return lerp(w,
				lerp(v, lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z)),
					 lerp(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))),
				lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1), grad(p[BA + 1], x - 1, y, z - 1)),
					 lerp(u, grad(p[AB + 1], x, y - 1, z - 1), grad(p[BB + 1], x - 1, y - 1, z - 1))));
}

double improvednoise_compute2d(improvednoise_t *noise, double x, double y) {
	return improvednoise_compute3d(noise, x, y, 0.0);
}

void improvednoise_destroy(improvednoise_t *noise) {
	free(noise->p);
	free(noise);
}


octavenoise_t *octavenoise_create(rng_t *rng, int numoctaves) {
	octavenoise_t *n = malloc(sizeof(octavenoise_t));
	n->num_octaves = numoctaves;
	n->octaves = calloc(numoctaves, sizeof(*n->octaves));

	for (int i = 0; i < numoctaves; i++) {
		n->octaves[i] = improvednoise_create(rng);
	}

	return n;
}

double octavenoise_compute3d(octavenoise_t *noise, double x, double y, double z) {
	double amplitude = 1, frequency = 1;
	double sum = 0;

	for (int i = 0; i < noise->num_octaves; i++) {
		sum += improvednoise_compute3d(noise->octaves[i], x * frequency, y * frequency, z * frequency) * amplitude;
		amplitude *= 2.0;
		frequency *= 0.5;
	}

	return sum;
}

double octavenoise_compute2d(octavenoise_t *noise, double x, double y) {
	double amplitude = 1, frequency = 1;
	double sum = 0;

	for (int i = 0; i < noise->num_octaves; i++) {
		sum += improvednoise_compute2d(noise->octaves[i], x * frequency, y * frequency) * amplitude;
		amplitude *= 2.0;
		frequency *= 0.5;
	}

	return sum;
}

void octavenoise_destroy(octavenoise_t *noise) {
	for (int i = 0; i < noise->num_octaves; i++) {
		improvednoise_destroy(noise->octaves[i]);
	}

	free(noise->octaves);
	free(noise);
}


combinednoise_t *combinednoise_create(octavenoise_t *n1, octavenoise_t *n2) {
	combinednoise_t *n = malloc(sizeof(combinednoise_t));
	n->n1 = n1;
	n->n2 = n2;
	return n;
}

double combinednoise_compute3d(combinednoise_t *noise, double x, double y, double z) {
	double offset = octavenoise_compute3d(noise->n2, x, y, z);
	return octavenoise_compute3d(noise->n1, x + offset, y, z);
}

double combinednoise_compute2d(combinednoise_t *noise, double x, double y) {
	double offset = octavenoise_compute2d(noise->n2, x, y);
	return octavenoise_compute2d(noise->n1, x + offset, y);
}

void combinednoise_destroy(combinednoise_t *noise) {
	octavenoise_destroy(noise->n1);
	octavenoise_destroy(noise->n2);
	free(noise);
}
