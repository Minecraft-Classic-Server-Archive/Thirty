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
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#ifdef USE_POLL
#include <poll.h>
#elif !defined(_WIN32)
#include <sys/select.h>
#endif
#include "commands.h"
#include "client.h"
#include "config.h"
#include "version.h"
#include "cpe.h"
#include "map.h"
#include "server.h"
#include "namelist.h"
#include "log.h"

typedef void (*commandfunc_t)(int argc, const char **argv, client_t *client);

typedef struct commanddef_s {
	const char *name;
	commandfunc_t func;
	const char *helpline;
	bool op_only;
} commanddef_t;

static void command_readline_callback(char *line);
static char **command_readline_completion(const char *text, int start, int end);
static char *command_readline_generator(const char *text, int state);
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
static void command_stop(int argc, const char **argv, client_t *client);
static void command_online(int argc, const char **argv, client_t *client);
static void command_env(int argc, const char **argv, client_t *client);

bool readline_enabled = false;
bool handling_readline = false;

static commanddef_t commands[] = {
	{ "ban", command_ban, "Manage username bans", true },
	{ "ban-ip", command_ipban, "Manage IP bans", true },
	{ "env", command_env, "Change map environmental settings", true },
	{ "help", command_help, "List available commands", false },
	{ "info", command_info, "View client info", false },
	{ "op", command_op, "Manage server admins", true },
	{ "online", command_online, "List online players", false },
	{ "save", command_save, "Save the level", true },
	{ "stop", command_stop, "Stop the server", true },
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
		else {
			client_send_message(client, msgtype_chat, "&cNo command exists with that name.");
		}

		for (int i = 0; i < argc; i++) {
			free(args[i]);
		}

		free(args);
	}
}

void command_readline_init(void) {
	rl_readline_name = "thirty";
	rl_callback_handler_install("> ", command_readline_callback);
	rl_attempted_completion_function = command_readline_completion;
	readline_enabled = true;
}

void command_readline_shutdown(void) {
	readline_enabled = false;
	rl_clear_visible_line();
	rl_callback_handler_remove();
}

void command_readline_callback(char *line) {
	if (!line) {
		return;
	}

	handling_readline = true;
	command_execute(&command_standin, line);
	handling_readline = false;

	add_history(line);
	free(line);
}

char **command_readline_completion(const char *text, int start, int end) {
	(void) text;
	(void) end;

	if (start == 0) {
		return rl_completion_matches(text, command_readline_generator);
	}

	return NULL;
}

char *command_readline_generator(const char *text, int state) {
	static size_t index = 0;
	static size_t len = 0;

	if (state == 0) {
		index = 0;
		len = strlen(text);
	}

	while (index < sizeof(commands) / sizeof(commanddef_t)) {
		commanddef_t *cmd = &commands[index++];

		if (strncmp(cmd->name, text, len) == 0) {
			return strdup(cmd->name);
		}
	}

	return NULL;
}

void command_tick_readline(void) {
#if defined(USE_POLL)
	struct pollfd fd = { fileno(stdin), POLLIN, 0 };

	int r = poll(&fd, 1, 0);
	if (r < 0) {
		perror("poll on stdin");
		return;
	}

	if (fd.revents == POLLIN) {
		rl_callback_read_char();
	}
#elif defined(_WIN32)
	DWORD n = 0;
	GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE), &n);
	if (n > 0) {
		rl_callback_read_char();
	}
#else
	fd_set fds;
	struct timeval tv = { 0, 0 };
	FD_ZERO(&fds);
	FD_SET(fileno(stdin), &fds);

	int r = select(1, &fds, NULL, NULL, &tv);
	if (r == -1) {
		perror("select on stdin");
	}
	else if (r > 0) {
		rl_callback_read_char();
	}
#endif
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

	client_send_message(client, msgtype_chat, "&fThis server is running &eThirty %s", THIRTY_VERSION);
	client_send_message(client, msgtype_chat, "&fMercurial changeset: &e%s", HG_CHANGESET_HASH);
	client_send_message(client, msgtype_chat, "Thirty is licenced under the GNU AGPL v3 or later, and its");
	client_send_message(client, msgtype_chat, "source is available at https://dev.firestick.games/sean/thirty");
}

