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
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include "client.h"
#include "server.h"
#include "buffer.h"
#include "map.h"
#include "packet.h"
#include "util.h"
#include "cpe.h"
#include "blocks.h"
#include "rng.h"
#include "md5.h"
#include "sha1.h"
#include "b64.h"
#include "config.h"

#define BUFFER_SIZE (32 * 1024)
#define PING_INTERVAL (1.0)

static void client_receive(client_t *client);
static void client_login(client_t *client);
static void client_send(client_t *client, buffer_t *buffer);
static void client_handle_in_buffer(client_t *client, buffer_t *in_buffer, size_t r);
static void client_send_level(client_t *client);
static void client_start_mapsave(client_t *client);
static void client_start_fast_mapsave(client_t *client);
static bool client_verify_key(char name[65], char key[65]);
static void client_ws_upgrade(client_t *client, int r);
static void client_ws_handle_packet(client_t *client, int len);
static void client_ws_handle_chunk(client_t *client);
static void client_ws_decode_frame(client_t *client);
static void client_ws_disconnect(client_t *client, int code);
static void client_ws_wrap_packet(client_t *client, buffer_t *buffer);

void client_init(client_t *client, int fd, size_t idx) {
	memset(client, 0, sizeof(*client));

	client->socket_fd = fd;
	client->connected = true;
	client->idx = idx;
	client->in_buffer = buffer_allocate_memory(BUFFER_SIZE, false);
	client->out_buffer = buffer_allocate_memory(BUFFER_SIZE, false);
	client->mapsend_state = mapsend_none;
	client->mapgz_buffer = NULL;
	client->last_ping = 0;
	client->ping = 0;
	client->x = rng_next(server.global_rng, util_min(1023, (int)server.map->width)) + 0.5f;
	client->z = rng_next(server.global_rng, util_min(1023, (int)server.map->height)) + 0.5f;
	client->y = map_get_top(server.map, (size_t)client->x, (size_t)client->z) + 2.0f;
	client->yaw = 0.0f;
	client->pitch = 0.0f;
	client->spawned = false;
	client->num_extensions = 0;
	client->extensions = NULL;
	client->customblocks_support = -1;
	client->ws_can_switch = true;
	client->using_websocket = false;
	client->ws_state = 0;
	client->ws_opcode = 0;
	client->ws_frame_len = 0;
	client->ws_frame_read = 0;
	client->ws_mask_read = 0;
	client->ws_frame = NULL;
	client->ws_out_buffer = NULL;
}

void client_destroy(client_t *client) {
	free(client->extensions);
	closesocket(client->socket_fd);
	buffer_destroy(client->ws_out_buffer);
	buffer_destroy(client->ws_frame);
	buffer_destroy(client->in_buffer);
	buffer_destroy(client->out_buffer);
}

