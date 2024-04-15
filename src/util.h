#pragma once

#define util_max(x, y) (((x) > (y)) ? (x) : (y))
#define util_min(x, y) (((x) < (y)) ? (x) : (y))
#define util_clamp(value, minimum, maximum) (util_max(util_min((value), (minimum)), (maximum)))