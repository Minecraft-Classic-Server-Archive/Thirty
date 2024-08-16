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

#include <string.h>
#include <stdlib.h>
#include "commands.h"
#include "client.h"
#include "log.h"
#include "version.h"

typedef void (*commandfunc_t)(int argc, char **argv, client_t *client);

typedef struct commanddef_s {
	const char *name;
	commandfunc_t func;
	const char *helpline;
} commanddef_t;

static commanddef_t *command_find(const char *name);

static void command_version(int argc, const char **argv, client_t *client);
static void command_help(int argc, const char **argv, client_t *client);

static commanddef_t commands[] = {
	{ "help", command_help, "List available commands" },
	{ "version", command_version, "Display software version" },
};

void command_execute(client_t *client, const char *command) {
	char **args = NULL;
	int argc = 0;
	char buffer[64];
	size_t bufferp = 0;

	for (int i = 0; i <= strlen(command); i++) {
		const char c = command[i];

		if (c == ' ' || c == '\0') {
			char *arg = strdup(buffer);
			bufferp = 0;

			size_t index = argc++;
			args = realloc(args, argc * sizeof(char *));
			args[index] = arg;
		}
		else if (argc == 0 && bufferp == 0 && c == '/') {
			continue;
		}
		else {
			buffer[bufferp++] = c;
			buffer[bufferp] = '\0';
		}
	}

	if (args != NULL) {
		commanddef_t *command = command_find(args[0]);
		if (command != NULL) {
			command->func(argc, args, client);
		}

		for (int i = 0; i < argc; i++) {
			free(args[i]);
		}

		free(args);
	}
}

commanddef_t *command_find(const char *name) {
	for (int i = 0; i < sizeof(commands) / sizeof(commanddef_t); i++) {
		if (strcmp(name, commands[i].name) == 0) {
			return &commands[i];
		}
	}

	return NULL;
}

void command_version(int argc, const char **argv, client_t *client) {
	client_send_message(client, "&fThis server is running &eThirty %s", THIRTY_VERSION);
	client_send_message(client, "&fMercurial changeset: &e%s", HG_CHANGESET_HASH);
	client_send_message(client, "Thirty is licenced under the GNU AGPL v3 or later, and its");
	client_send_message(client, "source is available at https://dev.firestick.games/sean/thirty");
}

void command_help(int argc, const char **argv, client_t *client) {
	for (int i = 0; i < sizeof(commands) / sizeof(commanddef_t); i++) {
		commanddef_t *command = &commands[i];

		client_send_message(client, "&e%s&f - %s", command->name, command->helpline);
	}
}