#pragma once

#include "pal4/thunder_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IThunderPropertyRuntime {
public:
    virtual ~IThunderPropertyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UpdateFrame(
        ThunderObjectState& state) = 0;
};

class ThunderProperties final {
public:
    // IDA 0x61c590/0x61c5c0/0x61c5f0.
    [[nodiscard]] static std::uint8_t GetRotation(
        const ThunderObjectState& state,
        std::array<std::uint32_t, 3u>& output);
    [[nodiscard]] static std::uint8_t GetScale(
        const ThunderObjectState& state,
        std::array<std::uint32_t, 3u>& output);
    [[nodiscard]] static std::uint8_t GetPosition(
        const ThunderObjectState& state,
        std::array<std::uint32_t, 3u>& output);

    // IDA 0x61c620/0x61c6e0/0x61c7a0.
    [[nodiscard]] static std::uint32_t SetScale(
        ThunderObjectState& state, const std::array<float, 3u>& values,
        bool relative, IThunderPropertyRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetRotation(
        ThunderObjectState& state, const std::array<float, 3u>& values,
        bool relative, IThunderPropertyRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetPosition(
        ThunderObjectState& state, const std::array<float, 3u>& values,
        bool relative, IThunderPropertyRuntime& runtime);
};

}  // namespace pal4
