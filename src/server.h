#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
	int socket_fd;
	uint16_t port;
} server_t;

bool server_init();
void server_shutdown();

extern server_t server;