#pragma once

#include <cstdint>

namespace pal4 {

class IGameObjectFrameEffectRuntime {
public:
    virtual ~IGameObjectFrameEffectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t RenderTransform(double elapsed) = 0;
    [[nodiscard]] virtual std::uint32_t DrawEffect(
        std::uint32_t effect, std::uint32_t register_b,
        std::uint32_t register_si, std::uint32_t first_argument,
        std::uint32_t second_argument, std::uint32_t third_argument,
        std::uint32_t fourth_argument) = 0;
};

class GameObjectFrameEffects final {
public:
    [[nodiscard]] static std::int32_t RenderTransform(
        float elapsed, IGameObjectFrameEffectRuntime& runtime);
    [[nodiscard]] static std::uint8_t Draw(
        std::uint32_t effect, std::uint32_t register_b,
        std::uint32_t register_si, std::uint32_t first_argument_address,
        std::uint32_t second_argument, std::uint32_t third_argument,
        std::uint32_t fourth_argument,
        IGameObjectFrameEffectRuntime& runtime);
};

}  // namespace pal4