void client_tick(client_t *client) {
	if (!client->connected) {
		return;
	}

	client_receive(client);

	if (client->spawned && get_time_s() - client->last_ping >= PING_INTERVAL) {
		if (client_supports_extension(client, "TwoWayPing", 1)) {
			client->ping_key = (uint16_t) rng_next(server.global_rng, UINT16_MAX);
			buffer_write_uint8(client->out_buffer, packet_two_way_ping);
			buffer_write_uint8(client->out_buffer, 1);
			buffer_write_uint16be(client->out_buffer, client->ping_key);
			client_flush(client);
		}
		else {
			buffer_write_uint8(client->out_buffer, packet_ping);
			client_flush(client);
		}
		client->last_ping = get_time_s();
	}

	if (client->mapsend_state != mapsend_none) {
		if (client->mapsend_state == mapsend_success) {
			for (int i = 0; i < 4; i++) {
				if (client->mapgz_buffer == NULL || buffer_size(client->mapgz_buffer) == buffer_tell(client->mapgz_buffer)) {
					if (client->mapgz_buffer != NULL) {
						buffer_destroy(client->mapgz_buffer);
					}

					client->mapsend_state = mapsend_sent;
					client->mapgz_buffer = NULL;

					buffer_write_uint8(client->out_buffer, packet_level_finish);
					buffer_write_uint16be(client->out_buffer, server.map->width);
					buffer_write_uint16be(client->out_buffer, server.map->depth);
					buffer_write_uint16be(client->out_buffer, server.map->height);
					client_flush(client);

					buffer_write_uint8(client->out_buffer, packet_player_pos_angle);
					buffer_write_int8(client->out_buffer, -1);
					buffer_write_int16be(client->out_buffer, util_float2fixed(client->x));
					buffer_write_int16be(client->out_buffer, util_float2fixed(client->y));
					buffer_write_int16be(client->out_buffer, util_float2fixed(client->z));
					buffer_write_int8(client->out_buffer, 0);
					buffer_write_int8(client->out_buffer, 0);
					client_flush(client);

					for (size_t j = 0; j < server.num_clients; j++) {
						client_t *other = &server.clients[j];
						if (other == client) {
							continue;
						}

						// Inform the connecting client about others
						buffer_write_uint8(client->out_buffer, packet_player_spawn);
						buffer_write_uint8(client->out_buffer, other->idx);
						buffer_write_mcstr(client->out_buffer, other->name, true);
						buffer_write_int16be(client->out_buffer, util_float2fixed(other->x));
						buffer_write_int16be(client->out_buffer, util_float2fixed(other->y));
						buffer_write_int16be(client->out_buffer, util_float2fixed(other->y));
						buffer_write_int8(client->out_buffer, util_degrees2fixed(other->yaw));
						buffer_write_int8(client->out_buffer, util_degrees2fixed(other->pitch));
						client_flush(client);

						// and inform others about this client
						buffer_write_uint8(other->out_buffer, packet_player_spawn);
						buffer_write_uint8(other->out_buffer, client->idx);
						buffer_write_mcstr(other->out_buffer, client->name, true);
						buffer_write_int16be(other->out_buffer, util_float2fixed(client->x));
						buffer_write_int16be(other->out_buffer, util_float2fixed(client->y));
						buffer_write_int16be(other->out_buffer, util_float2fixed(client->y));
						buffer_write_int8(other->out_buffer, util_degrees2fixed(client->yaw));
						buffer_write_int8(other->out_buffer, util_degrees2fixed(client->pitch));
						client_flush(other);
					}

					client->spawned = true;

					server_broadcast("&f%s &ejoined the game.", client->name);

					break;
				}
				else {
					uint8_t data[1024];
					memset(data, 0, 1024);

					size_t len = buffer_read(client->mapgz_buffer, data, 1024);

					buffer_write_uint8(client->out_buffer, packet_level_chunk);
					buffer_write_uint16be(client->out_buffer, (uint16_t)len);
					buffer_write(client->out_buffer, data, 1024);
					buffer_write_uint8(client->out_buffer, 0);

					client_flush(client);
				}
			}
		}
		else if (client->mapsend_state == mapsend_failure) {
			buffer_write_uint8(client->out_buffer, packet_player_disconnect);
			buffer_write_mcstr(client->out_buffer, "Failed to send map data", false);
			client_flush(client);
		}
	}

	client_flush(client);
}

void client_receive(client_t *client) {
	buffer_seek(client->in_buffer, 0);
#ifdef _WIN32
	int r = recv(client->socket_fd, (char *)client->in_buffer->mem.data, (int)client->in_buffer->mem.size, 0);
#else
	int r = recv(client->socket_fd, client->in_buffer->mem.data, client->in_buffer->mem.size, 0);
#endif
	if (r == SOCKET_ERROR) {
		int e = socket_error();
		if (e == EAGAIN || e == SOCKET_EWOULDBLOCK) {
			return;
		}

		if (e == EPIPE || e == SOCKET_ECONNABORTED || e == SOCKET_ECONNRESET) {
			client->connected = false;
			client_disconnect(client, "Disconnected");
			return;
		}

		fprintf(stderr, "recv error %d\n", e);
		return;
	}

	if (r == 0) {
		return;
	}

	if (client->ws_can_switch && memcmp(client->in_buffer->mem.data, "GET ", 4) == 0) {
		client_ws_upgrade(client, r);
		return;
	}

	if (client->using_websocket) {
		client_ws_handle_packet(client, r);
	}
	else {
		client_handle_in_buffer(client, client->in_buffer, (size_t)r);
	}
}

