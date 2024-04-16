#pragma once
#include <stdbool.h>
#include <stdint.h>

/* used for flood fill */
typedef struct fastintstack_s {
	unsigned int *values;
	unsigned int capacity, size;
} fastintstack_t;

typedef struct map_s map_t;
typedef struct rng_s rng_t;

void mapgen_classic(map_t *map);
void mapgen_debug(map_t *map);
void mapgen_random(map_t *map);

bool mapgen_space_for_tree(struct map_s *map, int x, int y, int z, int height);
void mapgen_grow_tree(struct map_s *map, rng_t *rng, int x, int y, int z, int height);

void fill_oblate_spherioid(map_t *map, int centreX, int centreY, int centreZ, double radius, uint8_t block);
void flood_fill(map_t *map, unsigned int startIndex, uint8_t block);

fastintstack_t *fastintstack_create(unsigned int capacity);
unsigned int fastintstack_pop(fastintstack_t *);
void fastintstack_push(fastintstack_t *, unsigned int);
void fastintstack_destroy(fastintstack_t *);
