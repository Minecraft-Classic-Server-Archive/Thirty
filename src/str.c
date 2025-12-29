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
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "str.h"
#include "util.h"

static void string_expand(str_t *str, size_t wanted_size);

void string_create(str_t *str) {
    str->len = 0;
    str->capacity = 0;
    str->data = NULL;
    str->stack = false;
}

void string_allocate(str_t *str, size_t capacity) {
    string_create(str);
    string_expand(str, capacity + 1);
}

void string_destroy(str_t *str) {
    str->len = 0;

    if (!str->stack) {
        free(str->data);
        str->capacity = 0;
        str->data = NULL;
    }
}

char *string_get_buffer(str_t *str) {
    return str->data;
}

void string_expand(str_t *str, size_t wanted_size) {
    if (str->stack || str->capacity >= wanted_size) {
        return;
    }

    size_t newsize = (size_t)ceil((double)wanted_size / 16.0) * 16;
    char *newptr = realloc(str->data, newsize);

    if (!newptr) {
        return;
    }

    str->data = newptr;
    str->capacity = newsize;

    memset(str->data + str->len, 0, newsize - str->len);
}

void string_append(str_t *str, const str_t *other) {
    if (other == NULL || other->data == NULL || other->len == 0) {
        return;
    }

    string_expand(str, str->len + other->len + 1);
    memcpy(str->data + str->len, other->data, util_min(other->len, str->capacity - other->len - 1));
    str->len += other->len;
}

void string_appendl(str_t *str, const char *text) {
    if (text == NULL) {
        return;
    }

    size_t copylen = strlen(text);
    string_expand(str, str->len + copylen + 1);
    memcpy(str->data + str->len, text, util_min(copylen, str->capacity - copylen - 1));
    str->len += copylen;
}

void string_appendf(str_t *str, const char *fmt, ...) {
    char buffer[2048];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    string_appendl(str, buffer);
}