void client_handle_in_buffer(client_t *client, buffer_t *in_buffer, size_t r) {
	while (buffer_tell(in_buffer) < (size_t)r) {
		uint8_t packet_id;
		buffer_read_uint8(in_buffer, &packet_id);

		switch (packet_id) {
			case packet_ident: {
				uint8_t protocol_version;
				char username[65];
				char key[65];
				uint8_t unused;

				client->ws_can_switch = false;

				buffer_read_uint8(in_buffer, &protocol_version);
				buffer_read_mcstr(in_buffer, username);
				buffer_read_mcstr(in_buffer, key);
				buffer_read_uint8(in_buffer, &unused);

				const bool supports_cpe = unused == 0x42;

				if (server.num_clients > config.server.max_players) {
					client_disconnect(client, "This server is full.");
					return;
				}

				for (size_t i = 0; i < server.num_clients; i++) {
					if (strcasecmp(server.clients[i].name, username) == 0) {
						client_disconnect(client, "Name already in use.");
						return;
					}
				}

				if (!config.server.offline && !client_verify_key(username, key)) {
					client_disconnect(client, "Authentication failed.");
					return;
				}

				strncpy(client->name, username, 64);

				if (supports_cpe) {
					buffer_write_uint8(client->out_buffer, packet_extinfo);
					buffer_write_mcstr(client->out_buffer, "Thirty", false);
					buffer_write_uint16be(client->out_buffer, cpe_count_supported());

					for (size_t i = 0; true; i++) {
						cpeext_t *ext = &supported_extensions[i];
						if (ext->name[0] == '\0') {
							break;
						}

						buffer_write_uint8(client->out_buffer, packet_extentry);
						buffer_write_mcstr(client->out_buffer, ext->name, false);
						buffer_write_int32be(client->out_buffer, ext->version);
					}

					client_flush(client);
				}
				else {
					client_login(client);
				}

				break;
			}

			case packet_extinfo: {
				char appname[65];
				uint16_t extcount;

				buffer_read_mcstr(in_buffer, appname);
				buffer_read_uint16be(in_buffer, &extcount);

				client->num_extensions = (size_t)extcount;
				client->extensions = calloc(extcount, sizeof(*client->extensions));

				printf("Client using %s with %d extensions\n", appname, extcount);

				break;
			}

			case packet_extentry: {
				char name[65];
				int32_t version;

				buffer_read_mcstr(in_buffer, name);
				buffer_read_int32be(in_buffer, &version);

				size_t i;
				for (i = 0; i < client->num_extensions; i++) {
					if (i == client->num_extensions) {
						fprintf(stderr, "extension overrun!\n");
						client_disconnect(client, "Invalid data.");
						return;
					}

					if (client->extensions[i].name[0] == '\0') {
						strncpy(client->extensions[i].name, name, 65);
						client->extensions[i].version = version;
						break;
					}
				}

				if (i == client->num_extensions - 1) {
					client_login(client);
				}

				break;
			}

			case packet_set_block_client: {
				uint16_t x, y, z;
				uint8_t mode;
				uint8_t block;

				buffer_read_uint16be(in_buffer, &x);
				buffer_read_uint16be(in_buffer, &y);
				buffer_read_uint16be(in_buffer, &z);
				buffer_read_uint8(in_buffer, &mode);
				buffer_read_uint8(in_buffer, &block);

				map_set(server.map, x, y, z, mode == 0x00 ? 0x00 : block);

				break;
			}

			case packet_message: {
				uint8_t unused;
				char msg[65];

				buffer_read_uint8(in_buffer, &unused);
				buffer_read_mcstr(in_buffer, msg);

				for (size_t i = 0; i < 64; i++) {
					if (msg[i] == '%') {
						msg[i] = '&';
					}
				}

				if (client->spawned) {
					server_broadcast("&e%s: &f%s", client->name, msg);
				}

				break;
			}

			case packet_player_pos_angle: {
				int8_t unused;
				int16_t x, y, z;
				int8_t yaw, pitch;

				buffer_read_int8(in_buffer, &unused);
				buffer_read_int16be(in_buffer, &x);
				buffer_read_int16be(in_buffer, &y);
				buffer_read_int16be(in_buffer, &z);
				buffer_read_int8(in_buffer, &yaw);
				buffer_read_int8(in_buffer, &pitch);

				client->x = util_fixed2float(x);
				client->y = util_fixed2float(y);
				client->z = util_fixed2float(z);
				client->yaw = util_fixed2degrees(yaw);
				client->pitch = util_fixed2degrees(pitch);

				for (size_t i = 0; i < server.num_clients; i++) {
					client_t *other = &server.clients[i];
					if (other == client) {
						continue;
					}

					buffer_write_uint8(other->out_buffer, packet_player_pos_angle);
					buffer_write_int8(other->out_buffer, client->idx);
					buffer_write_int16be(other->out_buffer, util_float2fixed(client->x));
					buffer_write_int16be(other->out_buffer, util_float2fixed(client->y));
					buffer_write_int16be(other->out_buffer, util_float2fixed(client->z));
					buffer_write_int8(other->out_buffer, util_degrees2fixed(client->yaw));
					buffer_write_int8(other->out_buffer, util_degrees2fixed(client->pitch));
					client_flush(other);
				}

				break;
			}

			case packet_custom_block_support_level: {
				uint8_t level;
				buffer_read_uint8(in_buffer, &level);
				client->customblocks_support = level;
				client_send_level(client);
				break;
			}

			case packet_two_way_ping: {
				uint8_t direction;
				uint16_t data;

				buffer_read_uint8(in_buffer, &direction);
				buffer_read_uint16be(in_buffer, &data);

				if (direction == 0) {
					buffer_write_uint8(client->out_buffer, packet_two_way_ping);
					buffer_write_uint8(client->out_buffer, direction);
					buffer_write_uint16be(client->out_buffer, data);
					client_flush(client);
				}
				else if (data == client->ping_key) {
					client->ping = get_time_s() - client->last_ping;
				}

				break;
			}

			default: {
				fprintf(stderr, "client %zu (%s) sent unknown packet 0x%02x\n", client->idx, client->name, packet_id);
				break;
			};
		}
	}
}

