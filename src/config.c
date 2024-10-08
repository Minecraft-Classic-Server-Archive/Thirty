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
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include "config.h"
#include "log.h"

typedef void (*configcallback_t)(const char *section, const char *key, const char *value);

enum parsestate_e {
	parse_section,
	parse_key,
	parse_value,
	parse_comment,
};

static void config_parse(const char *filename, configcallback_t callback);
static long parse_int(const char *ptr, bool *ok, int radix);
static void cfg_callback(const char *section, const char *key, const char *value);

config_t config;

static const char *config_default_paths[] = {
	"/etc/thirty.ini",
	"/usr/local/etc/thirty.ini",
	"thirty.ini",

	NULL
};

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
	log_printf(log_info, "Loading configuration from '%s'.", filename);

	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		log_printf(log_error, "Failed to open config file '%s' for reading.", filename);
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
					log_printf(log_error, "Section name is too long and will be truncated: '%s'", sectionbuf);
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
					log_printf(log_error, "Key name is too long and will be truncated: '%s'", keybuf);
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
					log_printf(log_error, "Value is too long and will be truncated: '%s'", valuebuf);
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

long parse_int(const char *ptr, bool *ok, int radix) {
	char *end;
	long value = strtol(ptr, &end, radix);
	if (end == ptr || (errno == ERANGE && (value == LONG_MIN || value == LONG_MAX))) {
		*ok = false;
		return 0;
	}
	*ok = true;
	return value;
}

