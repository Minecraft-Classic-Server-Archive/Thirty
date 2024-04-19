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

#include <stdio.h>
#include <inttypes.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include "server.h"
#include "sockets.h"
#include "config.h"

#ifndef _WIN32
#include <sys/types.h>
#include <netdb.h>
#endif

static void *heartbeat_main(void *data) {
	(void)data;

	char url[2048];
	char response[2048];
	snprintf(url, sizeof(url),
			 "GET /server/heartbeat/?port=%" PRIu16 "&web=True&max=%d&public=%s&version=7&salt=%s&users=%zu&software=%s&name=%s HTTP/1.1\r\n"
			 "Host: www.classicube.net\r\n"
			 "User-Agent: Thirty\r\n"
			 "\r\n",
			 25565,
			 128,
			 "False",
			 server.salt,
			 server.num_clients,
			 "Thirty",
			 "test"
	);

	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;

	int err = getaddrinfo("www.classicube.net", "80", &hints, &result);
	if (err != 0) {
		fprintf(stderr, "getaddrinfo error: %d\n", err);
		return NULL;
	}

	socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		fprintf(stderr, "socket error: %d\n", socket_error());
		goto cleanup;
	}

	err = connect(sock, result->ai_addr, result->ai_addrlen);
	if (err == SOCKET_ERROR) {
		fprintf(stderr, "connect error: %d\n", socket_error());
		goto cleanup;
	}

	err = send(sock, url, strlen(url), 0);
	if (err == SOCKET_ERROR) {
		fprintf(stderr, "send error: %d\n", socket_error());
		goto cleanup;
	}

	err = recv(sock, response, sizeof(response), 0);
	if (err == SOCKET_ERROR) {
		fprintf(stderr, "recv error: %d\n", socket_error());
		goto cleanup;
	}

	const char *expected = "HTTP/1.1 200 OK\r\n";
	if (strncmp(response, expected, strlen(expected)) != 0) {
		fprintf(stderr, "Heartbeat failed:\n%s\n", response);
	}
	else printf("%s\n", response);

cleanup:
	closesocket(sock);
	freeaddrinfo(result);

	return NULL;
}

void server_heartbeat(void) {
	if (config.server.offline) {
		return;
	}

	pthread_attr_t attr;
	pthread_attr_init(&attr);

	pthread_t thread;
	pthread_create(&thread, &attr, heartbeat_main, NULL);
}
