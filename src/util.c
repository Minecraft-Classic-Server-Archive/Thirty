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

extern bool args_disable_colour;

double get_time_s(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
}

bool util_httpheaders_parse(httpheaders_t *result, const char *text) {
	memset(result, 0, sizeof(*result));

	size_t n = 0;
	size_t inlen = strlen(text);

	char *key = NULL, *value = NULL;

	char buf[4096];
	size_t bufi = 0;
	bool parsevalue = false;

	const char *start = "HTTP/1.1 ";
	const size_t startlen = strlen(start);
	// 14 is the length of "HTTP/1.1 XXX\r\n"
	if (inlen >= 14 && memcmp(text, start, startlen) == 0) {
		long value = strtol(text + startlen, NULL, 10);
		result->code = (int)value;
		text = strstr(text, "\r\n") + 2;
	}

	size_t i;
	for (i = 0; i < inlen; i++) {
		const char c = text[i];
		bool end = false;

		switch (c) {
			case '\r': {
				continue;
			}

			case '\n': {
				if (!parsevalue) {
					if (bufi == 0) {
						end = true;
					}

					break;
				}

				value = strdup(buf);
				memset(buf, 0, sizeof(buf));
				bufi = 0;
				parsevalue = false;

				size_t idx = (n++) * 2;
				result->data = realloc(result->data, sizeof(*result->data) * n * 2);
				result->data[idx] = key;
				result->data[idx + 1] = value;

				key = NULL;
				value = NULL;

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
				buf[bufi] = '\0';

				break;
			}
		}

		if (end) {
			break;
		}
	}

	result->num_headers = n;
	result->end = text + i;
	return result;
}

const char *util_httpheaders_get(httpheaders_t *headers, const char *key) {
	for (size_t i = 0; i < headers->num_headers * 2; i += 2) {
		if (strcasecmp(headers->data[i], key) == 0) {
			return headers->data[i + 1];
		}
	}

	return NULL;
}

void util_httpheaders_destroy(httpheaders_t *list) {
	for (size_t i = 0; i < list->num_headers * 2; i++) {
		free(list->data[i]);
	}
	free(list->data);
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
						printf(fmt, sizeof(fmt), "%c%c", c, next);
					}

					break;
				}
			}

			if (!args_disable_colour) {
				printf("%s", fmt);
			}
		}
		else {
			printf("%c", c);
		}
	}

	if (!args_disable_colour) {
		printf("\033[0m");
	}

	printf("\n");
}
