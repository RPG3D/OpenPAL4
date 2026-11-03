#pragma once

#include "pal4/strip_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IStripControlRuntime {
public:
    virtual ~IStripControlRuntime() = default;
    virtual void SetColor(std::uint32_t model, std::uint32_t rgba) = 0;
    virtual void SetRotation(std::uint32_t model,
                             const std::array<float, 3>& values) = 0;
    virtual void SetScale(std::uint32_t model,
                          const std::array<float, 3>& values) = 0;
    virtual void SetPosition(std::uint32_t model,
                             const std::array<float, 3>& values) = 0;
    virtual void UpdateGeometryVertices(
        std::uint32_t model, const std::array<float, 4>& uv) = 0;
    virtual void UpdateFrameProperties(StripObjectState& state) = 0;
};

class IStripStopRuntime {
public:
    virtual ~IStripStopRuntime() = default;
    virtual void StopFacet(StripObjectState& state,
                           std::uint32_t offset) = 0;
    virtual void DestroyDocument(std::uint32_t document) = 0;
    virtual void DestroyAtomic(std::uint32_t atomic) = 0;
    virtual void DestroyStraight(std::uint32_t model) = 0;
    virtual void DestroyClump(std::uint32_t clump) = 0;
    virtual void DestroyFrame(std::uint32_t frame) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    virtual void ProcessEvent(std::uint32_t system_handle,
                              StripObjectState& state) = 0;
    [[nodiscard]] virtual std::uint32_t DeleteObject(
        StripObjectState& state, std::uint32_t flags) = 0;
};

class StripControl final {
public:
    // IDA 0x6197e0.
    [[nodiscard]] static std::uint8_t Reset(
        StripObjectState& state, IStripControlRuntime& runtime);
    // IDA 0x619910.
    [[nodiscard]] static std::uint32_t Stop(
        StripObjectState& state, IStripStopRuntime& runtime);
};

}  // namespace pal4
