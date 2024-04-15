#include <stdlib.h>
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

double improvednoise_compute(improvednoise_t *noise, double x, double y) {
	uint8_t *p = noise->p;

	int xFloor = x >= 0 ? (int) x : (int) x - 1;
	int yFloor = y >= 0 ? (int) y : (int) y - 1;
	int X = xFloor & 0xFF, Y = yFloor & 0xFF;
	x -= xFloor;
	y -= yFloor;

	double u = x * x * x * (x * (x * 6 - 15) + 10); // Fade(x)
	double v = y * y * y * (y * (y * 6 - 15) + 10); // Fade(y)
	int A = p[X] + Y, B = p[X + 1] + Y;

	// Normally, calculating Grad involves a function call. However, we can directly pack this table
	// (since each value indicates either -1, 0 1) into a set of bit flags. This way we avoid needing
	// to call another function that performs branching
	const int xFlags = 0x46552222, yFlags = 0x2222550A;

	int hash = (p[p[A]] & 0xF) << 1;
	double g22 = (((xFlags >> hash) & 3) - 1) * x + (((yFlags >> hash) & 3) - 1) * y; // Grad(p[p[A], x, y)
	hash = (p[p[B]] & 0xF) << 1;
	double g12 = (((xFlags >> hash) & 3) - 1) * (x - 1) +
				 (((yFlags >> hash) & 3) - 1) * y; // Grad(p[p[B], x - 1, y)
	double c1 = g22 + u * (g12 - g22);

	hash = (p[p[A + 1]] & 0xF) << 1;
	double g21 = (((xFlags >> hash) & 3) - 1) * x +
				 (((yFlags >> hash) & 3) - 1) * (y - 1); // Grad(p[p[A + 1], x, y - 1)
	hash = (p[p[B + 1]] & 0xF) << 1;
	double g11 = (((xFlags >> hash) & 3) - 1) * (x - 1) +
				 (((yFlags >> hash) & 3) - 1) * (y - 1); // Grad(p[p[B + 1], x - 1, y - 1)
	double c2 = g21 + u * (g11 - g21);

	return c1 + v * (c2 - c1);
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

double octavenoise_compute(octavenoise_t *noise, double x, double y) {
	double amplitude = 1, frequency = 1;
	double sum = 0;

	for (int i = 0; i < noise->num_octaves; i++) {
		sum += improvednoise_compute(noise->octaves[i], x * frequency, y * frequency) * amplitude;
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

double combinednoise_compute(combinednoise_t *noise, double x, double y) {
	double offset = octavenoise_compute(noise->n2, x, y);
	return octavenoise_compute(noise->n1, x + offset, y);
}

void combinednoise_destroy(combinednoise_t *noise) {
	octavenoise_destroy(noise->n1);
	octavenoise_destroy(noise->n2);
	free(noise);
}
