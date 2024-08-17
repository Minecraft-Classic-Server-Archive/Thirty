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
#include "cpe.h"
#include "server.h"

typedef void (*commandfunc_t)(int argc, const char **argv, client_t *client);

typedef struct commanddef_s {
	const char *name;
	commandfunc_t func;
	const char *helpline;
} commanddef_t;

static commanddef_t *command_find(const char *name);

static void command_version(int argc, const char **argv, client_t *client);
static void command_help(int argc, const char **argv, client_t *client);
static void command_info(int argc, const char **argv, client_t *client);
static void command_teleport(int argc, const char **argv, client_t *client);

static commanddef_t commands[] = {
	{ "help", command_help, "List available commands" },
	{ "info", command_info, "View client info" },
	{ "teleport", command_teleport, "Teleport a player" },
	{ "version", command_version, "Display software version" },
};

void command_execute(client_t *client, const char *command) {
	char **args = NULL;
	int argc = 0;
	char buffer[64];
	size_t bufferp = 0;

	for (size_t i = 0; i <= strlen(command); i++) {
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
			command->func(argc, (const char **)args, client);
		}

		for (int i = 0; i < argc; i++) {
			free(args[i]);
		}

		free(args);
	}
}

commanddef_t *command_find(const char *name) {
	for (size_t i = 0; i < sizeof(commands) / sizeof(commanddef_t); i++) {
		if (strcmp(name, commands[i].name) == 0) {
			return &commands[i];
		}
	}

	return NULL;
}

void command_version(int argc, const char **argv, client_t *client) {
	(void) argc;
	(void) argv;

	client_send_message(client, "&fThis server is running &eThirty %s", THIRTY_VERSION);
	client_send_message(client, "&fMercurial changeset: &e%s", HG_CHANGESET_HASH);
	client_send_message(client, "Thirty is licenced under the GNU AGPL v3 or later, and its");
	client_send_message(client, "source is available at https://dev.firestick.games/sean/thirty");
}

void command_help(int argc, const char **argv, client_t *client) {
	(void) argc;
	(void) argv;

	for (size_t i = 0; i < sizeof(commands) / sizeof(commanddef_t); i++) {
		commanddef_t *command = &commands[i];

		client_send_message(client, "&e%s&f - %s", command->name, command->helpline);
	}
}

void command_info(int argc, const char **argv, client_t *client) {
	(void) argc;
	(void) argv;

	client_send_message(client, "&eProtocol version: &f%d", client->protocol_version);

	client_send_message(client, "&eCPE extensions:&f (&amutual&f | &bclient&f | &dserver&f)");
	for (size_t j = 0; j < client->num_extensions; j++) {
		cpeext_t *ext = &client->extensions[j];
		const char colour = cpe_extension_supported(ext->name, ext->version) ? 'a' : 'b';
		client_send_message(client, "&f - &%c%s v%d", colour, ext->name, ext->version);
	}

	for (size_t j = 0; j < cpe_count_supported(); j++) {
		const cpeext_t *ext = &supported_extensions[j];
		if (!client_supports_extension(client, ext->name, ext->version)) {
			client_send_message(client, "&f - &d%s v%d", ext->name, ext->version);
		}
	}
}

void command_teleport(int argc, const char **argv, client_t *client) {
	if (argc <= 3) {
		client_send_message(client, "&eSyntax: &f/%s [player] <x> <y> <z>", argv[0]);
		return;
	}

	int o = 0;
	client_t *target = NULL;
	if (argc == 5) {
		if (!client->is_op && strcasecmp(argv[1], client->name) != 0) {
			client_send_message(client, "&eOnly ops can teleport other players");
			return;
		}

		const char *player = argv[1];
		for (size_t i = 0; i < server.num_clients; i++) {
			if (strcasecmp(server.clients[i].name, player) == 0) {
				target = &server.clients[i];
				break;
			}
		}

		if (target == NULL) {
			client_send_message(client, "&c'&f%s&c' is not a player", player);
			return;
		}
		o = 1;
	}
	else {
		target = client;
	}

	float x = strtof(argv[1 + o], NULL);
	float y = strtof(argv[2 + o], NULL);
	float z = strtof(argv[3 + o], NULL);

	client_teleport(target, x, y, z, 0.0f, 0.0f);
}
