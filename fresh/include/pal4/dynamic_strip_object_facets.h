#pragma once

#include "pal4/dynamic_strip_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IDynamicStripVisibilityRuntime {
public:
    virtual ~IDynamicStripVisibilityRuntime() = default;
    virtual void Deactivate(DynamicStripObjectState& state) = 0;
};

class DynamicStripVisibilityRuntimeStub final
    : public IDynamicStripVisibilityRuntime {
public:
    void Deactivate(DynamicStripObjectState& state) override;
};

class DynamicStripVisibilityFacet final {
public:
    // IDA 0x60f220/0x60f240; owner facet at +40.
    [[nodiscard]] static std::uint8_t Activate(
        DynamicStripObjectState& state,
        IDynamicStripVisibilityRuntime& runtime);
    [[nodiscard]] static std::uint8_t Deactivate(
        DynamicStripObjectState& state);
};

class DynamicStripColorFacet final {
public:
    // IDA 0x60f250/0x60f280; owner facet at +44.
    [[nodiscard]] static std::uint8_t Get(
        const DynamicStripObjectState& state,
        std::array<std::uint8_t, 4>& rgba);
    [[nodiscard]] static std::uint8_t Set(
        DynamicStripObjectState& state,
        const std::array<std::uint8_t, 4>& rgba);
};

class IDynamicStripTransformRuntime {
public:
    virtual ~IDynamicStripTransformRuntime() = default;
    [[nodiscard]] virtual std::uint8_t UpdateFrame(
        DynamicStripObjectState& state) = 0;
};

class DynamicStripTransformRuntimeStub final
    : public IDynamicStripTransformRuntime {
public:
    [[nodiscard]] std::uint8_t UpdateFrame(
        DynamicStripObjectState&) override { return 0u; }
};

class DynamicStripTransformFacet final {
public:
    // IDA 0x60f2c0/0x60f2f0/0x60f320.
    [[nodiscard]] static std::uint8_t GetRotation(
        const DynamicStripObjectState& state,
        std::array<std::uint32_t, 3>& values);
    [[nodiscard]] static std::uint8_t GetScale(
        const DynamicStripObjectState& state,
        std::array<std::uint32_t, 3>& values);
    [[nodiscard]] static std::uint8_t GetPosition(
        const DynamicStripObjectState& state,
        std::array<std::uint32_t, 3>& values);
    // IDA 0x60f350/0x60f410/0x60f4d0.
    [[nodiscard]] static std::uint8_t SetScale(
        DynamicStripObjectState& state,
        const std::array<float, 3>& values, std::uint8_t relative,
        IDynamicStripTransformRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetRotation(
        DynamicStripObjectState& state,
        const std::array<float, 3>& values, std::uint8_t relative,
        IDynamicStripTransformRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetPosition(
        DynamicStripObjectState& state,
        const std::array<float, 3>& values, std::uint8_t relative,
        IDynamicStripTransformRuntime& runtime);
    // IDA 0x60f590; Track facet at +36, raw owner words +92/+96/+100.
    [[nodiscard]] static std::uint8_t SetTrackPosition(
        DynamicStripObjectState& state,
        const std::array<std::uint32_t, 3>& values,
        IDynamicStripTransformRuntime& runtime);
};

}  // namespace pal4
