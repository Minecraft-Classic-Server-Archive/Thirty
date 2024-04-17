#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct buffer_s buffer_t;

typedef enum tag_e {
	tag_end,
	tag_byte,
	tag_short,
	tag_int,
	tag_long,
	tag_float,
	tag_double,
	tag_byte_array,
	tag_string,
	tag_list,
	tag_compound,

	num_tags

} tag_e;

typedef struct tag_s {
	char *name;
	tag_e type;

	union {
		int8_t b;
		int16_t s;
		int32_t i;
		int64_t l;
		float f;
		double d;
		uint8_t *pb;
		char *str;
		struct tag_s **list;
	};

	// for entries in lists
	bool no_header;

	// for byte_array, list, compound tags
	int32_t array_size;

	// for tag_list
	tag_e array_type;
} tag_t;

tag_t *nbt_create(const char *name);
tag_t *nbt_create_compound(const char *name);
tag_t *nbt_create_string(const char *name, const char *val);
tag_t *nbt_create_bytearray(const char *name, uint8_t *val, int32_t len);
tag_t *nbt_copy_bytearray(const char *name, uint8_t *val, int32_t len);

void nbt_destroy(tag_t *tag, bool destroy_values);

void nbt_set_int8(tag_t *tag, int8_t b);
void nbt_set_int16(tag_t *tag, int16_t b);
void nbt_set_int32(tag_t *tag, int32_t b);
void nbt_set_int64(tag_t *tag, int64_t b);
void nbt_add_tag(tag_t *tag, tag_t *new);

tag_t *nbt_get_tag(tag_t *tag, const char *n);

void nbt_write(tag_t *tag, buffer_t *buffer);
tag_t *nbt_read(buffer_t *buffer, bool named);

void nbt_dump(tag_t *tag, int indent);

