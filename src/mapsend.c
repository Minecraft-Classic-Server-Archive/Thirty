#include <pthread.h>
#include <zlib.h>
#include <stdlib.h>
#include "buffer.h"
#include "map.h"
#include "client.h"

void *mapsend_thread_start(void *data) {
	mapsend_t *info = (mapsend_t *)data;
	const uint32_t num_blocks = info->width * info->height * info->depth;

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

	}

	while ((deflate(&stream, Z_FINISH)) != Z_STREAM_END);
	deflateEnd(&stream);

	outsize = stream.total_out;

	info->client->mapgz_buffer = buffer_allocate_memory(outsize);
	buffer_write(info->client->mapgz_buffer, outbuf, outsize);
	buffer_seek(info->client->mapgz_buffer, 0);

	info->client->mapsend_state = mapsend_success;

	free(outbuf);
	free(info->data);
	free(info);

	pthread_exit(NULL);
}