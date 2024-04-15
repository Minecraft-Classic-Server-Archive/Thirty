#pragma once

struct buffer_s;

typedef struct client_s {
	int socket_fd;
	size_t idx;

	struct buffer_s *in_buffer;
	struct buffer_s *out_buffer;
} client_t;

void client_init(client_t *client, int fd, size_t idx);
void client_destroy(client_t *client);
void client_tick(client_t *client);
void client_flush(client_t *client);