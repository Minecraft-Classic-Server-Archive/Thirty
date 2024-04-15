#pragma once

#define util_max(x, y) (((x) > (y)) ? (x) : (y))
#define util_min(x, y) (((x) < (y)) ? (x) : (y))
#define util_clamp(value, minimum, maximum) (util_max(util_min((value), (minimum)), (maximum)))

#define util_float2fixed(value) ((int16_t)((value) * 32.0f))
#define util_fixed2float(value) ((float)(value) / 32.0f)

#define util_fixed2degrees(value) ((float)(((float)(value)) * 360.0f / 256.0f))
#define util_degrees2fixed(value) ((int8_t)(((float)(value)) / 360.0f * 256.0f))