#pragma once
#include <array>
#include <cstdint>
namespace pal4 {
// Original66D150/basic material boundary. The config owns the temporary normal
// texture reference returned by generation; materials retain their own refs.
class NativeClumpNormalMap final {
public:
    static bool Apply(std::uint32_t clump,std::array<std::uint32_t,3>& config);
    static bool ApplyExtended(std::uint32_t clump,std::array<std::uint32_t,8>& config);
};
}
