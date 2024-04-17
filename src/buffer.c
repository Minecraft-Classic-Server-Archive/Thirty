// classicserver, a ClassiCube (Minecraft Classic) server
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
#include "buffer.h"
#include "util.h"
#include "endian.h"

buffer_t *buffer_create_memory(uint8_t *data, size_t size) {
	buffer_t *buffer = malloc(sizeof(*buffer));
	buffer->type = buftype_memory;
	buffer->owned = false;
	buffer->mem.data = data;
	buffer->mem.size = size;
	buffer->mem.offset = 0;

	return buffer;
}

buffer_t *buffer_allocate_memory(size_t size) {
	buffer_t *buffer = malloc(sizeof(*buffer));
	buffer->type = buftype_memory;
	buffer->owned = true;
	buffer->mem.data = malloc(size);
	buffer->mem.size = size;
	buffer->mem.offset = 0;

	memset(buffer->mem.data, 0, size);

	return buffer;
}

buffer_t *buffer_create_file(FILE *fp) {
	buffer_t *buffer = malloc(sizeof(*buffer));
	buffer->type = buftype_file;
	buffer->owned = false;
	buffer->file.fp = fp;

	return buffer;
}

buffer_t *buffer_open_file(const char *path, const char *mode) {
	FILE *fp = fopen(path, mode);
	if (fp == NULL) {
		return NULL;
	}

	buffer_t *buffer = malloc(sizeof(*buffer));
	buffer->type = buftype_file;
	buffer->owned = true;
	buffer->file.fp = fp;

	return buffer;
}

void buffer_destroy(buffer_t *buffer) {
	if (buffer->owned) {
		switch (buffer->type) {
			case buftype_memory: {
				free(buffer->mem.data);
				break;
			}

			case buftype_file: {
				fclose(buffer->file.fp);
				break;
			}

			default: break;
		}
	}

	free(buffer);
}

size_t buffer_seek(buffer_t *buffer, size_t offset) {
	switch (buffer->type) {
		case buftype_memory: {
			return buffer->mem.offset = util_min(offset, buffer->mem.size);
		}

		case buftype_file: {
			return (size_t) fseek(buffer->file.fp, offset, SEEK_SET);
		}

		default: return 0;
	}
}

size_t buffer_tell(buffer_t *buffer) {
	switch (buffer->type) {
		case buftype_memory: {
			return buffer->mem.offset;
		}

		case buftype_file: {
			return (size_t) ftell(buffer->file.fp);
		}

		default: return 0;
	}
}

size_t buffer_size(buffer_t *buffer) {
	switch (buffer->type) {
		case buftype_memory: {
			return buffer->mem.size;
		}

		case buftype_file: {
			long p = ftell(buffer->file.fp);
			long size = fseek(buffer->file.fp, 0, SEEK_END);
			fseek(buffer->file.fp, p, SEEK_SET);
			return size;
		}

		default: return 0;
	}
}


size_t buffer_read(buffer_t *buffer, void *data, const size_t len) {
	switch (buffer->type) {
		case buftype_memory: {
			size_t read = util_min(len, buffer->mem.size - buffer->mem.offset);
			memcpy(data, buffer->mem.data + buffer->mem.offset, read);
			buffer->mem.offset += read;
			return read;
		}

		case buftype_file: {
			return fread(data, 1, len, buffer->file.fp);
		}

		default: return 0;
	}
}

size_t buffer_write(buffer_t *buffer, const void *data, const size_t len) {
	switch (buffer->type) {
		case buftype_memory: {
			size_t written = util_min(len, buffer->mem.size - buffer->mem.offset);
			memcpy(buffer->mem.data + buffer->mem.offset, data, written);
			buffer->mem.offset += written;
			return written;
		}

		case buftype_file: {
			return fwrite(data, 1, len, buffer->file.fp);
		}

		default: return 0;
	}
}

size_t buffer_write_mc(buffer_t *buffer, const void *data, const size_t len) {
	buffer_write(buffer, data, util_min(len, 1024));
	for (size_t i = len ; i < 1024; i++) {
		buffer_write_uint8(buffer, 0x00);
	}

	return 1024;
}

#define CHECK_SIZE()                                                \
	if (buffer_tell(buffer) + sizeof(c) > buffer_size(buffer)) {    \
		return false;                                               \
	}

bool buffer_read_uint8(buffer_t *buffer, uint8_t *data) {
	uint8_t c; CHECK_SIZE();
	buffer_read(buffer, &c, sizeof(c));
	*data = c;
	return true;
}

bool buffer_read_int8(buffer_t *buffer, int8_t *data) {
	int8_t c; CHECK_SIZE();
	buffer_read(buffer, &c, sizeof(c));
	*data = c;
	return true;
}

