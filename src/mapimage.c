#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "mapimage.h"
#include "stb_image_write.h"
#include "map.h"
#include "blocks.h"
#include "endian.h"

typedef struct {
	uint8_t *blocks;
	size_t width, height, depth;
	char *path;
} imagethread_t;

static inline uint8_t dim(uint8_t value, double factor) {
	return (uint8_t)((double)value * factor);
}

void *map_save_image(void *userdata) {
	imagethread_t *data = (imagethread_t *)userdata;

	uint32_t *pixels = calloc(data->width * data->height, sizeof(uint32_t));

	for (size_t x = 0; x < data->width; x++)
	for (size_t z = 0; z < data->height; z++) {
		for (ssize_t y = data->depth - 1; y >= 0; y--) {
			uint8_t tile = data->blocks[(y * data->height + z) * data->width + x];
			if (blockinfo[tile].solid || blockinfo[tile].liquid) {
				double factor = 0.5 + ((double)y / (double)data->depth) / 2.0;
				uint8_t b = dim(blockinfo[tile].colour & 0xFF, factor);
				uint8_t g = dim((blockinfo[tile].colour >> 8) & 0xFF, factor);
				uint8_t r = dim((blockinfo[tile].colour >> 16) & 0xFF, factor);
				pixels[x + z * data->width] = r | (g << 8) | (b << 16) | (0xFF << 24);
				break;
			}
		}
	}

	stbi_write_png(data->path, data->width, data->height, 4, pixels, data->width * sizeof(uint32_t));

	free(pixels);

	free(data->path);
	free(data->blocks);
	free(data);
	return NULL;
}

void map_save_image_threaded(map_t *map, const char *path) {
	if (path == NULL || path[0] == '\0') {
		return;
	}

	imagethread_t *data = malloc(sizeof(*data));
	data->width = map->width;
	data->depth = map->depth;
	data->height = map->height;
	data->blocks = malloc(data->width * data->height * data->depth);
	data->path = strdup(path);

	memcpy(data->blocks, map->blocks, data->width * data->height * data->depth);

	pthread_attr_t attr;
	pthread_attr_init(&attr);

	pthread_t thread;
	pthread_create(&thread, &attr, map_save_image, data);
}