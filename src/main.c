#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include "server.h"
#include "sockets.h"
#include "blocks.h"
#include "util.h"

static void signal_handler(int signum);

static bool running = true;

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;
	
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

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

#ifdef _WIN32
	WSACleanup();
#endif

	return 0;
}

void signal_handler(int signum) {
	printf("Received signal %d, will exit.\n", signum);
	running = false;
}
