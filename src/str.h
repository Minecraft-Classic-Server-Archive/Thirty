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
#include <stddef.h>
#include <stdbool.h>
#ifdef USE_ALLOCA_H
#include <alloca.h>
#endif

typedef struct str_s {
    size_t len;
    size_t capacity;
    char *data;
    bool stack;
} str_t;

void string_create(str_t *str);
void string_allocate(str_t *str, size_t capacity);
void string_destroy(str_t *str);
char *string_get_buffer(str_t *str);

void string_append(str_t *str, const str_t *other);
void string_appendl(str_t *str, const char *text);
void string_appendf(str_t *str, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

#define string_allocate_stack(str, _capacity) do { \
        (str)->len = 0; \
        (str)->capacity = (_capacity); \
        (str)->data = alloca((_capacity)); \
        (str)->stack = true; \
    } while (false)
