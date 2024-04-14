#include <stdio.h>
#include "server.h"

int main(int argc, char *argv[]) {
	printf("hi\n");

	server_init();
	server_shutdown();

	return 0;
}
