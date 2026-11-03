#pragma once
#include <array>
#include <cstdint>
namespace pal4 {
// Common 0x34-byte archive header used by the original Save/Load row builders.
struct SaveSlotData { std::array<std::uint32_t, 13> words{}; };
}
