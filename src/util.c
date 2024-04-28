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

#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "util.h"
#include "config.h"

double get_time_s(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
}

httpheader_t *util_httpheaders_parse(const char *text, size_t *num_headers) {
	size_t n = 0;
	httpheader_t *result = NULL;
	size_t inlen = strlen(text);

	char *key = NULL, *value = NULL;

	char buf[4096];
	size_t bufi = 0;
	bool parsevalue = false;

	for (size_t i = 0; i < inlen; i++) {
		const char c = text[i];
		switch (c) {
			case '\r': {
				continue;
			}

			case '\n': {
				if (!parsevalue) {
					break;
				}

				value = strdup(buf);
				memset(buf, 0, sizeof(buf));
				bufi = 0;
				parsevalue = false;

				size_t idx = n++;
				result = realloc(result, sizeof(*result) * n);
				result[idx].key = key;
				result[idx].value = value;

				break;
			}

			case ':': {
				if (!parsevalue) {
					parsevalue = true;

					key = strdup(buf);
					memset(buf, 0, sizeof(buf));
					bufi = 0;

					if (text[i + 1] == ' ') {
						i++;
					}

					break;
				}

				goto _default;
			}

			default:
			_default: {
				buf[bufi++] = c;

				break;
			}
		}
	}

	*num_headers = n;
	return result;
}

const char *util_httpheaders_get(httpheader_t *headers, size_t num_headers, const char *key) {
	for (size_t i = 0; i < num_headers; i++) {
		if (strcasecmp(headers[i].key, key) == 0) {
			return headers[i].value;
		}
	}

	return NULL;
}

void util_httpheaders_destroy(httpheader_t *list, size_t num_headers) {
	for (size_t i = 0; i < num_headers; i++) {
		free(list[i].key);
		free(list[i].value);
	}
	free(list);
}

void util_print_coloured(const char *msg) {
	const size_t len = strlen(msg);

	for (size_t i = 0; i < len; i++) {
		const char c = msg[i];

		if (c == '&' && i != len - 1) {
			const char next = msg[++i];
			char fmt[32];

			switch (next) {
				case '0': strncpy(fmt, "\033[0;30m", sizeof(fmt)); break;
				case '1': strncpy(fmt, "\033[0;34m", sizeof(fmt)); break;
				case '2': strncpy(fmt, "\033[0;32m", sizeof(fmt)); break;
				case '3': strncpy(fmt, "\033[0;36m", sizeof(fmt)); break;
				case '4': strncpy(fmt, "\033[0;31m", sizeof(fmt)); break;
				case '5': strncpy(fmt, "\033[0;35m", sizeof(fmt)); break;
				case '6': strncpy(fmt, "\033[0;33m", sizeof(fmt)); break;
				case '7': strncpy(fmt, "\033[0;37m", sizeof(fmt)); break;
				case '8': strncpy(fmt, "\033[0;90m", sizeof(fmt)); break;
				case '9': strncpy(fmt, "\033[0;94m", sizeof(fmt)); break;
				case 'a': strncpy(fmt, "\033[0;92m", sizeof(fmt)); break;
				case 'b': strncpy(fmt, "\033[0;96m", sizeof(fmt)); break;
				case 'c': strncpy(fmt, "\033[0;91m", sizeof(fmt)); break;
				case 'd': strncpy(fmt, "\033[0;95m", sizeof(fmt)); break;
				case 'e': strncpy(fmt, "\033[0;93m", sizeof(fmt)); break;
				case 'f': strncpy(fmt, "\033[0;97m", sizeof(fmt)); break;
				default: {
					const textcolour_t *col = config_find_colour(next);
					if (col != NULL) {
						snprintf(fmt, sizeof(fmt), "\033[38;2;%u;%u;%um", col->r, col->g, col->b);
					}
					else {
						snprintf(fmt, sizeof(fmt), "%c%c", c, next);
					}

					break;
				}
			}

			printf("%s", fmt);
		}
		else {
			printf("%c", c);
		}
	}

	printf("\n");
}
