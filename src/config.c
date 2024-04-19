// classicserver, a ClassiCube (Minecraft Classic) server
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
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>
#include <limits.h>
#include "config.h"

typedef void (*configcallback_t)(const char *section, const char *key, const char *value);

enum parsestate_e {
	parse_section,
	parse_key,
	parse_value,
	parse_comment,
};

static void config_parse(const char *filename, configcallback_t callback);
static long parse_int(const char *ptr, bool *ok);
static void cfg_callback(const char *section, const char *key, const char *value);

config_t config;

static void trim(char *p) {
	size_t len = strlen(p);
	size_t start = 0;
	for (; start < len; start++) {
		if (!isspace(p[start])) {
			break;
		}
	}

	memmove(p, p + start, len - start);
	p[len - start] = 0;
	len = strlen(p);

	for (ssize_t i = (ssize_t)len - 1; i >= 0; i--) {
		if (isspace(p[i])) {
			p[i] = 0;
		}
		else {
			break;
		}
	}
}

void config_parse(const char *filename, configcallback_t callback) {
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		fprintf(stderr, "Failed to open config file '%s' for reading.\n", filename);
		return;
	}

	enum parsestate_e state = parse_key;
	char sectionbuf[128];
	size_t sectionp = 0;
	char keybuf[128];
	size_t keyp = 0;
	char valuebuf[1024];
	size_t valuep = 0;

	while (!feof(fp)) {
		char c;
		fread(&c, sizeof(char), 1, fp);

		switch (state) {
			case parse_section: {
				if (c == ']') {
					state = parse_key;
					continue;
				}

				if (sectionp >= sizeof(sectionbuf) - 1) {
					fprintf(stderr, "Section name is too long and will be truncated: '%s'\n", sectionbuf);
					break;
				}

				sectionbuf[sectionp++] = c;
				sectionbuf[sectionp] = 0;

				break;
			}

			case parse_key: {
				if (keyp == 0) {
					if (c == '[') {
						state = parse_section;
						sectionp = 0;
						continue;
					}
					else if (isspace(c)) {
						continue;
					}
					else if (c == ';' || c == '#') {
						state = parse_comment;
						continue;
					}
				}
				else {
					if (c == '=') {
						state = parse_value;
						valuep = 0;
						continue;
					}
				}

				if (keyp >= sizeof(keybuf) - 1) {
					fprintf(stderr, "Key name is too long and will be truncated: '%s'\n", keybuf);
					break;
				}

				keybuf[keyp++] = c;
				keybuf[keyp] = 0;

				break;
			}

			case parse_value: {
				if (c == '\n') {
					trim(sectionbuf);
					trim(keybuf);
					trim(valuebuf);
					callback(sectionbuf, keybuf, valuebuf);
					state = parse_key;
					keyp = 0;
					break;
				}
				else if (valuep == 0) {
					if (isspace(c)) {
						continue;
					}
				}

				if (valuep >= sizeof(valuebuf) - 1) {
					fprintf(stderr, "Value is too long and will be truncated: '%s'\n", valuebuf);
					break;
				}

				valuebuf[valuep++] = c;
				valuebuf[valuep] = 0;

				break;
			}

			case parse_comment: {
				if (c == '\n') {
					state = parse_key;
				}

				break;
			}
		}
	}

	fclose(fp);
}

long parse_int(const char *ptr, bool *ok) {
	char *end;
	long value = strtol(ptr, &end, 10);
	if (end == ptr || (errno == ERANGE && (value == LONG_MIN || value == LONG_MAX))) {
		*ok = false;
		return 0;
	}
	*ok = true;
	return value;
}

void cfg_callback(const char *section, const char *key, const char *value) {
	bool ok;

	if (strcmp(section, "server") == 0) {
		if (strcmp(key, "name") == 0) {
			config.server.name = strdup(value);
		}
		else if (strcmp(key, "motd") == 0) {
			config.server.motd = strdup(value);
		}
		else if (strcmp(key, "port") == 0) {
			long port = parse_int(value, &ok);
			if (!ok) {
				fprintf(stderr, "Failed to parse 'port' as unsigned integer\n");
			} else {
				config.server.port = (uint16_t) port;
			}
		}
		else if (strcmp(key, "public") == 0) {
			config.server.public = strcmp(value, "true") == 0;
		}
		else if (strcmp(key, "offline") == 0) {
			config.server.offline = strcmp(value, "true") == 0;
		}
		else if (strcmp(key, "max_players") == 0) {
			long max = parse_int(value, &ok);
			if (!ok) {
				fprintf(stderr, "Failed to parse 'max_players' as unsigned integer\n");
			} else {
				config.server.max_players = (unsigned int) max;
			}
		}
	}

	else if (strcmp(section, "map") == 0) {
		if (strcmp(key, "name") == 0) {
			config.map.name = strdup(value);
		}
		else if (strcmp(key, "width") == 0) {
			long size = parse_int(value, &ok);
			if (!ok) {
				fprintf(stderr, "Failed to parse 'width' as unsigned integer\n");
			} else {
				config.map.width = size;
			}
		}
		else if (strcmp(key, "depth") == 0) {
			long size = parse_int(value, &ok);
			if (!ok) {
				fprintf(stderr, "Failed to parse 'depth' as unsigned integer\n");
			} else {
				config.map.depth = size;
			}
		}
		else if (strcmp(key, "height") == 0) {
			long size = parse_int(value, &ok);
			if (!ok) {
				fprintf(stderr, "Failed to parse 'height' as unsigned integer\n");
			} else {
				config.map.height = size;
			}
		}
		else if (strcmp(key, "generator") == 0) {
			config.map.generator = strdup(value);
		}
	}
}

void config_init(void) {
	memset(&config, 0, sizeof(config));

	config_parse("settings.ini", cfg_callback);

	if (config.server.name == NULL) {
		config.server.name = strdup("Unnamed server");
	}

	if (config.server.motd == NULL) {
		config.server.motd = strdup("The server owner needs to set a MotD in settings.ini.");
	}

	if (config.server.port == 0) {
		config.server.port = 25565;
	}

	if (config.server.max_players == 0) {
		config.server.max_players = 8;
	}

	if (config.map.name == NULL) {
		config.map.name = strdup("world");
	}

	if (config.map.width == 0) {
		config.map.width = 64;
	}

	if (config.map.depth == 0) {
		config.map.depth = 64;
	}

	if (config.map.height == 0) {
		config.map.height = 64;
	}

	if (config.map.generator == NULL) {
		config.map.generator = strdup("classic");
	}
}

void config_destroy(void) {
	free(config.server.name);
	free(config.server.motd);
	free(config.map.name);
	free(config.map.generator);

	memset(&config, 0, sizeof(config));
}
