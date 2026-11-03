#pragma once

#include "pal4/effect_camera_object.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IEffectCameraConfigurationRuntime {
public:
    virtual ~IEffectCameraConfigurationRuntime() = default;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t document, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t document, std::string_view key, float fallback) = 0;
    [[nodiscard]] virtual float ReadTreeFloat(
        std::uint32_t document, std::string_view key, float fallback) = 0;
    [[nodiscard]] virtual std::uint32_t ReadTargetName(
        std::uint32_t document, std::string_view key,
        std::uint32_t fallback_address) = 0;
    virtual void AssignTargetName(EffectCameraObjectState& state,
                                  std::uint32_t temporary) = 0;
    virtual void ReleaseTargetName(std::uint32_t temporary) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrame() = 0;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    virtual void RotateFrame(std::uint32_t frame,
                             std::uint32_t axis_address, float angle,
                             std::uint32_t mode) = 0;
    virtual void UpdateFrameObjects(std::uint32_t frame) = 0;
    virtual void AssignXml(EffectCameraObjectState& state,
                           std::uint32_t document) = 0;
    [[nodiscard]] virtual std::uint32_t CachedDocument(
        EffectCameraObjectState& state) = 0;
    virtual void StartCameraFacet(EffectCameraObjectState& state,
                                  std::uint32_t facet_offset) = 0;
};

class EffectCameraConfigurationRuntimeStub final
    : public IEffectCameraConfigurationRuntime {
public:
    [[nodiscard]] std::int32_t ReadInt(
        std::uint32_t, std::string_view, std::int32_t fallback) override {
        return fallback;
    }
    [[nodiscard]] float ReadFloat(
        std::uint32_t, std::string_view, float fallback) override {
        return fallback;
    }
    [[nodiscard]] float ReadTreeFloat(
        std::uint32_t, std::string_view, float fallback) override {
        return fallback;
    }
    [[nodiscard]] std::uint32_t ReadTargetName(
        std::uint32_t, std::string_view, std::uint32_t) override { return 0u; }
    void AssignTargetName(EffectCameraObjectState&, std::uint32_t) override {}
    void ReleaseTargetName(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t CreateFrame() override { return 0u; }
    void SetFrameIdentity(std::uint32_t) override {}
    void RotateFrame(std::uint32_t, std::uint32_t, float,
                     std::uint32_t) override {}
    void UpdateFrameObjects(std::uint32_t) override {}
    void AssignXml(EffectCameraObjectState&, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t CachedDocument(
        EffectCameraObjectState&) override { return 0u; }
    void StartCameraFacet(EffectCameraObjectState&, std::uint32_t) override {}
};

class EffectCameraConfiguration final {
public:
    // IDA 0x6074b0. Native XML, string and RenderWare services are injected.
    [[nodiscard]] static bool Load(
        EffectCameraObjectState& state, std::uint32_t document,
        IEffectCameraConfigurationRuntime& runtime);
    // IDA 0x6078e0. Reuses the embedded XML document and existing frame.
    [[nodiscard]] static bool Reload(
        EffectCameraObjectState& state,
        IEffectCameraConfigurationRuntime& runtime);
};

}  // namespace pal4
