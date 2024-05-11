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

#include <pthread.h>
#include <zlib.h>
#include <stdlib.h>
#include "buffer.h"
#include "map.h"
#include "client.h"
#include "server.h"
#include "packet.h"
#include "blocks.h"
#include "util.h"
#include "log.h"

void *mapsend_thread_start(void *data) {
	mapsend_t *info = (mapsend_t *)data;
	const uint32_t num_blocks = info->width * info->height * info->depth;

	if (!client_supports_extension(info->client, "CustomBlocks", 1)) {
		for (size_t i = 0; i < num_blocks; i++) {
			info->data[i] = block_get_fallback(info->data[i]);
		}
	}

	int outsize = ((num_blocks + sizeof(uint32_t)) * 1.1) + 12;
	uint8_t *outbuf = malloc(outsize);

	z_stream stream;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;
	stream.avail_in = num_blocks + sizeof(uint32_t);
	stream.next_in = (Bytef *)info->data;
	stream.avail_out = outsize;
	stream.next_out = (Bytef *)outbuf;

	int err = deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
	if (err != Z_OK) {
		info->client->mapsend_state = mapsend_failure;
		log_printf(log_error, "Failed to init zlib stream.");
		goto cleanup;
	}

	while ((deflate(&stream, Z_FINISH)) != Z_STREAM_END);
	deflateEnd(&stream);

	outsize = stream.total_out;

	info->client->mapgz_buffer = buffer_allocate_memory(outsize, false);
	buffer_write(info->client->mapgz_buffer, outbuf, outsize);
	buffer_seek(info->client->mapgz_buffer, 0);

	info->client->mapsend_state = mapsend_success;

cleanup:
	free(outbuf);
	free(info->data);
	free(info);

	pthread_exit(NULL);
	return NULL;
}

#define OUTBUFSIZE 1024

void *mapsend_fast_thread_start(void *data) {
	client_t *client = (client_t *)data;
	const uint32_t num_blocks = server.map->width * server.map->height * server.map->depth;

	buffer_t *blockbuffer = buffer_create_memory(server.map->blocks, num_blocks);
	const size_t inbufsize = util_min(2 * 1024 * 1024, num_blocks);

	uint8_t *inbuf = malloc(inbufsize);
	uint8_t outbuf[OUTBUFSIZE];

	buffer_t *packetbuffer = buffer_allocate_memory(32 * 1024, false);

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	int err = deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
	if (err != Z_OK) {
		client->mapsend_state = mapsend_failure;
		log_printf(log_error, "Failed to init zlib stream.");
		goto cleanup;
	}

	int flush, have;
	do {
		strm.avail_in = buffer_read(blockbuffer, inbuf, inbufsize);
		flush = (buffer_tell(blockbuffer) == buffer_size(blockbuffer)) ? Z_FINISH : Z_SYNC_FLUSH;
		strm.next_in = inbuf;

		do {
			strm.avail_out = OUTBUFSIZE;
			strm.next_out = outbuf;

			err = deflate(&strm, flush);
			if (err == Z_STREAM_ERROR) {
				client->mapsend_state = mapsend_failure;
				log_printf(log_error, "Failed to compress data.");
				goto cleanup;
			}

			have = OUTBUFSIZE - strm.avail_out;

			if (!client->connected) {
				goto cleanup;
			}

			if (buffer_tell(packetbuffer) + 1028 >= buffer_size(packetbuffer)) {
				client_flush_buffer(client, packetbuffer);
				usleep(1000000 / 20);
			}

			buffer_write_uint8(packetbuffer, packet_level_chunk);
			buffer_write_uint16be(packetbuffer, have);
			buffer_write(packetbuffer, outbuf, 1024);
			buffer_write_uint8(packetbuffer, 0);
		} while (strm.avail_out == 0);
	} while (flush != Z_FINISH);

	deflateEnd(&strm);
	client_flush_buffer(client, packetbuffer);

	client->mapsend_state = mapsend_success;

cleanup:
	free(inbuf);
	buffer_destroy(packetbuffer);
	buffer_destroy(blockbuffer);

	pthread_exit(NULL);
	return NULL;
}
