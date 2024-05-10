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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include "nbt.h"
#include "buffer.h"

static void write_len_str(buffer_t *buffer, const char *s);

static tag_t *tag_create_empty(void);
static const char *tag_get_type_name(tag_e type);
static int tag_get_actual_num_entries(tag_t *t);

static tag_e expected = tag_end;

tag_t *tag_create_empty(void) {
	tag_t *t = malloc(sizeof(tag_t));

	t->type = 0;
	t->name = NULL;
	t->no_header = true;
	t->array_size = 0;
	t->l = 0LL;

	return t;
}

tag_t *nbt_create(const char *name) {
	tag_t *t = tag_create_empty();

	if (name != NULL) {
		t->no_header = false;

		size_t nl = strlen(name);
		t->name = calloc(nl + 1, sizeof(char));
		memcpy(t->name, name, nl);
		t->name[nl] = '\0';
	}

	return t;
}

tag_t *nbt_create_compound(const char *name) {
	tag_t *t = nbt_create(name);
	t->type = tag_compound;
	t->array_size = 32;
	t->list = calloc(t->array_size, sizeof(*t->list));

	return t;
}

tag_t *nbt_create_list(const char *name, tag_e type, int32_t length) {
	tag_t *t = nbt_create(name);
	t->type = tag_list;
	t->array_type = type;
	t->array_size = length;
	t->list = calloc(t->array_size, sizeof(*t->list));

	return t;
}

tag_t *nbt_create_string(const char *name, const char *val){
	tag_t *t = nbt_create(name);
	t->type = tag_string;

	t->array_size = (int32_t) strlen(val);
	t->str = malloc(t->array_size + 1);
	memcpy(t->str, val, t->array_size);
	t->str[t->array_size] = '\0';

	return t;
}

tag_t *nbt_create_bytearray(const char *name, uint8_t *val, int32_t len) {
	tag_t *t = nbt_create(name);
	t->type = tag_byte_array;
	t->array_size = len;
	t->pb = val;

	return t;
}

tag_t *nbt_create_intarray(const char *name, int32_t *val, int32_t len) {
	tag_t *t = nbt_create(name);
	t->type = tag_int_array;
	t->array_size = len;
	t->pi = val;

	return t;
}

tag_t *nbt_create_longarray(const char *name, int64_t *val, int32_t len) {
	tag_t *t = nbt_create(name);
	t->type = tag_long_array;
	t->array_size = len;
	t->pl = val;

	return t;
}

tag_t *nbt_copy_bytearray(const char *name, uint8_t *val, int32_t len) {
	uint8_t *a = malloc(len);
	memcpy(a, val, len);

	return nbt_create_bytearray(name, a, len);
}

void nbt_destroy(tag_t *tag, bool destroy_values) {
	if (tag == NULL) {
		return;
	}

	if (tag->type == tag_list || tag->type == tag_compound) {
		for (int32_t i = 0; i < tag->array_size; i++) {
			tag_t *t = tag->list[i];

			if (t != NULL) {
				nbt_destroy(t, destroy_values);
			}
		}

		free(tag->list);
	}

	if (!tag->no_header) {
		if (tag->name != NULL) {
			free(tag->name);
			tag->name = NULL;
		}
	}

	if (destroy_values) {
		switch (tag->type) {
			case tag_string:        free(tag->str); break;
			case tag_byte_array:    free(tag->pb);  break;
			case tag_int_array:	    free(tag->pi);  break;
			case tag_long_array:    free(tag->pl);  break;
			default: break;
		}
	}

	free(tag);
}