bool client_verify_key(char name[65], char key[65]) {
	char work[128];
	snprintf(work, sizeof(work), "%s%s", server.salt, name);

	uint8_t hash[16];
	char hashstr[33];
	md5String(work, hash);
	for (size_t i = 0; i < 16; i++) {
		snprintf(hashstr + i * 2, 33 - i * 2, "%02x", hash[i]);
	}

	return strcasecmp(hashstr, key) == 0;
}

void client_login(client_t *client) {
	const bool cp437 = client_supports_extension(client, "FullCP437", 1);
	const bool customblocks = client_supports_extension(client, "CustomBlocks", 1);
	const bool textcolours = client_supports_extension(client, "TextColors", 1);

	if (customblocks && client->customblocks_support == -1) {
		buffer_write_uint8(client->out_buffer, packet_custom_block_support_level);
		buffer_write_uint8(client->out_buffer, CPE_CUSTOMBLOCKS_LEVEL);
		client_flush(client);
	}

	buffer_write_uint8(client->out_buffer, packet_ident);
	buffer_write_uint8(client->out_buffer, 0x07);
	buffer_write_mcstr(client->out_buffer, config.server.name, cp437);
	buffer_write_mcstr(client->out_buffer, config.server.motd, cp437);
	buffer_write_uint8(client->out_buffer, 0x64);
	client_flush(client);

	if (textcolours) {
		for (size_t i = 0; i < config.num_colours; i++) {
			buffer_write_uint8(client->out_buffer, packet_set_text_colour);
			buffer_write_uint8(client->out_buffer, config.colours[i].r);
			buffer_write_uint8(client->out_buffer, config.colours[i].g);
			buffer_write_uint8(client->out_buffer, config.colours[i].b);
			buffer_write_uint8(client->out_buffer, config.colours[i].a);
			buffer_write_uint8(client->out_buffer, (uint8_t)config.colours[i].code);
		}
	}

	if (!customblocks) {
		client_send_level(client);
	}
}

void client_send_level(client_t *client) {
	const bool fastmap = client_supports_extension(client, "FastMap", 1);

	if (fastmap && client->customblocks_support >= CPE_CUSTOMBLOCKS_LEVEL) {
		buffer_write_uint8(client->out_buffer, packet_level_init);
		buffer_write_uint32be(client->out_buffer, server.map->width * server.map->depth * server.map->height);
		client_flush(client);
		client_start_fast_mapsave(client);
	}
	else {
		buffer_write_uint8(client->out_buffer, packet_level_init);
		client_flush(client);
		client_start_mapsave(client);
	}
}

void client_send(client_t *client, buffer_t *buffer) {
	if (buffer->mem.offset == 0) {
		return;
	}

	int sendflags = 0;
#ifndef _WIN32
	sendflags |= MSG_NOSIGNAL;
#endif

#ifdef _WIN32
	int r = send(client->socket_fd, (const char *)buffer->mem.data, (int)buffer->mem.offset, sendflags);
#else
	int r = send(client->socket_fd, buffer->mem.data, buffer->mem.offset, sendflags);
#endif
	buffer_seek(buffer, 0);

	if (r == SOCKET_ERROR) {
		int e = socket_error();
		if (e == EAGAIN || e == SOCKET_EWOULDBLOCK) {
			return;
		}

		if (e == EPIPE || e == SOCKET_ECONNABORTED || e == SOCKET_ECONNRESET) {
			client->connected = false;
			client_disconnect(client, "Disconnected");
			return;
		}

		fprintf(stderr, "send error %d\n", e);
	}
}

