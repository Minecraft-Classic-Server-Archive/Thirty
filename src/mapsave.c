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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "map.h"
#include "nbt.h"
#include "buffer.h"
#include "server.h"

void map_save(map_t *map) {
	char filename[256];
	snprintf(filename, sizeof(filename), "%s.cw", map->name);

	printf("Saving map to '%s'\n", filename);

	tag_t *root = nbt_create_compound("ClassicWorld");
	uint8_t uuid[16];

	const size_t num_blocks = map->width * map->depth * map->height;

	tag_t *format_version = nbt_create("FormatVersion"); nbt_set_int8(format_version, 1);
	tag_t *name_tag = nbt_create_string("Name", map->name);
	tag_t *uuid_tag = nbt_copy_bytearray("UUID", uuid, 16);
	tag_t *x_size = nbt_create("X"); nbt_set_int16(x_size, (int16_t)map->width);
	tag_t *y_size = nbt_create("Y"); nbt_set_int16(y_size, (int16_t)map->depth);
	tag_t *z_size = nbt_create("Z"); nbt_set_int16(z_size, (int16_t)map->height);
	tag_t *block_array = nbt_copy_bytearray("BlockArray", map->blocks, num_blocks);
	tag_t *metadata = nbt_create_compound("Metadata");
	tag_t *software_data = nbt_create_compound("Thirty");

	tag_t *scheduled_ticks = nbt_create_compound("ScheduledTicks");
	int32_t *tick_indices_data = calloc(map->num_ticks, sizeof(int32_t));
	tag_t *tick_indices = nbt_create_intarray("Indices", tick_indices_data, (int32_t)map->num_ticks);
	int32_t *tick_times_data = calloc(map->num_ticks, sizeof(int32_t));
	tag_t *tick_times = nbt_create_intarray("Times", tick_times_data, (int32_t)map->num_ticks);

	for (size_t i = 0; i < map->num_ticks; i++) {
		const scheduledtick_t *tick = &map->ticks[i];

		tick_indices_data[i] = (int32_t)map_get_block_index(map, tick->x, tick->y, tick->z);
		tick_times_data[i] = (int32_t)(tick->time - server.tick);
	}

	nbt_add_tag(scheduled_ticks, tick_indices);
	nbt_add_tag(scheduled_ticks, tick_times);

	nbt_add_tag(software_data, scheduled_ticks);

	nbt_add_tag(metadata, software_data);

	nbt_add_tag(root, format_version);
	nbt_add_tag(root, name_tag);
	nbt_add_tag(root, uuid_tag);
	nbt_add_tag(root, x_size);
	nbt_add_tag(root, y_size);
	nbt_add_tag(root, z_size);
	nbt_add_tag(root, block_array);
	nbt_add_tag(root, metadata);

	// Now write it out and gzip it.

	buffer_t *outbuf = buffer_allocate_memory(num_blocks, true);
	nbt_write(root, outbuf);
	buffer_seek(outbuf, 0);

	size_t readbufsize = 2 * 1024 * 1024;
	uint8_t *readbuf = malloc(readbufsize);
	size_t gzbufsize = 2 * 1024 * 1024;
	uint8_t *gzbuf = malloc(gzbufsize);
	FILE *fp = fopen(filename, "wb");

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	int err = deflateInit2(&strm, Z_BEST_SPEED, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
	if (err != Z_OK) {
		fprintf(stderr, "Failed to init zlib stream.");
		goto cleanup;
	}

	unsigned int have;
	int flush;
	do {
		strm.avail_in = buffer_read(outbuf, readbuf, readbufsize);
		flush = (buffer_tell(outbuf) == buffer_size(outbuf)) ? Z_FINISH : Z_NO_FLUSH;
		strm.next_in = readbuf;

		do {
			strm.avail_out = gzbufsize;
			strm.next_out = gzbuf;

			err = deflate(&strm, flush);
			if (err == Z_STREAM_ERROR) {
				fprintf(stderr, "Failed to compress data.");
				goto cleanup;
			}

			have = gzbufsize - strm.avail_out;

			fwrite(gzbuf, 1, have, fp);
		} while (strm.avail_out == 0);
	} while (flush != Z_FINISH);

	printf("Saved!\n");

cleanup:
	deflateEnd(&strm);

	fclose(fp);
	free(gzbuf);
	free(readbuf);
	buffer_destroy(outbuf);

	nbt_destroy(root, true);
}

map_t *map_load(const char *name) {
	char filename[256];
	snprintf(filename, sizeof(filename), "%s.cw", name);

	map_t *map = NULL;
	uint8_t *inbuf = NULL, *outbuf = NULL;
	tag_t *root = NULL;
	buffer_t *nbtbuf = NULL;

	FILE *fp = fopen(filename, "rb");
	if (fp == NULL) {
		goto cleanup;
	}

	const size_t inbufsize = 2 * 1024 * 1024;
	inbuf = malloc(inbufsize);
	const size_t outbufsize = 2 * 1024 * 1024;
	outbuf = malloc(outbufsize);

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	int ret = inflateInit2(&strm, 15 | 16);
	if (ret != Z_OK) {
		fprintf(stderr, "Failed to init zlib stream.");
		goto cleanup;
	}

	nbtbuf = buffer_allocate_memory(0, true);

	do {
		strm.avail_in = fread(inbuf, 1, inbufsize, fp);
		if (ferror(fp)) {
			inflateEnd(&strm);
			return NULL;
		}

		if (strm.avail_in == 0) {
			break;
		}

		strm.next_in = inbuf;

		do {
			strm.avail_out = outbufsize;
			strm.next_out = outbuf;

			ret = inflate(&strm, Z_NO_FLUSH);

			unsigned have = outbufsize - strm.avail_out;
			buffer_write(nbtbuf, outbuf, have);
		} while (strm.avail_out == 0);
	} while (ret != Z_STREAM_END);

	fclose(fp); fp = NULL;
	free(outbuf); outbuf = NULL;
	free(inbuf); inbuf = NULL;

	buffer_seek(nbtbuf, 0);

	root = nbt_read(nbtbuf, true);
	buffer_destroy(nbtbuf); nbtbuf = NULL;

	if (root == NULL) {
		fprintf(stderr, "Not an NBT file: '%s'\n", filename);
	}

	tag_t *format_version = nbt_get_tag(root, "FormatVersion");
	if (format_version == NULL || format_version->type != tag_byte) {
		fprintf(stderr, "Map format invalid: '%s'", filename);
		goto cleanup;
	}

	if (format_version->b != 1) {
		fprintf(stderr, "Unsupported ClassicWorld version: '%s'", filename);
		goto cleanup;
	}

	size_t w, d, h;
	tag_t *xsize = nbt_get_tag(root, "X");
	tag_t *ysize = nbt_get_tag(root, "Y");
	tag_t *zsize = nbt_get_tag(root, "Z");
	tag_t *blocks = nbt_get_tag(root, "BlockArray");

	if (xsize == NULL || xsize->type != tag_short || ysize == NULL || ysize->type != tag_short || zsize == NULL || zsize->type != tag_short || blocks == NULL || blocks->type != tag_byte_array) {
		fprintf(stderr, "World file is invalid: '%s'\n", filename);
		goto cleanup;
	}

	w = (size_t)xsize->s;
	d = (size_t)ysize->s;
	h = (size_t)zsize->s;
	map = map_create(name, w, d, h);
	memcpy(map->blocks, blocks->pb, w * d * h);

	{
		tag_t *metadata = nbt_get_tag(root, "Metadata");
		if (metadata != NULL && metadata->type == tag_compound) {
			tag_t *software_data = nbt_get_tag(metadata, "Thirty");
			if (software_data != NULL && metadata->type == tag_compound) {
				tag_t *scheduled_ticks = nbt_get_tag(software_data, "ScheduledTicks");
				if (scheduled_ticks != NULL && metadata->type == tag_compound) {
					tag_t *indices = nbt_get_tag(scheduled_ticks, "Indices");
					tag_t *times = nbt_get_tag(scheduled_ticks, "Times");

					if (indices != NULL && times != NULL && indices->type == tag_int_array && times->type == tag_int_array && indices->array_size == times->array_size) {
						for (int32_t i = 0; i < indices->array_size; i++) {
							size_t index = (size_t)indices->pi[i];
							int32_t time = times->pi[i];
							size_t x, y, z;
							map_index_to_pos(map, index, &x, &y, &z);
							map_add_tick(map, x, y, z, time);
						}
					}
				}
			}
		}
	}

cleanup:
	nbt_destroy(root, true);
	buffer_destroy(nbtbuf);
	free(outbuf);
	free(inbuf);
	if (fp != NULL) fclose(fp);

	return map;
}
