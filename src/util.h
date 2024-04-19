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

#define util_max(x, y) (((x) > (y)) ? (x) : (y))
#define util_min(x, y) (((x) < (y)) ? (x) : (y))
#define util_clamp(value, minimum, maximum) (util_max(util_min((value), (minimum)), (maximum)))

#define util_float2fixed(value) ((int16_t)((value) * 32.0f))
#define util_fixed2float(value) ((float)(value) / 32.0f)

#define util_fixed2degrees(value) ((float)(((float)(value)) * 360.0f / 256.0f))
#define util_degrees2fixed(value) ((int8_t)(((float)(value)) / 360.0f * 256.0f))

double get_time_s(void);

typedef struct {
	char *key, *value;
} httpheader_t;

httpheader_t *util_httpheaders_parse(const char *text, size_t *num_headers);
const char *util_httpheaders_get(httpheader_t *headers, size_t num_headers, const char *key);
void util_httpheaders_destroy(httpheader_t *list, size_t num_headers);