#define GENERIC_READ(function_name, data_type, swap_function)	    \
	bool function_name (buffer_t *buffer, data_type *data) {        \
		data_type c = (data_type)0; CHECK_SIZE();                   \
		buffer_read(buffer, &c, sizeof(c));                         \
		*data = swap_function(c);                                   \
		return true;                                                \
	}

GENERIC_READ(buffer_read_uint16le, uint16_t, endian_tolittle16)
GENERIC_READ(buffer_read_int16le,   int16_t, endian_tolittle16)
GENERIC_READ(buffer_read_uint16be, uint16_t, endian_tobig16)
GENERIC_READ(buffer_read_int16be,   int16_t, endian_tobig16)
GENERIC_READ(buffer_read_uint32le, uint32_t, endian_tolittle32)
GENERIC_READ(buffer_read_int32le,   int32_t, endian_tolittle32)
GENERIC_READ(buffer_read_uint32be, uint32_t, endian_tobig32)
GENERIC_READ(buffer_read_int32be,   int32_t, endian_tobig32)
GENERIC_READ(buffer_read_uint64le, uint64_t, endian_tolittle64)
GENERIC_READ(buffer_read_int64le,   int64_t, endian_tolittle64)
GENERIC_READ(buffer_read_uint64be, uint64_t, endian_tobig64)
GENERIC_READ(buffer_read_int64be,   int64_t, endian_tobig64)
GENERIC_READ(buffer_read_floatle,     float, endian_tolittlef)
GENERIC_READ(buffer_read_floatbe,     float, endian_tobigf)
GENERIC_READ(buffer_read_doublele,   double, endian_tolittled)
GENERIC_READ(buffer_read_doublebe,   double, endian_tobigd)

#define GENERIC_WRITE(function_name, data_type, swap_function)      \
	bool function_name (buffer_t *buffer, const data_type data) {   \
		const data_type c = (data_type)swap_function(data);         \
		CHECK_SIZE();                                               \
		buffer_write(buffer, &c, sizeof(c));                        \
		return true;                                                \
	}

bool buffer_write_uint8(buffer_t *buffer, const uint8_t data) {
	const uint8_t c = data; CHECK_SIZE();
	buffer_write(buffer, &c, sizeof(c));
	return true;
}

bool buffer_write_int8(buffer_t *buffer, const int8_t data) {
	const int8_t c = data; CHECK_SIZE();
	buffer_write(buffer, &c, sizeof(c));
	return true;
}

GENERIC_WRITE(buffer_write_uint16le, uint16_t, endian_tolittle16)
GENERIC_WRITE(buffer_write_int16le,   int16_t, endian_tolittle16)
GENERIC_WRITE(buffer_write_uint16be, uint16_t, endian_tobig16)
GENERIC_WRITE(buffer_write_int16be,   int16_t, endian_tobig16)
GENERIC_WRITE(buffer_write_uint32le, uint32_t, endian_tolittle32)
GENERIC_WRITE(buffer_write_int32le,   int32_t, endian_tolittle32)
GENERIC_WRITE(buffer_write_uint32be, uint32_t, endian_tobig32)
GENERIC_WRITE(buffer_write_int32be,   int32_t, endian_tobig32)
GENERIC_WRITE(buffer_write_uint64le, uint64_t, endian_tolittle64)
GENERIC_WRITE(buffer_write_int64le,   int64_t, endian_tolittle64)
GENERIC_WRITE(buffer_write_uint64be, uint64_t, endian_tobig64)
GENERIC_WRITE(buffer_write_int64be,   int64_t, endian_tobig64)
GENERIC_WRITE(buffer_write_floatle,     float, endian_tolittlef)
GENERIC_WRITE(buffer_write_floatbe,     float, endian_tobigf)
GENERIC_WRITE(buffer_write_doublele,   double, endian_tolittled)
GENERIC_WRITE(buffer_write_doublebe,   double, endian_tobigd)

void buffer_read_mcstr(buffer_t *buffer, char data[65]) {
	buffer_read(buffer, data, 64);
	int end = 63;

	for (; end >= 0;) {
		if (data[end - 1] != ' ') {
			break;
		}
		end--;
	}

	data[end] = '\0';
}

void buffer_write_mcstr(buffer_t *buffer, const char *data, bool filter) {
	char s[64];
	memset(s, ' ', 64);

	const size_t sl = strlen(data);
	memcpy(s, data, util_min(sl, 64));

	if (filter) {
		for (size_t i = 0; i < 64; i++) {
			if (s[i] < 0) {
				s[i] = '?';
			}
		}
	}

	buffer_write(buffer, s, 64);
}
