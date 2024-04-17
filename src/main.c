#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include "server.h"
#include "sockets.h"
#include "blocks.h"

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
		server_tick();

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
