#pragma once

#include "pal4/effect_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IEffectObjectTransformFacetRuntime {
public:
    virtual ~IEffectObjectTransformFacetRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UpdateFrame(
        EffectObjectState& state) = 0;
};

class EffectObjectTransformFacetRuntimeStub final
    : public IEffectObjectTransformFacetRuntime {
public:
    [[nodiscard]] std::uint32_t UpdateFrame(
        EffectObjectState&) override { return 0u; }
};

class EffectObjectTransformFacets final {
public:
    // IDA 0x610ba0, owner +20 facet.
    [[nodiscard]] static std::uint8_t GetScale(
        const EffectObjectState& state,
        std::array<std::uint32_t, 3u>& output);
    // IDA 0x610bd0/0x610c30/0x610c90; relative is native low byte.
    [[nodiscard]] static std::uint32_t SetScale(
        EffectObjectState& state, const std::array<float, 3u>& values,
        std::uint8_t relative, std::uint32_t entry_eax,
        IEffectObjectTransformFacetRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetRotation(
        EffectObjectState& state, const std::array<float, 3u>& values,
        std::uint8_t relative, std::uint32_t entry_eax,
        IEffectObjectTransformFacetRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetPosition(
        EffectObjectState& state, const std::array<float, 3u>& values,
        std::uint8_t relative, std::uint32_t entry_eax,
        IEffectObjectTransformFacetRuntime& runtime);
};

class IEffectObjectFrameRuntime {
public:
    virtual ~IEffectObjectFrameRuntime() = default;
    virtual void SetIdentity(std::uint32_t frame) = 0;
    virtual void Scale(std::uint32_t frame,
                       const std::array<std::uint32_t, 3u>& values,
                       std::uint32_t mode) = 0;
    virtual void Rotate(std::uint32_t frame,
                        const std::array<float, 3u>& axis,
                        float angle, std::uint32_t mode) = 0;
    virtual void Translate(std::uint32_t frame,
                           const std::array<std::uint32_t, 3u>& values,
                           std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateObjects(
        std::uint32_t frame) = 0;
};

class EffectObjectFrameRuntimeStub final : public IEffectObjectFrameRuntime {
public:
    void SetIdentity(std::uint32_t) override {}
    void Scale(std::uint32_t, const std::array<std::uint32_t, 3u>&,
               std::uint32_t) override {}
    void Rotate(std::uint32_t, const std::array<float, 3u>&,
                float, std::uint32_t) override {}
    void Translate(std::uint32_t, const std::array<std::uint32_t, 3u>&,
                   std::uint32_t) override {}
    [[nodiscard]] std::uint32_t UpdateObjects(
        std::uint32_t) override { return 0u; }
};

class EffectObjectFrame final {
public:
    // IDA 0x610f40.
    [[nodiscard]] static std::uint32_t Rebuild(
        const EffectObjectState& state, IEffectObjectFrameRuntime& runtime);
};

}  // namespace pal4
