#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include "client.h"
#include "server.h"
#include "buffer.h"
#include "map.h"
#include "packet.h"
#include "util.h"
#include "cpe.h"

#define BUFFER_SIZE (32 * 1024)
#define PING_INTERVAL (1 * 20)

static void client_receive(client_t *client);
static void client_login(client_t *client);
static void client_start_mapsave(client_t *client);
static void client_start_fast_mapsave(client_t *client);

void client_init(client_t *client, int fd, size_t idx) {
	memset(client, 0, sizeof(*client));

	client->socket_fd = fd;
	client->connected = true;
	client->idx = idx;
	client->in_buffer = buffer_allocate_memory(BUFFER_SIZE);
	client->out_buffer = buffer_allocate_memory(BUFFER_SIZE);
	client->mapsend_state = mapsend_none;
	client->mapgz_buffer = NULL;
	client->last_ping = 0;
	client->x = server.map->width / 2.0f;
	client->y = (server.map->depth / 2.0f) + 2.0f;
	client->z = server.map->height / 2.0f;
	client->yaw = 0.0f;
	client->pitch = 0.0f;
	client->spawned = false;
	client->num_extensions = 0;
	client->extensions = NULL;
}

void client_destroy(client_t *client) {
	closesocket(client->socket_fd);
	buffer_destroy(client->in_buffer);
	buffer_destroy(client->out_buffer);
}

void client_tick(client_t *client) {
	if (!client->connected) {
		return;
	}

	client_receive(client);

	if (client->spawned && server.tick - client->last_ping >= PING_INTERVAL) {
		buffer_write_uint8(client->out_buffer, packet_ping);
		client_flush(client);
		client->last_ping = server.tick;
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
					buffer_write_uint16be(client->out_buffer, server.map->height);
					buffer_write_uint16be(client->out_buffer, server.map->depth);
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

	while (buffer_tell(client->in_buffer) < (size_t)r) {
		uint8_t packet_id;
		buffer_read_uint8(client->in_buffer, &packet_id);

		switch (packet_id) {
			case packet_ident: {
				uint8_t protocol_version;
				char username[65];
				char key[65];
				uint8_t unused;

				buffer_read_uint8(client->in_buffer, &protocol_version);
				buffer_read_mcstr(client->in_buffer, username);
				buffer_read_mcstr(client->in_buffer, key);
				buffer_read_uint8(client->in_buffer, &unused);

				const bool supports_cpe = unused == 0x42;

				for (size_t i = 0; i < server.num_clients; i++) {
					if (strcasecmp(server.clients[i].name, username) == 0) {
						client_disconnect(client, "Name already in use.");
						return;
					}
				}

				strncpy(client->name, username, 64);

				if (supports_cpe) {
					buffer_write_uint8(client->out_buffer, packet_extinfo);
					buffer_write_mcstr(client->out_buffer, "classicserver", false);
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

				buffer_read_mcstr(client->in_buffer, appname);
				buffer_read_uint16be(client->in_buffer, &extcount);

				client->num_extensions = (size_t)extcount;
				client->extensions = calloc(extcount, sizeof(*client->extensions));

				printf("Client using %s with %d extensions\n", appname, extcount);

				break;
			}

			case packet_extentry: {
				char name[65];
				int32_t version;

				buffer_read_mcstr(client->in_buffer, name);
				buffer_read_int32be(client->in_buffer, &version);

				printf("Client supports %s v%d\n", name, version);

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

				buffer_read_uint16be(client->in_buffer, &x);
				buffer_read_uint16be(client->in_buffer, &y);
				buffer_read_uint16be(client->in_buffer, &z);
				buffer_read_uint8(client->in_buffer, &mode);
				buffer_read_uint8(client->in_buffer, &block);

				map_set(server.map, x, y, z, mode == 0x00 ? 0x00 : block);

				printf("%s changed block: x = %d, y = %d, z = %d, block = %d, mode = %d\n", client->name, x, y, z, block, mode);

				break;
			}

			case packet_message: {
				uint8_t unused;
				char msg[65];

				buffer_read_uint8(client->in_buffer, &unused);
				buffer_read_mcstr(client->in_buffer, msg);

				server_broadcast("&e%s: &f%s", client->name, msg);

				break;
			}

			case packet_player_pos_angle: {
				int8_t unused;
				int16_t x, y, z;
				int8_t yaw, pitch;

				buffer_read_int8(client->in_buffer, &unused);
				buffer_read_int16be(client->in_buffer, &x);
				buffer_read_int16be(client->in_buffer, &y);
				buffer_read_int16be(client->in_buffer, &z);
				buffer_read_int8(client->in_buffer, &yaw);
				buffer_read_int8(client->in_buffer, &pitch);

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

			default: {
				fprintf(stderr, "client %zu (%s) sent unknown packet 0x%02x\n", client->idx, client->name, packet_id);
				break;
			};
		}
	}
}

void client_login(client_t *client) {
	const bool cp437 = client_supports_extension(client, "FullCP437", 1);
	const bool fastmap = client_supports_extension(client, "FastMap", 1);

	buffer_write_uint8(client->out_buffer, packet_ident);
	buffer_write_uint8(client->out_buffer, 0x07);
	buffer_write_mcstr(client->out_buffer, "hostname", cp437);
	buffer_write_mcstr(client->out_buffer, "motd", cp437);
	buffer_write_uint8(client->out_buffer, 0x00);
	client_flush(client);

	if (fastmap) {
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

void client_flush(client_t *client) {
	if (!client->connected) {
		return;
	}

	int sendflags = 0;
#ifdef __linux__
	sendflags |= MSG_NOSIGNAL;
#endif

#ifdef _WIN32
	int r = send(client->socket_fd, (const char *)client->out_buffer->mem.data, (int)client->out_buffer->mem.offset, sendflags);
#else
	int r = send(client->socket_fd, client->out_buffer->mem.data, client->out_buffer->mem.offset, sendflags);
#endif
	buffer_seek(client->out_buffer, 0);

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
