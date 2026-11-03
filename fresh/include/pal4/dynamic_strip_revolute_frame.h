#pragma once

#include "pal4/dynamic_strip_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IDynamicStripRevoluteFrameRuntime {
public:
    virtual ~IDynamicStripRevoluteFrameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetFrame(
        const DynamicStripObjectState& state) = 0;
    virtual void TranslateMatrix(std::uint32_t matrix,
                                 const std::array<float, 3>& vector,
                                 std::uint32_t mode) = 0;
    virtual void RotateMatrix(std::uint32_t matrix,
                              const std::array<float, 3>& axis,
                              float angle, std::uint32_t mode) = 0;
    virtual void UpdateFrameObjects(std::uint32_t frame) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SetPosition(
        DynamicStripObjectState& state,
        const std::array<float, 3>& values,
        std::uint32_t mode) = 0;
};

class DynamicStripRevoluteFrameRuntimeStub final
    : public IDynamicStripRevoluteFrameRuntime {
public:
    [[nodiscard]] std::uint32_t GetFrame(
        const DynamicStripObjectState&) override { return 0u; }
    void TranslateMatrix(std::uint32_t, const std::array<float, 3>&,
                         std::uint32_t) override {}
    void RotateMatrix(std::uint32_t, const std::array<float, 3>&,
                      float, std::uint32_t) override {}
    void UpdateFrameObjects(std::uint32_t) override {}
    [[nodiscard]] float ReadFloat(std::uint32_t) override { return 0.0f; }
    [[nodiscard]] std::uint32_t SetPosition(
        DynamicStripObjectState&, const std::array<float, 3>&,
        std::uint32_t) override { return 0u; }
};

class DynamicStripRevoluteFrame final {
public:
    // IDA 0x60f5e0; facet +32, owner frame slot +32, position facet +28.
    [[nodiscard]] static std::uint32_t Apply(
        DynamicStripObjectState& state,
        const std::array<float, 3>& target,
        const std::array<float, 3>& rotation,
        const std::array<float, 3>& step,
        IDynamicStripRevoluteFrameRuntime& runtime);
};

}  // namespace pal4
