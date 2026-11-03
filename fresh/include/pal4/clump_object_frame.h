#pragma once

#include "pal4/clump_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IClumpObjectFrameRuntime {
public:
    virtual ~IClumpObjectFrameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetFrame(const ClumpObjectState& state) = 0;
    virtual void TranslateMatrix(std::uint32_t matrix,
                                 const std::array<float, 3>& vector,
                                 std::uint32_t mode) = 0;
    virtual void RotateMatrix(std::uint32_t matrix,
                              const std::array<float, 3>& axis,
                              float angle, std::uint32_t mode) = 0;
    virtual void UpdateFrameObjects(std::uint32_t frame) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SetPosition(
        std::uint32_t manager, const std::array<float, 3>& position,
        std::uint32_t mode) = 0;
};

class ClumpObjectFrameRuntimeStub final : public IClumpObjectFrameRuntime {
public:
    [[nodiscard]] std::uint32_t GetFrame(const ClumpObjectState&) override { return 0u; }
    void TranslateMatrix(std::uint32_t, const std::array<float, 3>&,
                         std::uint32_t) override {}
    void RotateMatrix(std::uint32_t, const std::array<float, 3>&,
                      float, std::uint32_t) override {}
    void UpdateFrameObjects(std::uint32_t) override {}
    [[nodiscard]] float ReadFloat(std::uint32_t) override { return 0.0f; }
    [[nodiscard]] std::uint32_t SetPosition(
        std::uint32_t, const std::array<float, 3>&,
        std::uint32_t) override { return 0u; }
};

}  // namespace pal4
