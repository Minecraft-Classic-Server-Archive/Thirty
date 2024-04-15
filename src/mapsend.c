#include <pthread.h>
#include <zlib.h>
#include <stdlib.h>
#include "buffer.h"
#include "map.h"
#include "client.h"
#include "server.h"
#include "packet.h"
#include "blocks.h"

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
		fprintf(stderr, "Failed to init zlib stream.");
		goto cleanup;
	}

	while ((deflate(&stream, Z_FINISH)) != Z_STREAM_END);
	deflateEnd(&stream);

	outsize = stream.total_out;

	info->client->mapgz_buffer = buffer_allocate_memory(outsize);
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

#define BUFSIZE 1024

void *mapsend_fast_thread_start(void *data) {
	client_t *client = (client_t *)data;
	const uint32_t num_blocks = server.map->width * server.map->height * server.map->depth;

	buffer_t *blockbuffer = buffer_create_memory(server.map->blocks, num_blocks);

	uint8_t inbuf[BUFSIZE];
	uint8_t outbuf[BUFSIZE];

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	int err = deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
	if (err != Z_OK) {
		client->mapsend_state = mapsend_failure;
		fprintf(stderr, "Failed to init zlib stream.");
		goto cleanup;
	}

	int flush, have;
	do {
		strm.avail_in = buffer_read(blockbuffer, inbuf, BUFSIZE);
		flush = (buffer_tell(blockbuffer) == buffer_size(blockbuffer)) ? Z_FINISH : Z_SYNC_FLUSH;
		strm.next_in = inbuf;

		do {
			strm.avail_out = BUFSIZE;
			strm.next_out = outbuf;

			err = deflate(&strm, flush);
			if (err == Z_STREAM_ERROR) {
				client->mapsend_state = mapsend_failure;
				fprintf(stderr, "Failed to compress data.");
				goto cleanup;
			}

			have = BUFSIZE - strm.avail_out;

			if (!client->connected) {
				goto cleanup;
			}

			if (buffer_tell(client->out_buffer) + 1028 >= buffer_size(client->out_buffer)) {
				client_flush(client);
				usleep(1000000 / 20);
			}

			buffer_write_uint8(client->out_buffer, packet_level_chunk);
			buffer_write_uint16be(client->out_buffer, have);
			buffer_write(client->out_buffer, outbuf, 1024);
			buffer_write_uint8(client->out_buffer, 0);
		} while (strm.avail_out == 0);
	} while (flush != Z_FINISH);

	deflateEnd(&strm);
	client_flush(client);

	client->mapsend_state = mapsend_success;

cleanup:
	buffer_destroy(blockbuffer);

	pthread_exit(NULL);
	return NULL;
}
