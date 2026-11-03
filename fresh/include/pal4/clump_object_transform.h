#pragma once

#include "pal4/clump_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IClumpObjectTransformRuntime {
public:
    virtual ~IClumpObjectTransformRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Translate(
        std::uint32_t manager, const std::array<float, 3>& position,
        std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint8_t Rotate(
        std::uint32_t manager, const std::array<float, 3>& rotation,
        std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t Scale(
        std::uint32_t manager, const std::array<float, 3>& scale,
        std::uint32_t mode) = 0;
};

class ClumpObjectTransformRuntimeStub final : public IClumpObjectTransformRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Translate(
        std::uint32_t, const std::array<float, 3>&,
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t Rotate(
        std::uint32_t, const std::array<float, 3>&,
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Scale(
        std::uint32_t, const std::array<float, 3>&,
        std::uint32_t) override { return 0u; }
};

}  // namespace pal4
