#include <stddef.h>
#include <stdbool.h>
#include "cpe.h"

cpeext_t supported_extensions[] = {
		{ "FullCP437", 1 },
		{ "FastMap", 1 },
		{ "CustomBlocks", 1 },
		{ "TwoWayPing", 1 },

		{ "", 0 }
};

size_t cpe_count_supported(void) {
	for (size_t i = 0; true; i++) {
		if (supported_extensions[i].name[0] == '\0') {
			return i;
		}
	}
}
