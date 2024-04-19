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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "map.h"
#include "nbt.h"
#include "buffer.h"

void map_save(map_t *map) {
	char filename[256];
	snprintf(filename, sizeof(filename), "%s.cw", map->name);

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

	nbt_add_tag(root, format_version);
	nbt_add_tag(root, name_tag);
	nbt_add_tag(root, uuid_tag);
	nbt_add_tag(root, x_size);
	nbt_add_tag(root, y_size);
	nbt_add_tag(root, z_size);
	nbt_add_tag(root, block_array);
	nbt_add_tag(root, metadata);

	// Now write it out and gzip it.

	buffer_t *outbuf = buffer_allocate_memory(num_blocks + (1 * 1024 * 1024));
	nbt_write(root, outbuf);
	buffer_seek(outbuf, 0);

	size_t readbufsize = 16 * 1024 * 1024;
	uint8_t *readbuf = malloc(16 * 1024 * 1024);
	uint8_t gzbuf[2048];
	FILE *fp = fopen(filename, "wb");

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	int err = deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
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
			strm.avail_out = 2048;
			strm.next_out = gzbuf;

			err = deflate(&strm, flush);
			if (err == Z_STREAM_ERROR) {
				fprintf(stderr, "Failed to compress data.");
				goto cleanup;
			}

			have = 2048 - strm.avail_out;

			fwrite(gzbuf, 1, have, fp);
		} while (strm.avail_out == 0);
	} while (flush != Z_FINISH);

cleanup:
	deflateEnd(&strm);

	fclose(fp);
	free(readbuf);
	buffer_destroy(outbuf);

	nbt_destroy(root, true);
}
