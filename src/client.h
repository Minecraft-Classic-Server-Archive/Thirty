#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "sockets.h"
#include "cpe.h"

struct buffer_s;

enum {
	mapsend_none,
	mapsend_running,
	mapsend_success,
	mapsend_sent,
	mapsend_failure
};

typedef struct client_s {
	socket_t socket_fd;
	bool connected;
	size_t idx;

	struct buffer_s *in_buffer;
	struct buffer_s *out_buffer;

	int mapsend_state;
	struct buffer_s *mapgz_buffer;

	double last_ping;
	double ping;
	uint16_t ping_key;

	char name[64];

	bool spawned;
	float x, y, z;
	float yaw, pitch;

	size_t num_extensions;
	cpeext_t *extensions;

	int customblocks_support;
} client_t;

void client_init(client_t *client, int fd, size_t idx);
void client_destroy(client_t *client);
void client_tick(client_t *client);
void client_flush(client_t *client);
void client_flush_buffer(client_t *client, struct buffer_s *buffer);
void client_disconnect(client_t *client, const char *msg);

bool client_supports_extension(client_t *client, const char *name, int version);

typedef struct {
	client_t *client;
	uint8_t *data;
	size_t width, height, depth;
} mapsend_t;

void *mapsend_thread_start(void *data);
void *mapsend_fast_thread_start(void *data);
