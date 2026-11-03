#pragma once

#include "pal4/effect_system_object_f.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IEffectSystemObjectFTransformRuntime {
public:
    virtual ~IEffectSystemObjectFTransformRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeTransform(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t GetFrame(
        EffectSystemObjectFState& state) = 0;
    virtual void TranslateMatrix(
        std::uint32_t matrix, const std::array<float, 3>& vector,
        std::uint32_t mode) = 0;
    virtual void RotateMatrix(std::uint32_t matrix,
                              std::uint32_t axis_address, float angle,
                              std::uint32_t mode) = 0;
    virtual void UpdateFrameObjects(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::array<float, 3> ReadFramePosition(
        std::uint32_t frame) = 0;
};

class EffectSystemObjectFTransformRuntimeStub final
    : public IEffectSystemObjectFTransformRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t InitializeTransform(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t GetFrame(
        EffectSystemObjectFState&) override { return 0u; }
    void TranslateMatrix(std::uint32_t,
                         const std::array<float, 3>&,
                         std::uint32_t) override {}
    void RotateMatrix(std::uint32_t, std::uint32_t,
                      float, std::uint32_t) override {}
    void UpdateFrameObjects(std::uint32_t) override {}
    [[nodiscard]] std::array<float, 3> ReadFramePosition(
        std::uint32_t) override { return {}; }
};

class EffectSystemObjectFTransform final {
public:
    // IDA 0x6060b0. Returns the second written float's bits when active.
    [[nodiscard]] static std::uint32_t SetScale(
        const EffectSystemObjectFState& state, float width, float height,
        bool relative, IEffectSystemObjectFTransformRuntime& runtime);
    // IDA 0x606120. Relative mode leaves the embedded pointer in EAX.
    [[nodiscard]] static std::uint32_t SetRotation(
        const EffectSystemObjectFState& state, float angle, bool relative,
        IEffectSystemObjectFTransformRuntime& runtime);
    // IDA 0x606150.
    [[nodiscard]] static std::uint32_t SetPosition(
        const EffectSystemObjectFState& state,
        const std::array<float, 3>& position, bool relative,
        IEffectSystemObjectFTransformRuntime& runtime);
    // IDA 0x6061f0. RenderWare frame and matrix operations are injected.
    [[nodiscard]] static std::uint32_t ApplyMotion(
        EffectSystemObjectFState& state,
        const std::array<float, 3>& target,
        const std::array<float, 3>& angles,
        const std::array<float, 3>& steps,
        IEffectSystemObjectFTransformRuntime& runtime);
    // IDA 0x606440. The +32 facet copies three raw coordinate words.
    [[nodiscard]] static std::uint8_t SetPositionWords(
        const EffectSystemObjectFState& state,
        const std::array<std::uint32_t, 3>& position,
        IEffectSystemObjectFTransformRuntime& runtime);
};

}  // namespace pal4
