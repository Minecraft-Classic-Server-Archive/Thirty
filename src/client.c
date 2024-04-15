#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include "client.h"
#include "buffer.h"

void client_init(client_t *client, int fd, size_t idx) {
	memset(client, 0, sizeof(*client));

	client->socket_fd = fd;
	client->idx = idx;
	client->in_buffer = buffer_allocate_memory(8192);
	client->out_buffer = buffer_allocate_memory(8192);
}

void client_destroy(client_t *client) {
	buffer_destroy(client->in_buffer);
	buffer_destroy(client->out_buffer);
}

void client_tick(client_t *client) {
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

		buffer_write_uint8(client->out_buffer, 0x00);
		buffer_write_uint8(client->out_buffer, 0x07);
		buffer_write_mcstr(client->out_buffer, "hostname");
		buffer_write_mcstr(client->out_buffer, "motd");
		buffer_write_uint8(client->out_buffer, 0x00);
		client_flush(client);

		buffer_write_uint8(client->out_buffer, 0x02);
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