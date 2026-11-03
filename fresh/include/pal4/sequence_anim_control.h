#pragma once

#include "pal4/sequence_anim_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class ISequenceAnimControlRuntime {
public:
    virtual ~ISequenceAnimControlRuntime() = default;
    virtual void WriteResourceWord(std::uint32_t address,
                                   std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Geometry(std::uint32_t resource) = 0;
    virtual void SetColor(std::uint32_t geometry, std::uint32_t rgba) = 0;
    virtual void InitializeTransform(std::uint32_t resource) = 0;
    virtual void UpdateGeometry(std::uint32_t geometry,
                                const std::array<std::uint32_t, 4>& corners) = 0;
    virtual void StopPlaybackFacet(SequenceAnimObjectState& state) = 0;
    virtual void DeinitializeResource(std::uint32_t resource) = 0;
    virtual void FreeResource(std::uint32_t resource) = 0;
    virtual void ProcessEvent(std::uint32_t system,
                              SequenceAnimObjectState& state) = 0;
    [[nodiscard]] virtual std::uint32_t DeletingDestroy(
        SequenceAnimObjectState& state, std::uint32_t flags) = 0;
};

class SequenceAnimControl final {
public:
    // IDA 0x6160e0.
    [[nodiscard]] static std::uint8_t Reset(
        SequenceAnimObjectState& state, ISequenceAnimControlRuntime& runtime);
    // IDA 0x616180.
    [[nodiscard]] static std::uint32_t Stop(
        SequenceAnimObjectState& state, ISequenceAnimControlRuntime& runtime);
};

}  // namespace pal4
