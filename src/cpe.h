#pragma once

typedef struct {
	char name[65];
	int version;
} cpeext_t;

extern cpeext_t supported_extensions[];

size_t cpe_count_supported(void);