void command_help(int argc, const char **argv, client_t *client) {
	(void) argc;
	(void) argv;

	for (size_t i = 0; i < sizeof(commands) / sizeof(commanddef_t); i++) {
		commanddef_t *command = &commands[i];

		if (command->op_only && !client->is_op) {
			continue;
		}

		client_send_message(client, msgtype_chat, "&e%s&f - %s", command->name, command->helpline);
	}
}

void command_info(int argc, const char **argv, client_t *client) {
	(void) argc;
	(void) argv;

	client_send_message(client, msgtype_chat, "&eProtocol version: &f%d", client->protocol_version);

	client_send_message(client, msgtype_chat, "&eCPE extensions:&f (&amutual&f | &bclient&f | &dserver&f)");
	for (size_t j = 0; j < client->num_extensions; j++) {
		cpeext_t *ext = &client->extensions[j];
		const char colour = cpe_extension_supported(ext->name, ext->version) ? 'a' : 'b';
		client_send_message(client, msgtype_chat, "&f - &%c%s v%d", colour, ext->name, ext->version);
	}

	for (size_t j = 0; j < cpe_count_supported(); j++) {
		const cpeext_t *ext = &supported_extensions[j];
		if (!client_supports_extension(client, ext->name, ext->version)) {
			client_send_message(client, msgtype_chat, "&f - &d%s v%d", ext->name, ext->version);
		}
	}
}

