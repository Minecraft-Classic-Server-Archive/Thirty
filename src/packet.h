#pragma once

enum {
	packet_ident = 0x00,
	packet_ping = 0x01,
	packet_level_init = 0x02,
	packet_level_chunk = 0x03,
	packet_level_finish = 0x04,
	packet_set_block_client = 0x05,
	packet_set_block_server = 0x06,
	packet_player_spawn = 0x07,
	packet_player_pos_angle = 0x08,
	packet_player_pos_angle_update = 0x09,
	packet_player_pos_update = 0xa,
	packet_player_angle_update = 0x0b,
	packet_player_despawn = 0x0c,
	packet_message = 0x0d,
	packet_player_disconnect = 0x0e,
	packet_player_set_type = 0x0f,

	packet_extinfo = 0x10,
	packet_extentry = 0x11
};
