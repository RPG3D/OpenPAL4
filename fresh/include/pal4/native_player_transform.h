#pragma once
#include <array>
#include <cstdint>
namespace pal4 {
class IGameResourceManagerRuntime;
class NativePlayerTransform final {
public:
    static void Apply(std::uint32_t player, std::uint32_t virtual_slot,
        const std::array<std::uint32_t, 3>& bits, std::uint32_t option,
        IGameResourceManagerRuntime& runtime);
};
}
