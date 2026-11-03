#pragma once

#include "pal4/cylinder_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class ICylinderObjectFrameRuntime {
public:
    virtual ~ICylinderObjectFrameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetFrame(
        const CylinderObjectState& state) = 0;
    virtual void TranslateMatrix(std::uint32_t matrix,
                                 const std::array<float, 3>& vector,
                                 std::uint32_t mode) = 0;
    virtual void RotateMatrix(std::uint32_t matrix,
                              const std::array<float, 3>& axis,
                              float angle, std::uint32_t mode) = 0;
    virtual void UpdateFrameObjects(std::uint32_t frame) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SetPosition(
        std::uint32_t model, const std::array<float, 3>& position,
        std::uint32_t mode) = 0;
};

class CylinderObjectFrameRuntimeStub final : public ICylinderObjectFrameRuntime {
public:
    [[nodiscard]] std::uint32_t GetFrame(const CylinderObjectState&) override {
        return 0u;
    }
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

class CylinderObjectFrame final {
public:
    // IDA 0x60d690, object +36 facet and owner virtual slot +32.
    [[nodiscard]] static std::uint32_t Apply(
        const CylinderObjectState& state,
        const std::array<float, 3>& target,
        const std::array<float, 3>& rotation,
        const std::array<float, 3>& step,
        ICylinderObjectFrameRuntime& runtime);
};

}  // namespace pal4
