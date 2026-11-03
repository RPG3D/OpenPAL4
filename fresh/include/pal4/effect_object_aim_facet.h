#pragma once

#include "pal4/effect_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IEffectObjectAimFacetRuntime {
public:
    virtual ~IEffectObjectAimFacetRuntime() = default;
    [[nodiscard]] virtual std::uint8_t GetPosition(
        const EffectObjectState& state,
        std::array<std::uint32_t, 3u>& output) = 0;
    [[nodiscard]] virtual std::uint8_t SetPosition(
        EffectObjectState& state,
        const std::array<std::uint32_t, 3u>& values,
        std::uint32_t mode) = 0;
};

class EffectObjectAimFacetRuntimeStub final : public IEffectObjectAimFacetRuntime {
public:
    [[nodiscard]] std::uint8_t GetPosition(
        const EffectObjectState&,
        std::array<std::uint32_t, 3u>&) override { return 0u; }
    [[nodiscard]] std::uint8_t SetPosition(
        EffectObjectState&, const std::array<std::uint32_t, 3u>&,
        std::uint32_t) override { return 0u; }
};

class EffectObjectAimFacet final {
public:
    // IDA 0x6110e0, owner +48 facet to owner +28 virtual slot +4.
    [[nodiscard]] static std::uint8_t GetPosition(
        const EffectObjectState& state,
        std::array<std::uint32_t, 3u>& output,
        IEffectObjectAimFacetRuntime& runtime);
    // IDA 0x611410, owner +48 facet to owner +28 virtual slot +8.
    [[nodiscard]] static std::uint8_t SetPositionWithMode(
        EffectObjectState& state,
        const std::array<std::uint32_t, 3u>& values,
        std::uint32_t mode,
        IEffectObjectAimFacetRuntime& runtime);
    // IDA 0x611100/0x611140.
    [[nodiscard]] static std::uint8_t SetTarget(
        EffectObjectState& state,
        const std::array<std::uint32_t, 3u>& values);
    [[nodiscard]] static std::uint8_t SetRotationOffset(
        EffectObjectState& state,
        const std::array<std::uint32_t, 3u>& values);
    // IDA 0x611160.
    [[nodiscard]] static std::uint8_t UpdateFrame(
        EffectObjectState& state, std::uint8_t enabled,
        class IEffectObjectAimRuntime& runtime);
};

struct EffectObjectAimMatrix final {
    std::array<float, 3u> right{};
    std::uint32_t flags = 0x20003u;
    std::array<float, 3u> up{};
    std::uint32_t pad1 = 0u;
    std::array<float, 3u> at{};
    std::uint32_t pad2 = 0u;
    std::array<float, 3u> position{};
    std::uint32_t pad3 = 0u;
};

static_assert(sizeof(EffectObjectAimMatrix) == 64u);

class IEffectObjectAimRuntime {
public:
    virtual ~IEffectObjectAimRuntime() = default;
    virtual void GetPosition(const EffectObjectState& state,
                             std::array<std::uint32_t, 3u>& output) = 0;
    virtual void Normalize(std::array<float, 3u>& vector) = 0;
    virtual void UpdateMatrix(EffectObjectAimMatrix& matrix) = 0;
    [[nodiscard]] virtual std::uint32_t GetFrame(
        const EffectObjectState& state) = 0;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    virtual void ScaleFrame(std::uint32_t frame,
                            const std::array<std::uint32_t, 3u>& scale,
                            std::uint32_t mode) = 0;
    virtual void ApplyMatrix(std::uint32_t frame,
                             const EffectObjectAimMatrix& matrix,
                             std::uint32_t mode) = 0;
    virtual void TranslateFrame(
        std::uint32_t frame,
        const std::array<std::uint32_t, 3u>& position,
        std::uint32_t mode) = 0;
    virtual void UpdateFrameRotation(
        std::uint32_t frame,
        const std::array<std::uint32_t, 3u>& offset,
        std::array<float, 3u>& output) = 0;
    [[nodiscard]] virtual std::uint8_t SetRotation(
        EffectObjectState& state,
        const std::array<float, 3u>& angles,
        std::uint32_t mode) = 0;
};

class EffectObjectAimRuntimeStub final : public IEffectObjectAimRuntime {
public:
    void GetPosition(const EffectObjectState&,
                     std::array<std::uint32_t, 3u>&) override {}
    void Normalize(std::array<float, 3u>&) override {}
    void UpdateMatrix(EffectObjectAimMatrix&) override {}
    [[nodiscard]] std::uint32_t GetFrame(
        const EffectObjectState&) override { return 0u; }
    void SetFrameIdentity(std::uint32_t) override {}
    void ScaleFrame(std::uint32_t,
                    const std::array<std::uint32_t, 3u>&,
                    std::uint32_t) override {}
    void ApplyMatrix(std::uint32_t, const EffectObjectAimMatrix&,
                     std::uint32_t) override {}
    void TranslateFrame(std::uint32_t,
                        const std::array<std::uint32_t, 3u>&,
                        std::uint32_t) override {}
    void UpdateFrameRotation(
        std::uint32_t, const std::array<std::uint32_t, 3u>&,
        std::array<float, 3u>&) override {}
    [[nodiscard]] std::uint8_t SetRotation(
        EffectObjectState&, const std::array<float, 3u>&,
        std::uint32_t) override { return 0u; }
};

}  // namespace pal4
