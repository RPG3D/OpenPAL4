#pragma once
#include <array>
#include <cstdint>

namespace pal4 {
class NativeClumpEnvironmentMap final {
public:
    // Original config: frame, texture, coefficient float bits. Borrows inputs;
    // SDK material effects retain their own texture references.
    static bool Apply(std::uint32_t clump,std::array<std::uint32_t,3u>& config);
    // Clears MatFX materials and restores the SDK default atomic pipeline.
    static void Reset(std::uint32_t clump);
};
}  // namespace pal4