void nbt_write(tag_t *tag, buffer_t *buffer) {
	if (!tag->no_header) {
		buffer_write_uint8(buffer, tag->type);

		if (tag->name != NULL) {
			write_len_str(buffer, tag->name);
		}
	}

	switch (tag->type) {
		case tag_end: {
			break;
		}

		case tag_byte: {
			buffer_write_uint8(buffer, tag->b);
			break;
		}

		case tag_short: {
			buffer_write_int16be(buffer, tag->s);
			break;
		}

		case tag_int: {
			buffer_write_int32be(buffer, tag->i);
			break;
		}

		case tag_long: {
			buffer_write_int64be(buffer, tag->l);
			break;
		}

		case tag_float: {
			buffer_write_floatbe(buffer, tag->f);
			break;
		}

		case tag_double: {
			buffer_write_doublebe(buffer, tag->f);
			break;
		}

		case tag_byte_array: {
			buffer_write_int32be(buffer, tag->array_size);
			buffer_write(buffer, tag->pb, tag->array_size);
			break;
		}

		case tag_string: {
			write_len_str(buffer, tag->str);
			break;
		}

		case tag_list: {
			buffer_write_uint8(buffer, (uint8_t) tag->array_type);
			buffer_write_int32be(buffer, tag->array_size);

			for (int32_t i = 0; i < tag->array_size; i++) {
				if (tag->list[i] == NULL) continue;

				nbt_write(tag->list[i], buffer);
			}

			break;
		}

		case tag_compound: {
			for (int32_t i = 0; i < tag->array_size; i++) {
				if (tag->list[i] == NULL) continue;

				nbt_write(tag->list[i], buffer);
			}

			buffer_write_uint8(buffer, tag_end);

			break;
		}

		case tag_int_array: {
			buffer_write_int32be(buffer, tag->array_size);
			for (int32_t i = 0; i < tag->array_size; i++) {
				buffer_write_int32be(buffer, tag->pi[i]);
			}
			break;
		}

		case tag_long_array: {
			buffer_write_int32be(buffer, tag->array_size);
			for (int32_t i = 0; i < tag->array_size; i++) {
				buffer_write_int64be(buffer, tag->pl[i]);
			}
			break;
		}

		default: {
			fprintf(stderr, "unknown tag type %d\n", tag->type);
			break;
		}
	}
}

void write_len_str(buffer_t *buffer, const char *s) {
	int16_t len = (int16_t) strlen(s);

	buffer_write_int16be(buffer, len);
	buffer_write(buffer, (void *)s, len);
}

void nbt_set_int8(tag_t *tag, int8_t b) {
	tag->type = tag_byte;
	tag->b = b;
}

void nbt_set_int16(tag_t *tag, int16_t b) {
	tag->type = tag_short;
	tag->s = b;
}

void nbt_set_int32(tag_t *tag, int32_t b) {
	tag->type = tag_int;
	tag->i = b;
}

void nbt_set_int64(tag_t *tag, int64_t b) {
	tag->type = tag_long;
	tag->l = b;
}

void nbt_add_tag(tag_t *tag, tag_t *new) {
	if ((tag->type != tag_list && tag->type != tag_compound) || tag->list == NULL) {
		return;
	}

	int32_t i;
	bool found = false;
	for (i = 0; i < tag->array_size; i++) {
		if (tag->list[i] == NULL) {
			found = true;
			break;
		}
	}

	if (!found) {
		printf("no room for new element in %p\n", (void *)tag);
		return;
	}

	tag->list[i] = new;
}

tag_t *nbt_read(buffer_t *buffer, bool named) {
	tag_t *t = tag_create_empty();

	if (named) {
		uint8_t type;
		buffer_read_uint8(buffer, &type);
		t->type = (tag_e)type;
		t->no_header = false;

		if (t->type != tag_end) {
			int16_t namelen;
			buffer_read_int16be(buffer, &namelen);
			t->name = malloc(namelen + 1);
			buffer_read(buffer, t->name, namelen);
			t->name[namelen] = 0;
		}
	} else {
		t->type = expected;
	}

	switch (t->type) {
		case tag_end: {
			break;
		}

		case tag_byte: {
			buffer_read_int8(buffer, &t->b);
			break;
		}

		case tag_short: {
			buffer_read_int16be(buffer, &t->s);
			break;
		}

		case tag_int: {
			buffer_read_int32be(buffer, &t->i);
			break;
		}

		case tag_long: {
			buffer_read_int64be(buffer, &t->l);
			break;
		}

		case tag_float: {
			buffer_read_floatbe(buffer, &t->f);
			break;
		}

		case tag_double: {
			buffer_read_doublebe(buffer, &t->d);
			break;
		}

		case tag_byte_array: {
			buffer_read_int32be(buffer, &t->array_size);
			t->pb = malloc(t->array_size);
			buffer_read(buffer, t->pb, t->array_size);

			break;
		}

		case tag_string: {
			int16_t size;
			buffer_read_int16be(buffer, &size);
			t->array_size = size;
			t->str = malloc(t->array_size);
			buffer_read(buffer, t->str, t->array_size);

			break;
		}

		case tag_list: {
			uint8_t type;
			buffer_read_uint8(buffer, &type);
			t->array_type = (tag_e) type;
			buffer_read_int32be(buffer, &t->array_size);
			expected = t->array_type;

			t->list = calloc(t->array_size, sizeof(*t->list));

			for (int32_t i = 0; i < t->array_size; i++) {
				t->list[i] = nbt_read(buffer, false);
			}

			break;
		}

		case tag_compound: {
			t->array_size = 0;
			tag_t *subtag;
			int i = 0;
			size_t off = buffer_tell(buffer);

			// count how many sub-tags we have 
			while ((subtag = nbt_read(buffer, true))->type != tag_end) {
				t->array_size++;
				nbt_destroy(subtag, true);
			}

			t->list = calloc(t->array_size, sizeof(*t->list));

			buffer_seek(buffer, off);

			while ((subtag = nbt_read(buffer, true))->type != tag_end) {
				t->list[i] = subtag;
				i++;
			}

			break;
		}

		case tag_int_array: {
			buffer_read_int32be(buffer, &t->array_size);
			t->pi = malloc(t->array_size * sizeof(int32_t));
			for (int32_t i = 0; i < t->array_size; i++) {
				buffer_read_int32be(buffer, &t->pi[i]);
			}

			break;
		}

		case tag_long_array: {
			buffer_read_int32be(buffer, &t->array_size);
			t->pl = malloc(t->array_size * sizeof(int64_t));
			for (int32_t i = 0; i < t->array_size; i++) {
				buffer_read_int32be(buffer, &t->pi[i]);
			}

			break;
		}

		default: {
			printf("attempt to read unknown tag type %d at position 0x%08zx\n", t->type, buffer_tell(buffer));

			nbt_destroy(t, true);

			return NULL;
		}
	}

	return t;
}

