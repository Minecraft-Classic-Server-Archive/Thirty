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

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <getopt.h>
#include "server.h"
#include "sockets.h"
#include "blocks.h"
#include "util.h"
#include "config.h"

static void signal_handler(int signum);

static bool running = true;

bool args_disable_colour = false;

int main(int argc, char *argv[]) {
	const char *config_file = NULL;
	int opt;
	while ((opt = getopt(argc, argv, "Cc:")) != -1) {
		switch (opt) {
			case 'C': {
				args_disable_colour = true;
				break;
			}

			case 'c': {
				config_file = optarg;
				break;
			}

			default: {
				printf("Usage: %s [-C] [-c config]\n", argv[0]);
				return 0;
			}
		}
	}

	config_init(config_file);
	blocks_init();

#ifdef _WIN32
	{
		WSADATA wsadata;
		WSAStartup(MAKEWORD(2, 2), &wsadata);
	}
#endif

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	server_init();

	printf("Ready!\n");

	while (running) {
		double start = get_time_s();
		server_tick();
		double end = get_time_s();
		if (end - start > 1.0 / 20.0) {
			printf("Server lagged: Tick %" PRIu64 " took too long (%f ms)\n", server.tick - 1, (end - start) * 1000.0);
		}

		usleep(1000000 / 20);
	}

	server_shutdown();
	config_destroy();

#ifdef _WIN32
	WSACleanup();
#endif

	return 0;
}

void signal_handler(int signum) {
	printf("Received signal %d, will exit.\n", signum);
	running = false;
}