void command_teleport(int argc, const char **argv, client_t *client) {
	if (argc < 3) {
		client_send_message(client, msgtype_chat, "&eSyntax: &f/%s [player] <x> <y> <z>", argv[0]);
		return;
	}

	int o = 0;
	client_t *target = NULL;
	if (argc == 4) {
		if (!client->is_op && strcasecmp(argv[1], client->name) != 0) {
			client_send_message(client, msgtype_chat, "&eOnly ops can teleport other players");
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
			client_send_message(client, msgtype_chat, "&c'&f%s&c' is not a player", player);
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

	if (x < 0.0f || x >= server.map->width || y < 0.0f || y >= server.map->depth || z < 0.0f || z >= server.map->height) {
		client_send_message(client, msgtype_chat, "&cThat position is outside of the world");
		return;
	}

	client_teleport(target, x, y, z, 0.0f, 0.0f);
}

static void namelist_command(int argc, const char **argv, client_t *client, namelist_t *namelist, const char *addWord, const char *removeWord, const char *listTitle) {
	if (!client->is_op) {
		client_send_message(client, msgtype_chat, "&cThis command is op-only");
		return;
	}

	const char *subcommand = argv[1];
	if (argc >= 2 && strcasecmp(subcommand, "add") == 0) {
		const char *player = argv[2];
		namelist_add(namelist, player);

		client_send_message(client, msgtype_chat, "&aPlayer '%s' has been %s.", player, addWord);
	}
	else if (argc >= 2 && strcasecmp(subcommand, "remove") == 0) {
		const char *player = argv[2];
		namelist_remove(namelist, player);

		client_send_message(client, msgtype_chat, "&aPlayer '%s' has been %s.", player, removeWord);
	}
	else if (argc >= 1 && strcasecmp(subcommand, "list") == 0) {
		client_send_message(client, msgtype_chat, "&e%s:", listTitle);
		for (size_t i = 0; i < namelist->num_names; i++) {
			if (namelist->names[i] != NULL) {
				client_send_message(client, msgtype_chat, "&f- &e%s", namelist->names[i]);
			}
		}
	} else {
		client_send_message(client, msgtype_chat, "&e Syntax: &f/%s <add | remove | list> [player]", argv[0]);
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
		client_send_message(client, msgtype_chat, "&cThe server whitelist is not enabled.");
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
		client_send_message(client, msgtype_chat, "&cThis command is op-only");
		return;
	}

	map_save(server.map);
}

void command_stop(int argc, const char **argv, client_t *client) {
	(void) argc;
	(void) argv;

	if (!client->is_op) {
		client_send_message(client, msgtype_chat, "&cThis command is op-only");
		return;
	}

	log_printf(log_info, "%s is stopping the server", client->name);
	server_stop();
}

void command_online(int argc, const char **argv, client_t *client) {
	(void) argc;
	(void) argv;
	char msg[256];

	size_t actual_total = 0;

	for (size_t i = 0; i < server.num_clients; i++) {
		if (server.clients[i].spawned) {
			actual_total++;
		}
	}

	client_send_message(client, msgtype_chat, "&eThere %s &f%zu &eplayer%s online:", actual_total == 1 ? "is" : "are", actual_total, actual_total == 1 ? "" : "s");

	for (size_t i = 0; i < server.num_clients; i++) {
		client_t *c = &server.clients[i];

		memset(msg, 0, sizeof(msg));
		strcat(msg, "&e- ");

		if (!client->is_op && !c->spawned) {
			continue;
		}

		if (!c->spawned) {
			strcat(msg, "&7");
		}
		else if (c->is_op) {
			strcat(msg, "&c");
		}
		else {
			strcat(msg, "&f");
		}

		strcat(msg, c->name);

		if (c->is_op) {
			strcat(msg, " &f(op)");
		}
		if (!c->spawned) {
			strcat(msg, " &f(joining)");
		}

		client_send_message(client, msgtype_chat, "%s", msg);
	}
}

void command_env(int argc, const char **argv, client_t *client) {
	if (!client->is_op) {
		client_send_message(client, msgtype_chat, "&cThis command is op-only");
		return;
	}

	if (argc == 0) {
		client_send_message(client, msgtype_chat, "&e Syntax: &f/%s <colour> <type> <r> <g> <b>", argv[0]);
		client_send_message(client, msgtype_chat, "&e Syntax: &f/%s <colour> <type> default", argv[0]);
		client_send_message(client, msgtype_chat, "&e Syntax: &f/%s <weather> <clear|rain|snow>", argv[0]);
		return;
	}

	map_t *map = server.map;

	const char *subcommand = argv[1];
	if (strcmp(subcommand, "colour") == 0 || strcmp(subcommand, "color") == 0) {
		if (argc != 5 && argc != 3) {
			client_send_message(client, msgtype_chat, "&e Syntax: &f/%s <colour> <type> <r> <g> <b>", argv[0]);
			client_send_message(client, msgtype_chat, "&e Syntax: &f/%s <colour> <type> default", argv[0]);
			return;
		}

		const char *type = argv[2];
		int what = -1;
		if (strcmp(type, "sky") == 0) what = env_colour_sky;
		if (strcmp(type, "cloud") == 0) what = env_colour_cloud;
		if (strcmp(type, "fog") == 0) what = env_colour_fog;
		if (strcmp(type, "ambient") == 0) what = env_colour_ambient;
		if (strcmp(type, "sunlight") == 0) what = env_colour_sunlight;
		if (strcmp(type, "skybox") == 0) what = env_colour_skybox;

		if (what == -1) {
			client_send_message(client, msgtype_chat, "&cInvalid colour type");
			return;
		}

		const char *rs = argv[3];
		if (strcmp(rs, "default") == 0) {
			rgb_t value = { ENV_COLOUR_DEFAULT };
			map_set_colour(map, what, &value);
		}
		else if (argc == 5) {
			const char *gs = argv[4];
			const char *bs = argv[5];

			rgb_t value;
			value.r = atoi(rs);
			value.g = atoi(gs);
			value.b = atoi(bs);
			map_set_colour(map, what, &value);
		}
	}
	else if (strcmp(subcommand, "weather") == 0) {
		if (argc != 2) {
			client_send_message(client, msgtype_chat, "&e Syntax: &f/%s <weather> <clear|rain|snow>", argv[0]);
			return;
		}

		const char *type = argv[2];
		int what = -1;
		if (strcmp(type, "clear") == 0) what = weather_clear;
		if (strcmp(type, "rain") == 0) what = weather_rain;
		if (strcmp(type, "snow") == 0) what = weather_snow;

		if (what == -1) {
			client_send_message(client, msgtype_chat, "&cInvalid weather type");
			return;
		}

		map_set_weather(map, what);
	}
}
