#pragma once

#include "pal4/effect_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IEffectObjectRevoluteFrameRuntime {
public:
    virtual ~IEffectObjectRevoluteFrameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetFrame(
        EffectObjectState& state) = 0;
    virtual void TranslateMatrix(std::uint32_t matrix,
                                 const std::array<float, 3u>& vector,
                                 std::uint32_t mode) = 0;
    virtual void RotateMatrix(std::uint32_t matrix,
                              const std::array<float, 3u>& axis,
                              float angle, std::uint32_t mode) = 0;
    virtual void UpdateFrameObjects(std::uint32_t frame) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SetPosition(
        EffectObjectState& state, const std::array<float, 3u>& values,
        std::uint32_t mode) = 0;
};

class EffectObjectRevoluteFrameRuntimeStub final
    : public IEffectObjectRevoluteFrameRuntime {
public:
    [[nodiscard]] std::uint32_t GetFrame(
        EffectObjectState&) override { return 0u; }
    void TranslateMatrix(std::uint32_t, const std::array<float, 3u>&,
                         std::uint32_t) override {}
    void RotateMatrix(std::uint32_t, const std::array<float, 3u>&,
                      float, std::uint32_t) override {}
    void UpdateFrameObjects(std::uint32_t) override {}
    [[nodiscard]] float ReadFloat(std::uint32_t) override { return 0.0f; }
    [[nodiscard]] std::uint32_t SetPosition(
        EffectObjectState&, const std::array<float, 3u>&,
        std::uint32_t) override { return 0u; }
};

class EffectObjectRevoluteFrame final {
public:
    // IDA 0x610cf0; entered through the owner +32 facet.
    [[nodiscard]] static std::uint32_t Apply(
        EffectObjectState& state, const std::array<float, 3u>& target,
        const std::array<float, 3u>& rotation,
        const std::array<float, 3u>& step, std::uint32_t entry_eax,
        IEffectObjectRevoluteFrameRuntime& runtime);
};

}  // namespace pal4
