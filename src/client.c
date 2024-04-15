#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include "client.h"
#include "server.h"
#include "buffer.h"
#include "map.h"
#include "packet.h"
#include "util.h"

static void client_receive(client_t *client);
static void client_start_mapsave(client_t *client);

void client_init(client_t *client, int fd, size_t idx) {
	memset(client, 0, sizeof(*client));

	client->socket_fd = fd;
	client->idx = idx;
	client->in_buffer = buffer_allocate_memory(8192);
	client->out_buffer = buffer_allocate_memory(8192);
	client->mapsend_state = mapsend_none;
	client->mapgz_buffer = NULL;
}

void client_destroy(client_t *client) {
	buffer_destroy(client->in_buffer);
	buffer_destroy(client->out_buffer);
}

void client_tick(client_t *client) {
	client_receive(client);

	if (client->mapsend_state != mapsend_none) {
		if (client->mapsend_state == mapsend_success) {
			for (int i = 0; i < 4; i++) {
				if (buffer_size(client->mapgz_buffer) == buffer_tell(client->mapgz_buffer)) {
					buffer_destroy(client->mapgz_buffer);
					client->mapsend_state = mapsend_sent;
					client->mapgz_buffer = NULL;

					buffer_write_uint8(client->out_buffer, packet_level_finish);
					buffer_write_uint16be(client->out_buffer, server.map->width);
					buffer_write_uint16be(client->out_buffer, server.map->height);
					buffer_write_uint16be(client->out_buffer, server.map->depth);
					client_flush(client);
					break;
				}
				else {
					uint8_t data[1024];
					memset(data, 0, 1024);

					size_t len = buffer_read(client->mapgz_buffer, data, 1024);

					buffer_write_uint8(client->out_buffer, packet_level_chunk);
					buffer_write_uint16be(client->out_buffer, (uint16_t)len);
					buffer_write(client->out_buffer, data, 1024);
					buffer_write_uint8(client->out_buffer, 0);

					client_flush(client);
				}
			}
		}
		else if (client->mapsend_state == mapsend_failure) {
			buffer_write_uint8(client->out_buffer, packet_player_disconnect);
			buffer_write_mcstr(client->out_buffer, "Failed to send map data");
			client_flush(client);
		}
	}
}

void client_receive(client_t *client) {
	buffer_seek(client->in_buffer, 0);
	int r = recv(client->socket_fd, client->in_buffer->mem.data, client->in_buffer->mem.size, 0);
	if (r == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return;
		}

		perror("recv");
		return;
	}

	if (r == 0) {
		return;
	}

	uint8_t packet_id;
	buffer_read_uint8(client->in_buffer, &packet_id);

	printf("packet id: 0x%02x\n", packet_id);

	if (packet_id == 0x00) {
		uint8_t protocol_version;
		char username[65];
		char key[65];
		uint8_t unused;

		buffer_read_uint8(client->in_buffer, &protocol_version);
		buffer_read_mcstr(client->in_buffer, username);
		buffer_read_mcstr(client->in_buffer, key);
		buffer_read_uint8(client->in_buffer, &unused);

		printf("protool: %d\n", protocol_version);
		printf("username: %s\n", username);
		printf("key: %s\n", key);
		printf("unused: %d\n", unused);

		buffer_write_uint8(client->out_buffer, packet_ident);
		buffer_write_uint8(client->out_buffer, 0x07);
		buffer_write_mcstr(client->out_buffer, "hostname");
		buffer_write_mcstr(client->out_buffer, "motd");
		buffer_write_uint8(client->out_buffer, 0x00);
		client_flush(client);

		client_start_mapsave(client);

		buffer_write_uint8(client->out_buffer, packet_level_init);
		client_flush(client);
	}
}

void client_flush(client_t *client) {
	int sendflags = 0;
#ifdef __linux__
	sendflags |= MSG_NOSIGNAL;
#endif

	int r = send(client->socket_fd, client->out_buffer->mem.data, client->out_buffer->mem.offset, sendflags);
	buffer_seek(client->out_buffer, 0);

	if (r == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return;
		}

		perror("send");
	}

	printf("sending %d\n", r);
}

void client_start_mapsave(client_t *client) {
	mapsend_t *data = malloc(sizeof(*data));
	data->client = client;
	data->width = server.map->width;
	data->height = server.map->height;
	data->depth = server.map->depth;

	const uint32_t num_blocks = data->width * data->height * data->depth;
	const size_t bufsize = sizeof(uint32_t) + num_blocks;
	data->data = malloc(bufsize);

	buffer_t *buf = buffer_create_memory(data->data, bufsize);
	buffer_write_uint32be(buf, num_blocks);
	buffer_write(buf, server.map->blocks, num_blocks);
	buffer_destroy(buf);

	pthread_attr_t attr;
	pthread_attr_init(&attr);

	pthread_t thread;
	pthread_create(&thread, &attr, mapsend_thread_start, data);
}