void client_flush_buffer(client_t *client, buffer_t *buffer) {
	if (!client->connected) {
		return;
	}

	pthread_mutex_lock(&client->out_mutex);

	if (client->using_websocket) {
		client_ws_wrap_packet(client, buffer);
		client_send(client, client->ws_out_buffer);
		buffer_seek(buffer, 0);
	}
	else {
		client_send(client, buffer);
	}

	pthread_mutex_unlock(&client->out_mutex);
}

void client_flush(client_t *client) {
	client_flush_buffer(client, client->out_buffer);
}

void client_start_mapsave(client_t *client) {
	mapsend_t *data = malloc(sizeof(*data));
	data->client = client;
	data->width = server.map->width;
	data->height = server.map->height;
	data->depth = server.map->depth;

	const uint32_t num_blocks = data->width * data->height * data->depth;
	const size_t bufsize = sizeof(uint32_t) + num_blocks;
	data->data = malloc(bufsize);

	buffer_t *buf = buffer_create_memory(data->data, bufsize);
	buffer_write_uint32be(buf, num_blocks);
	buffer_write(buf, server.map->blocks, num_blocks);
	buffer_destroy(buf);

	pthread_attr_t attr;
	pthread_attr_init(&attr);

	pthread_t thread;
	pthread_create(&thread, &attr, mapsend_thread_start, data);
}

void client_start_fast_mapsave(client_t *client) {
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	pthread_t thread;
	pthread_create(&thread, &attr, mapsend_fast_thread_start, client);
}

void client_disconnect(client_t *client, const char *msg) {
	if (client->connected) {
		buffer_write_uint8(client->out_buffer, packet_player_disconnect);
		buffer_write_mcstr(client->out_buffer, msg, client_supports_extension(client, "FullCP437", 1));
		client_flush(client);

		if (client->using_websocket) {
			client_ws_disconnect(client, 1000);
		}
	}

	client->connected = false;

	if (client->spawned) {
		client->spawned = false;
		server_broadcast("&f%s &edisconnected (&f%s&e)", client->name, msg);

		for (size_t i = 0; i < server.num_clients; i++) {
			client_t *other = &server.clients[i];
			if (other == client) {
				continue;
			}

			buffer_write_uint8(other->out_buffer, packet_player_despawn);
			buffer_write_int8(other->out_buffer, client->idx);
			client_flush(other);
		}
	}
}

bool client_supports_extension(client_t *client, const char *name, int version) {
	for (size_t i = 0; i < client->num_extensions; i++) {
		if (strcasecmp(client->extensions[i].name, name) == 0 && client->extensions[i].version == version) {
			return true;
		}
	}

	return false;
}

void client_ws_upgrade(client_t *client, int r) {
	client->in_buffer->mem.data[r + 1] = 0;

	size_t num_headers;
	httpheader_t *headers = util_httpheaders_parse((const char *) client->in_buffer->mem.data, &num_headers);

	bool is_valid =
			util_httpheaders_get(headers, num_headers, "Connection") != NULL
			&& (strstr(util_httpheaders_get(headers, num_headers, "Connection"), "upgrade") || strstr(util_httpheaders_get(headers, num_headers, "Connection"), "Upgrade"))
			&& util_httpheaders_get(headers, num_headers, "Upgrade") != NULL
			&& strcasecmp(util_httpheaders_get(headers, num_headers, "Upgrade"), "WebSocket") == 0
			&& util_httpheaders_get(headers, num_headers, "Sec-WebSocket-Version") != NULL
			&& strcmp(util_httpheaders_get(headers, num_headers, "Sec-WebSocket-Version"), "13") == 0
			&& util_httpheaders_get(headers, num_headers, "Sec-WebSocket-Protocol") != NULL
			&& strcasecmp(util_httpheaders_get(headers, num_headers, "Sec-WebSocket-Protocol"), "ClassiCube") == 0
			&& util_httpheaders_get(headers, num_headers, "Sec-WebSocket-Key") != NULL;

	if (!is_valid) {
		client_disconnect(client, "");
		return;
	}

	char key[512];
	snprintf(key, 512, "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", util_httpheaders_get(headers, num_headers, "Sec-WebSocket-Key"));

	unsigned char key_sha1[20];
	SHA1(key_sha1, key, strlen(key));

	char *key_b64 = base64_enc_malloc(key_sha1, 20);

	char response[2048];
	snprintf(response, 2048,
			 "HTTP/1.1 101 Switching Protocols\r\n"
			 "Connection: upgrade\r\n"
			 "Upgrade: websocket\r\n"
			 "Sec-WebSocket-Accept: %s\r\n"
			 "Sec-WebSocket-Protocol: ClassiCube\r\n"
			 "Server: %s\r\n"
			 "\r\n",

			 key_b64,
			 "Thirty"
	);

	buffer_write(client->out_buffer, response, strlen(response));
	client_flush(client);
	client->using_websocket = true;
	client->ws_out_buffer = buffer_allocate_memory(BUFFER_SIZE + 4, false);

	free(key_b64);
	util_httpheaders_destroy(headers, num_headers);
}

