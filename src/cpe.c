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