tag_t *nbt_get_tag(tag_t *tag, const char *n) {
	if (tag->type != tag_compound) {
		return NULL;
	}

	for (int32_t i = 0; i < tag->array_size; i++) {
		tag_t *t = tag->list[i];
		if (t == NULL) {
			continue;
		}

		if (strcmp(t->name, n) == 0) {
			return t;
		}
	}

	fprintf(stderr, "tag %s('%s') has no sub-tag %s\n", tag_get_type_name(tag->type), tag->name, n);

	return NULL;
}

void nbt_dump(tag_t *tag, int indent) {
	char *indentstr = malloc(indent + 1);
	for (int i = 0; i < indent; i++) {
		indentstr[i] = ' ';
	}
	indentstr[indent] = 0;

	printf("%s", indentstr);
	printf("%s", tag_get_type_name(tag->type));
	printf("(");

	if (tag->name == NULL) {
		printf("None");
	} else {
		printf("'%s'", tag->name);
	}

	printf("): ");

	switch (tag->type) {
		case tag_compound:
		case tag_list:;
			int actual_entries = tag_get_actual_num_entries(tag);
			printf("%d entr%s\n%s{\n", actual_entries, actual_entries == 1 ? "y" : "ies", indentstr);

			for (int32_t i = 0; i < tag->array_size; i++) {
				tag_t *sub = tag->list[i];
				if (sub == NULL) continue;
				nbt_dump(sub, indent + 4);
			}

			printf("%s}\n", indentstr);
			break;

		case tag_string:
			printf("'%s'\n", tag->str); break;

		case tag_byte: printf("%" PRId8 "\n", tag->b); break;
		case tag_short: printf("%" PRId16 "\n", tag->s); break;
		case tag_int: printf("%" PRId32 "\n", tag->i); break;
		case tag_long: printf("%" PRId64 "\n", tag->l); break;
		case tag_float: printf("%f\n", tag->f); break;
		case tag_double: printf("%f\n", tag->d); break;
		case tag_byte_array: printf("[%" PRId32 " bytes]\n", tag->array_size); break;
		case tag_int_array: printf("[%" PRId32 " ints]\n", tag->array_size); break;
		case tag_long_array: printf("[%" PRId32 " longs]\n", tag->array_size); break;

		default: printf("\n"); break;
	}

	free(indentstr);
}

const char *tag_get_type_name(tag_e type) {
	switch (type) {
		case tag_end:
			return "TAG_End";
		case tag_byte:
			return "TAG_Byte";
		case tag_short:
			return "TAG_Short";
		case tag_int:
			return "TAG_Int";
		case tag_long:
			return "TAG_Long";
		case tag_float:
			return "TAG_Float";
		case tag_double:
			return "TAG_Double";
		case tag_byte_array:
			return "TAG_Byte_Array";
		case tag_string:
			return "TAG_String";
		case tag_list:
			return "TAG_List";
		case tag_compound:
			return "TAG_Compound";
		case tag_int_array:
			return "TAG_Int_Array";
		case tag_long_array:
			return "TAG_Long_Array";
		default:
			return "Unknown";
	}
}

int tag_get_actual_num_entries(tag_t *t) {
	int n = 0;

	for (int32_t i = 0; i < t->array_size; i++) {
		if (t->list[i] != NULL) {
			n++;
		}
	}

	return n;
}