unsigned long parse_uint(const char *ptr, bool *ok, int radix) {
	char *end;
	unsigned long value = strtoul(ptr, &end, radix);
	if (end == ptr || (errno == ERANGE && value == ULONG_MAX)) {
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
			long port = parse_int(value, &ok, 10);
			if (!ok) {
				log_printf(log_error, "Failed to parse 'port' as unsigned integer");
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
			long max = parse_int(value, &ok, 10);
			if (!ok) {
				log_printf(log_error, "Failed to parse 'max_players' as unsigned integer");
			} else {
				config.server.max_players = (unsigned int) max;
			}
		}
		else if (strcmp(key, "whitelist") == 0) {
			config.server.enable_whitelist = strcmp(value, "true") == 0;
		}
		else if (strcmp(key, "web_proxies") == 0) {
			for (size_t i = 0; i < config.server.num_proxies; i++) {
				free(config.server.allowed_web_proxies[i]);
			}
			
			free(config.server.allowed_web_proxies);
			config.server.allowed_web_proxies = NULL;
			config.server.num_proxies = 0;
			
			char buf[64];
			size_t bufp = 0;
			const size_t vallen = strlen(value);
			for (size_t i = 0; i <= vallen; i++) {
				const char c = value[i];
				if (c == ' ' || i == vallen) {
					if (bufp == 0) {
						continue;
					}

					trim(buf);
					size_t idx = config.server.num_proxies++;
					config.server.allowed_web_proxies = realloc(config.server.allowed_web_proxies, sizeof(char *) * config.server.num_proxies);
					config.server.allowed_web_proxies[idx] = strdup(buf);
					bufp = 0;
				}
				else if (bufp <= sizeof(buf) - 1) {
					buf[bufp++] = c;
					buf[bufp] = '\0';
				}
			}
		}
		else if (strcmp(key, "enable_old_clients") == 0) {
			config.server.enable_old_clients = strcmp(value, "true") == 0;
		}
	}

	else if (strcmp(section, "map") == 0) {
		if (strcmp(key, "name") == 0) {
			config.map.name = strdup(value);
		}
		else if (strcmp(key, "width") == 0) {
			long size = parse_int(value, &ok, 10);
			if (!ok) {
				log_printf(log_error, "Failed to parse 'width' as unsigned integer");
			} else {
				config.map.width = size;
			}
		}
		else if (strcmp(key, "depth") == 0) {
			long size = parse_int(value, &ok, 10);
			if (!ok) {
				log_printf(log_error, "Failed to parse 'depth' as unsigned integer");
			} else {
				config.map.depth = size;
			}
		}
		else if (strcmp(key, "height") == 0) {
			long size = parse_int(value, &ok, 10);
			if (!ok) {
				log_printf(log_error, "Failed to parse 'height' as unsigned integer");
			} else {
				config.map.height = size;
			}
		}
		else if (strcmp(key, "generator") == 0) {
			config.map.generator = strdup(value);
		}
		else if (strcmp(key, "seed") == 0) {
			long seed = parse_int(value, &ok, 10);
			if (!ok) {
				log_printf(log_error, "Failed to parse 'seed' as unsigned integer");
			} else {
				config.map.seed = seed;
				config.map.random_seed = false;
			}
		}
		else if (strcmp(key, "image_path") == 0) {
			config.map.image_path = strdup(value);
		}
		else if (strcmp(key, "image_interval") == 0) {
			long size = parse_int(value, &ok, 10);
			if (!ok) {
				log_printf(log_error, "Failed to parse 'image_interval' as unsigned integer");
			} else {
				config.map.image_interval = size;
			}
		}
	}

	else if (strcmp(section, "colours") == 0) {
		if (strlen(key) != 1) {
			log_printf(log_error, "Colour name must be exactly 1 character.");
			return;
		}
		if (strlen(value) != 8) {
			log_printf(log_error, "Colour value must be an 8-digit RGBA hex number.");
			return;
		}

		uint32_t colour = (uint32_t) parse_uint(value, &ok, 16);
		if (!ok) {
			log_printf(log_error, "Colour value failed to parse, it must be an 8-digit RGBA hex number.");
			return;
		}

		size_t idx = config.num_colours++;
		config.colours = realloc(config.colours, sizeof(*config.colours) * config.num_colours);
		config.colours[idx].code = key[0];
		config.colours[idx].r = (colour >> 24U) & 0xFFU;
		config.colours[idx].g = (colour >> 16U) & 0xFFU;
		config.colours[idx].b = (colour >> 8U) & 0xFFU;
		config.colours[idx].a = colour & 0xFFU;
	}

	else if (strcmp(section, "debug") == 0) {
		if (strcmp(key, "fixed_salt") == 0) {
			strncpy(config.debug.fixed_salt, value, sizeof(config.debug.fixed_salt) - 1);
		}
		else if (strcmp(key, "disable_save") == 0) {
			config.debug.disable_save = strcmp(value, "true") == 0;
		}
	}
}

void config_init(const char *config_path) {
	memset(&config, 0, sizeof(config));

	config.map.random_seed = true;

	config.server.allowed_web_proxies = malloc(sizeof(char *));
	config.server.allowed_web_proxies[0] = strdup("34.223.5.250");
	config.server.num_proxies = 1;

	if (config_path != NULL) {
		config_parse(config_path, cfg_callback);
	}
	else {
		for (size_t i = 0; config_default_paths[i] != NULL; i++) {
			if (access(config_default_paths[i], R_OK) != 0) {
				continue;
			}

			config_parse(config_default_paths[i], cfg_callback);
			break;
		}
	}

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

	if (config.map.image_path == NULL) {
		config.map.image_path = strdup("");
	}
}

void config_destroy(void) {
	for (size_t i = 0; i < config.server.num_proxies; i++) {
		free(config.server.allowed_web_proxies[i]);
	}

	free(config.server.allowed_web_proxies);
	config.server.allowed_web_proxies = NULL;
	config.server.num_proxies = 0;
	
	free(config.server.name);
	free(config.server.motd);
	free(config.map.image_path);
	free(config.map.name);
	free(config.map.generator);

	memset(&config, 0, sizeof(config));
}

const textcolour_t *config_find_colour(const char code) {
	for (size_t i = 0; i < config.num_colours; i++) {
		if (config.colours[i].code == code) {
			return &config.colours[i];
		}
	}

	return NULL;
}
