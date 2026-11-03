#pragma once

#include "pal4/sequence_anim_object.h"

#include <cstdint>

namespace pal4 {

class ISequenceAnimPlaybackFacetRuntime {
public:
    virtual ~ISequenceAnimPlaybackFacetRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t WorldForClump(
        std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::uint8_t RemoveClump(
        std::uint32_t world, std::uint32_t clump) = 0;
};

class SequenceAnimPlaybackFacetRuntimeStub final
    : public ISequenceAnimPlaybackFacetRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t WorldForClump(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t RemoveClump(
        std::uint32_t, std::uint32_t) override { return 0u; }
};

class SequenceAnimPlaybackFacet final {
public:
    // IDA 0x616400, native facet at owner +20.
    [[nodiscard]] static std::uint8_t Stop(
        SequenceAnimObjectState& state,
        ISequenceAnimPlaybackFacetRuntime& runtime);
};

}  // namespace pal4
