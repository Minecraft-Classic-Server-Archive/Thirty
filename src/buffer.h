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

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

typedef enum {
	buftype_memory,
	buftype_file
} buftype_t;

typedef struct buffer_s {
	buftype_t type;
	bool owned; // whether the buffer owns its underlying handle

	union {
		struct {
			bool grows;
			uint8_t *data;
			size_t size;
			size_t offset;
		} mem;

		struct {
			FILE *fp;
		} file;
	};
} buffer_t;

buffer_t *buffer_create_memory(uint8_t *data, size_t size);
buffer_t *buffer_allocate_memory(size_t size, bool grows);
buffer_t *buffer_create_file(FILE *fp);
buffer_t *buffer_open_file(const char *path, const char *mode);
void buffer_destroy(buffer_t *buffer);

size_t buffer_seek(buffer_t *buffer, size_t size);
size_t buffer_tell(buffer_t *buffer);
size_t buffer_size(buffer_t *buffer);
void buffer_resize(buffer_t *buffer, size_t newsize);

size_t buffer_read(buffer_t *buffer, void *data, size_t len);
size_t buffer_write(buffer_t *buffer, const void *data, size_t len);
size_t buffer_write_mc(buffer_t *buffer, const void *data, size_t len);

bool buffer_read_uint8(buffer_t *buffer, uint8_t *data);
bool buffer_read_int8(buffer_t *buffer, int8_t *data);
bool buffer_read_uint16le(buffer_t *buffer, uint16_t *data);
bool buffer_read_int16le(buffer_t *buffer, int16_t *data);
bool buffer_read_uint16be(buffer_t *buffer, uint16_t *data);
bool buffer_read_int16be(buffer_t *buffer, int16_t *data);
bool buffer_read_uint32le(buffer_t *buffer, uint32_t *data);
bool buffer_read_int32le(buffer_t *buffer, int32_t *data);
bool buffer_read_uint32be(buffer_t *buffer, uint32_t *data);
bool buffer_read_int32be(buffer_t *buffer, int32_t *data);
bool buffer_read_uint64le(buffer_t *buffer, uint64_t *data);
bool buffer_read_int64le(buffer_t *buffer, int64_t *data);
bool buffer_read_uint64be(buffer_t *buffer, uint64_t *data);
bool buffer_read_int64be(buffer_t *buffer, int64_t *data);
bool buffer_read_floatle(buffer_t *buffer, float *data);
bool buffer_read_floatbe(buffer_t *buffer, float *data);
bool buffer_read_doublele(buffer_t *buffer, double *data);
bool buffer_read_doublebe(buffer_t *buffer, double *data);

bool buffer_write_uint8(buffer_t *buffer, uint8_t data);
bool buffer_write_int8(buffer_t *buffer, int8_t data);
bool buffer_write_uint16le(buffer_t *buffer, uint16_t data);
bool buffer_write_int16le(buffer_t *buffer, int16_t data);
bool buffer_write_uint16be(buffer_t *buffer, uint16_t data);
bool buffer_write_int16be(buffer_t *buffer, int16_t data);
bool buffer_write_uint32le(buffer_t *buffer, uint32_t data);
bool buffer_write_int32le(buffer_t *buffer, int32_t data);
bool buffer_write_uint32be(buffer_t *buffer, uint32_t data);
bool buffer_write_int32be(buffer_t *buffer, int32_t data);
bool buffer_write_uint64le(buffer_t *buffer, uint64_t data);
bool buffer_write_int64le(buffer_t *buffer, int64_t data);
bool buffer_write_uint64be(buffer_t *buffer, uint64_t data);
bool buffer_write_int64be(buffer_t *buffer, int64_t data);
bool buffer_write_floatle(buffer_t *buffer, float data);
bool buffer_write_floatbe(buffer_t *buffer, float data);
bool buffer_write_doublele(buffer_t *buffer, double data);
bool buffer_write_doublebe(buffer_t *buffer, double data);

void buffer_read_mcstr(buffer_t *buffer, char data[65]);
void buffer_write_mcstr(buffer_t *buffer, const char *data, bool filter);
