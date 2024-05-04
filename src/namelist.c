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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "namelist.h"

static void namelist_parse(namelist_t *list);

namelist_t *namelist_create(const char *filename) {
	namelist_t *list = malloc(sizeof(*list));
	if (list == NULL) {
		return NULL;
	}

	memset(list, 0, sizeof(*list));
	list->filename = strdup(filename);

	namelist_parse(list);

	return list;
}

void namelist_parse(namelist_t *list) {
	FILE *fp = fopen(list->filename, "r");
	if (fp == NULL) {
		fprintf(stderr, "Failed to open '%s' for reading: %s\n", list->filename, strerror(errno));
		return;
	}

	char namebuf[128];
	size_t namep = 0;
	size_t spaceskip = 0;
	bool comment = false;

	while (!feof(fp)) {
		char c;
		fread(&c, sizeof(char), 1, fp);

		switch (c) {
			case '\n': {
				if (namep != 0 && !namelist_contains(list, namebuf)) {
					size_t idx = list->num_names++;
					list->names = realloc(list->names, list->num_names * sizeof(*list->names));
					list->names[idx] = strdup(namebuf);
				}

				namep = 0;
				namebuf[namep] = '\0';

				comment = false;
				spaceskip = 0;

				break;
			}

			case '#': {
				if (namep == 0) {
					comment = true;
				}

				break;
			}

			case ' ': {
				spaceskip++;
				break;
			}

			default: {
				if (comment) {
					break;
				}

				if (namep + spaceskip > sizeof(namebuf) - 1) {
					break;
				}

				if (namep > 0) {
					for (size_t i = 0; i < spaceskip; i++) {
						namebuf[namep++] = ' ';
						namebuf[namep] = '\0';
					}
				}

				spaceskip = 0;

				namebuf[namep++] = c;
				namebuf[namep] = '\0';

				break;
			}
		}
	}

	fclose(fp);
}

void namelist_destroy(namelist_t *list) {
	for (size_t i = 0; i < list->num_names; i++) {
		free(list->names[i]);
	}

	free(list->names);
	free(list->filename);
	free(list);
}

bool namelist_contains(namelist_t *list, const char *name) {
	for (size_t i = 0; i < list->num_names; i++) {
		if (strcasecmp(list->names[i], name) == 0) {
			return true;
		}
	}

	return false;
}
