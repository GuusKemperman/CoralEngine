#pragma once
#include <stdint.h>

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using intptr = intptr_t;
using uintptr = uintptr_t;

using float32 = float; static_assert(sizeof(float32) == 4);
using float64 = double; static_assert(sizeof(float64) == 8);