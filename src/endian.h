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

#define ENDIAN_LITTLE

static inline uint16_t endian_swap16(uint16_t x) {
	return __builtin_bswap16(x);
}

static inline uint32_t endian_swap32(uint32_t x) {
	return __builtin_bswap32(x);
}

static inline uint64_t endian_swap64(uint64_t x) {
	return __builtin_bswap64(x);
}

static inline float endian_swapf(float x) {
	union { float f; uint32_t i; } temp;
	temp.f = x;
	temp.i = endian_swap32(temp.i);
	return temp.f;
}

static inline double endian_swapd(double x) {
	union { double f; uint64_t i; } temp;
	temp.f = x;
	temp.i = endian_swap64(temp.i);
	return temp.f;
}

static inline uint16_t endian_tolittle16(uint16_t x) {
#ifdef ENDIAN_LITTLE
	return x;
#else
	return endian_swap16(x);
#endif
}

static inline uint32_t endian_tolittle32(uint32_t x) {
#ifdef ENDIAN_LITTLE
	return x;
#else
	return endian_swap32(x);
#endif
}

static inline uint64_t endian_tolittle64(uint64_t x) {
#ifdef ENDIAN_LITTLE
	return x;
#else
	return endian_swap64(x);
#endif
}

static inline float endian_tolittlef(float x) {
#ifdef ENDIAN_LITTLE
	return x;
#else
	return endian_swapf(x);
#endif
}

static inline double endian_tolittled(double x) {
#ifdef ENDIAN_LITTLE
	return x;
#else
	return endian_swapd(x);
#endif
}

static inline uint16_t endian_tobig16(uint16_t x) {
#ifdef ENDIAN_LITTLE
	return endian_swap16(x);
#else
	return x;
#endif
}

static inline uint32_t endian_tobig32(uint32_t x) {
#ifdef ENDIAN_LITTLE
	return endian_swap32(x);
#else
	return x;
#endif
}

static inline uint64_t endian_tobig64(uint64_t x) {
#ifdef ENDIAN_LITTLE
	return endian_swap64(x);
#else
	return x;
#endif
}

static inline float endian_tobigf(float x) {
#ifdef ENDIAN_LITTLE
	return endian_swapf(x);
#else
	return x;
#endif
}

static inline double endian_tobigd(double x) {
#ifdef ENDIAN_LITTLE
	return endian_swapd(x);
#else
	return x;
#endif
}
