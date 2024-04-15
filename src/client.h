#pragma once
#include <stdint.h>
#include <stdbool.h>

struct buffer_s;

enum {
	mapsend_none,
	mapsend_running,
	mapsend_success,
	mapsend_sent,
	mapsend_failure
};

typedef struct client_s {
	int socket_fd;
	bool connected;
	size_t idx;

	struct buffer_s *in_buffer;
	struct buffer_s *out_buffer;

	int mapsend_state;
	struct buffer_s *mapgz_buffer;

	uint64_t last_ping;

	char name[64];

	float x, y, z;
	float yaw, pitch;
} client_t;

void client_init(client_t *client, int fd, size_t idx);
void client_destroy(client_t *client);
void client_tick(client_t *client);
void client_flush(client_t *client);
void client_disconnect(client_t *client, const char *msg);

typedef struct {
	client_t *client;
	uint8_t *data;
	size_t width, height, depth;
} mapsend_t;

void *mapsend_thread_start(void *data);
