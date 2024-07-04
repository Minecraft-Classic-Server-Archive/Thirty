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
#include <stdarg.h>
#include <pthread.h>
#include <time.h>
#include "log.h"

#include "util.h"

static FILE *log_fp = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *level_prefixes[] = {
	"INFO",
	"ERR "
};

void log_init(void) {
	log_fp = fopen("server.log", "a");

	char timestamp[32];
	{
		time_t now;
		time(&now);
		struct tm *info = localtime(&now);
		strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", info);
	}

	fprintf(log_fp, "--- Log opened at %s ---\n", timestamp);
}

void log_shutdown(void) {
	fprintf(log_fp, "\n");
	fclose(log_fp);
}

void log_printf(enum loglevel_e level, const char *fmt, ...) {
	pthread_mutex_lock(&log_mutex);

	FILE *outbuf = level >= log_error ? stderr : stdout;

	char timestamp[32];
	{
		time_t now;
		time(&now);
		struct tm *info = localtime(&now);
		strftime(timestamp, sizeof(timestamp), "%H:%M:%S", info);
	}

	char buffer[2048];
	{
		va_list args;
		va_start(args, fmt);
		vsnprintf(buffer, sizeof(buffer), fmt, args);
		va_end(args);
	}

	fprintf(outbuf, "[%s %s] ", level_prefixes[level], timestamp);
	util_print_coloured(outbuf, buffer);

	fprintf(log_fp, "[%s %s] ", level_prefixes[level], timestamp);
	util_print_strip_colours(log_fp, buffer);
	fflush(log_fp);

	pthread_mutex_unlock(&log_mutex);
}
