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
buffer_t *buffer_allocate_memory(size_t size);
buffer_t *buffer_create_file(FILE *fp);
buffer_t *buffer_open_file(const char *path, const char *mode);
void buffer_destroy(buffer_t *buffer);

size_t buffer_seek(buffer_t *buffer, size_t size);
size_t buffer_tell(buffer_t *buffer);
size_t buffer_size(buffer_t *buffer);

size_t buffer_read(buffer_t *buffer, void *data, size_t len);
size_t buffer_write(buffer_t *buffer, const void *data, size_t len);

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

void buffer_read_mcstr(buffer_t *buffer, char data[65]);
void buffer_write_mcstr(buffer_t *buffer, const char *data);