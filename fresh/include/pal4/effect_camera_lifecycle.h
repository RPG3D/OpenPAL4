#pragma once

#include "pal4/effect_camera_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IEffectCameraLifecycleRuntime {
public:
    virtual ~IEffectCameraLifecycleRuntime() = default;
    virtual void StopFacet(EffectCameraObjectState& state,
                           std::uint32_t facet_offset) = 0;
    virtual void ProcessEvent2(EffectCameraObjectState& state) = 0;
    [[nodiscard]] virtual std::uint32_t Finalize(
        EffectCameraObjectState& state, std::uint32_t flag) = 0;
    virtual void DestroyFrame(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint8_t UpdateTransform(
        EffectCameraObjectState& state) = 0;
    [[nodiscard]] virtual bool ContextReady(
        EffectCameraObjectState& state) = 0;
    [[nodiscard]] virtual bool HasTargetObject(
        EffectCameraObjectState& state) = 0;
    [[nodiscard]] virtual std::uint32_t TargetFrame(
        EffectCameraObjectState& state) = 0;
    [[nodiscard]] virtual std::uint32_t CloneHierarchy(
        std::uint32_t frame) = 0;
    virtual void ProcessEvent(EffectCameraObjectState& state) = 0;
    virtual void TranslateFrame(std::uint32_t frame,
                                const std::array<std::uint32_t, 3>& position,
                                std::uint32_t mode) = 0;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectTransform(
        std::uint32_t frame) = 0;
    virtual void ApplyTransform(std::uint32_t target_frame,
                                std::uint32_t transform,
                                std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint8_t UpdateFrameObjects(
        std::uint32_t frame) = 0;
};

class EffectCameraLifecycleRuntimeStub final
    : public IEffectCameraLifecycleRuntime {
public:
    void StopFacet(EffectCameraObjectState&, std::uint32_t) override {}
    void ProcessEvent2(EffectCameraObjectState&) override {}
    [[nodiscard]] std::uint32_t Finalize(
        EffectCameraObjectState&, std::uint32_t) override { return 0u; }
    void DestroyFrame(std::uint32_t) override {}
    [[nodiscard]] std::uint8_t UpdateTransform(
        EffectCameraObjectState&) override { return 0u; }
    [[nodiscard]] bool ContextReady(
        EffectCameraObjectState&) override { return true; }
    [[nodiscard]] bool HasTargetObject(
        EffectCameraObjectState&) override { return false; }
    [[nodiscard]] std::uint32_t TargetFrame(
        EffectCameraObjectState&) override { return 0u; }
    [[nodiscard]] std::uint32_t CloneHierarchy(
        std::uint32_t) override { return 0u; }
    void ProcessEvent(EffectCameraObjectState&) override {}
    void TranslateFrame(std::uint32_t,
                        const std::array<std::uint32_t, 3>&,
                        std::uint32_t) override {}
    void SetFrameIdentity(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ObjectTransform(
        std::uint32_t) override { return 0u; }
    void ApplyTransform(std::uint32_t, std::uint32_t,
                        std::uint32_t) override {}
    [[nodiscard]] std::uint8_t UpdateFrameObjects(
        std::uint32_t) override { return 0u; }
};

class EffectCameraLifecycle final {
public:
    // IDA 0x607d40. Final virtual dispatch occurs even when inactive.
    [[nodiscard]] static std::uint32_t Stop(
        EffectCameraObjectState& state,
        IEffectCameraLifecycleRuntime& runtime);
    // IDA 0x607e00.
    [[nodiscard]] static std::uint8_t Update(
        EffectCameraObjectState& state,
        IEffectCameraLifecycleRuntime& runtime);
    // IDA 0x607e80/0x607f00 use the owner facet at +24.
    [[nodiscard]] static std::uint8_t Start(
        EffectCameraObjectState& state,
        IEffectCameraLifecycleRuntime& runtime);
    static void Pause(EffectCameraObjectState& state,
                      IEffectCameraLifecycleRuntime& runtime);
};

}  // namespace pal4
