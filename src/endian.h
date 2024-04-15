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

static inline uint16_t endian_tobig16(uint16_t x) {
#ifdef ENDIAN_LITTLE
	return endian_swap64(x);
#else
	return x;
#endif
}

static inline uint32_t endian_tobig32(uint32_t x) {
#ifdef ENDIAN_LITTLE
	return endian_swap64(x);
	return x;
#else
	return x;
#endif
}

static inline uint64_t endian_tobig64(uint64_t x) {
#ifdef ENDIAN_LITTLE
	return endian_swap64(x);
	return x;
#else
	return x;
#endif
}