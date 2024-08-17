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
#include "config.h"
#include "log.h"
#include "version.h"
#include "cpe.h"
#include "map.h"
#include "server.h"
#include "namelist.h"

typedef void (*commandfunc_t)(int argc, const char **argv, client_t *client);

typedef struct commanddef_s {
	const char *name;
	commandfunc_t func;
	const char *helpline;
	bool op_only;
} commanddef_t;

static commanddef_t *command_find(const char *name);

static void command_version(int argc, const char **argv, client_t *client);
static void command_help(int argc, const char **argv, client_t *client);
static void command_info(int argc, const char **argv, client_t *client);
static void command_teleport(int argc, const char **argv, client_t *client);
static void command_ban(int argc, const char **argv, client_t *client);
static void command_ipban(int argc, const char **argv, client_t *client);
static void command_whitelist(int argc, const char **argv, client_t *client);
static void command_op(int argc, const char **argv, client_t *client);
static void command_save(int argc, const char **argv, client_t *client);

static commanddef_t commands[] = {
	{ "ban", command_ban, "Manage username bans", true },
	{ "ban-ip", command_ipban, "Manage IP bans", true },
	{ "help", command_help, "List available commands", false },
	{ "info", command_info, "View client info", false },
	{ "op", command_op, "Manage server admins", true },
	{ "save", command_save, "Save the level", true },
	{ "teleport", command_teleport, "Teleport a player", false },
	{ "version", command_version, "Display software version", false },
	{ "whitelist", command_whitelist, "Manage server whitelist", true },
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
			command->func(argc - 1, (const char **)args, client);
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

		if (command->op_only && !client->is_op) {
			continue;
		}

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
	if (argc == 4) {
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

static void namelist_command(int argc, const char **argv, client_t *client, namelist_t *namelist, const char *addWord, const char *removeWord, const char *listTitle) {
	if (!client->is_op) {
		client_send_message(client, "&cThis command is op-only");
		return;
	}

	const char *subcommand = argv[1];
	if (argc >= 2 && strcasecmp(subcommand, "add") == 0) {
		const char *player = argv[2];
		namelist_add(namelist, player);

		client_send_message(client, "&aPlayer '%s' has been %s.", player, addWord);
	}
	else if (argc >= 2 && strcasecmp(subcommand, "remove") == 0) {
		const char *player = argv[2];
		namelist_remove(namelist, player);

		client_send_message(client, "&aPlayer '%s' has been %s.", player, removeWord);
	}
	else if (argc >= 1 && strcasecmp(subcommand, "list") == 0) {
		client_send_message(client, "&e%s:", listTitle);
		for (size_t i = 0; i < namelist->num_names; i++) {
			if (namelist->names[i] != NULL) {
				client_send_message(client, "&f- &e%s", namelist->names[i]);
			}
		}
	} else {
		client_send_message(client, "&e Syntax: &f/%s <add | remove | list> [player]", argv[0]);
	}
}

void command_ban(int argc, const char **argv, client_t *client) {
	namelist_command(argc, argv, client, server.banned_users, "banned", "unbanned", "Banned users");
}

void command_ipban(int argc, const char **argv, client_t *client) {
	namelist_command(argc, argv, client, server.banned_ips, "banned", "unbanned", "Banned IPs");
}

void command_whitelist(int argc, const char **argv, client_t *client) {
	if (!config.server.enable_whitelist) {
		client_send_message(client, "&cThe server whitelist is not enabled.");
		return;
	}

	namelist_command(argc, argv, client, server.whitelist, "added", "removed", "Whitelisted users");
}

void command_op(int argc, const char **argv, client_t *client) {
	namelist_command(argc, argv, client, server.ops, "opped", "deopped", "Operators");
}

void command_save(int argc, const char **argv, client_t *client) {
	(void) argc;
	(void) argv;

	if (!client->is_op) {
		client_send_message(client, "&cThis command is op-only");
		return;
	}

	map_save(server.map);
}
