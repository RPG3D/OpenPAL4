#pragma once

#include "pal4/effect_system_object_f.h"

#include <cstdint>

namespace pal4 {

class IEffectSystemObjectFFacetRuntime {
public:
    virtual ~IEffectSystemObjectFFacetRuntime() = default;
    virtual void InitializeFacet(EffectSystemObjectFState& state,
                                 std::uint32_t byte_offset) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FindWorld(
        std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveClump(
        std::uint32_t world, std::uint32_t clump) = 0;
};

class EffectSystemObjectFFacetRuntimeStub final
    : public IEffectSystemObjectFFacetRuntime {
public:
    void InitializeFacet(EffectSystemObjectFState&,
                         std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t FindWorld(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t RemoveClump(
        std::uint32_t, std::uint32_t) override { return 0u; }
};

class EffectSystemObjectFFacet final {
public:
    // Common effect prefix, called after the concrete hide/reset facet.
    [[nodiscard]] static std::uint8_t EnableVisibility(std::uint32_t& status);
    // IDA 0x605fc0, facet pointer is object +20.
    [[nodiscard]] static std::uint8_t Start(
        EffectSystemObjectFState& state,
        IEffectSystemObjectFFacetRuntime& runtime);
    // IDA 0x605fe0.
    [[nodiscard]] static std::uint8_t Stop(
        EffectSystemObjectFState& state,
        IEffectSystemObjectFFacetRuntime& runtime);
};

}  // namespace pal4