void client_ws_handle_packet(client_t *client, int len) {
	while (buffer_tell(client->in_buffer) < (size_t)len) {
		client_ws_handle_chunk(client);
	}
}

enum {
	wss_handle_header1,
	wss_extended_length1,
	wss_mask,
	wss_data
};

void client_ws_handle_chunk(client_t *client) {
	switch (client->ws_state) {
		case wss_handle_header1: {
			uint8_t tmp;

			buffer_read_uint8(client->in_buffer, &tmp);
			client->ws_opcode = tmp & 0x0F;
			buffer_read_uint8(client->in_buffer, &tmp);
			int flags = tmp & 0x7F;

			if (flags == 127) {
				// dc
				client_ws_disconnect(client, 1009);
				return;
			}
			else if (flags == 126) {
				client->ws_state = wss_extended_length1;
				goto extended_length1;
			}
			else {
				client->ws_frame_len = flags;
				client->ws_state = wss_mask;
				goto mask;
			}
		}

		case wss_extended_length1:
		extended_length1: {
			uint16_t tmp;
			buffer_read_uint16be(client->in_buffer, &tmp);
			client->ws_frame_len = tmp;
			client->ws_state = wss_mask;

			goto mask;
		}

		case wss_mask:
		mask: {
			buffer_read(client->in_buffer, client->ws_mask, 4);

			client->ws_state = wss_data;
			goto data;
		}

		case wss_data:
		data: {
			if (client->ws_frame == NULL || client->ws_frame_len > buffer_size(client->ws_frame)) {
				buffer_destroy(client->ws_frame);
				client->ws_frame = buffer_allocate_memory(client->ws_frame_len, false);
			}

			buffer_seek(client->ws_frame, 0);

			size_t copy = client->ws_frame_len - client->ws_frame_read;
			buffer_read(client->in_buffer, client->ws_frame->mem.data + client->ws_frame_read, copy);
			client->ws_frame_read += copy;

			if (client->ws_frame_read == client->ws_frame_len) {
				client_ws_decode_frame(client);
				client->ws_mask_read = 0;
				client->ws_frame_read = 0;
				client->ws_state = wss_handle_header1;
			}

			break;
		}

		default: break;
	}
}

void client_ws_decode_frame(client_t *client) {
	for (size_t i = 0; i < client->ws_frame_len; i++) {
		client->ws_frame->mem.data[i] ^= client->ws_mask[i & 3];
	}

	switch (client->ws_opcode) {
		case 0x00:
		case 0x02: {
			client_handle_in_buffer(client, client->ws_frame, client->ws_frame_len);
			break;
		}

		case 0x08: {
			client_ws_disconnect(client, 1000);
			break;
		}

		default: {
			client_ws_disconnect(client, 1003);
			break;
		}
	}
}

void client_ws_wrap_packet(client_t *client, buffer_t *buffer) {
	size_t data_len = buffer_tell(buffer);
	if (data_len == 0) {
		return;
	}

	buffer_write_uint8(client->ws_out_buffer, 0x82U);

	if (data_len >= 126) {
		buffer_write_uint8(client->ws_out_buffer, 126);
		buffer_write_uint16be(client->ws_out_buffer, data_len);
	}
	else {
		buffer_write_uint8(client->ws_out_buffer, data_len);
	}

	buffer_write(client->ws_out_buffer, buffer->mem.data, data_len);
}

void client_ws_disconnect(client_t *client, int code) {
	buffer_write_uint8(client->ws_out_buffer, 0x88);
	buffer_write_uint8(client->ws_out_buffer, 0x02);
	buffer_write_uint16be(client->ws_out_buffer, code);
	client_send(client, client->ws_out_buffer);
}
