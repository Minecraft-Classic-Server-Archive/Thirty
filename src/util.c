#include <time.h>
#include "util.h"

double get_time_s(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
}
