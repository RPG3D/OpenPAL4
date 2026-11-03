#pragma once

#include "pal4/clump_object.h"

#include <cstdint>

namespace pal4 {

class IClumpObjectVisibilityRuntime {
public:
    virtual ~IClumpObjectVisibilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EffectManager() = 0;
    virtual void RemoveClumpAndProcessAtomics(std::uint32_t effect_manager,
                                            std::uint32_t clump) = 0;
    virtual void PerformOwnerAction2(std::uint32_t owner,
                                     std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::uint32_t WorldForClump(
        std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::uint8_t RemoveClump(std::uint32_t world,
                                                    std::uint32_t clump) = 0;
    [[nodiscard]] virtual bool OwnerAllowsClump(std::uint32_t owner,
                                                 std::uint32_t clump) = 0;
    virtual void PerformOwnerAction(std::uint32_t owner,
                                    std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::uint8_t HandleEffectAddition(
        std::uint32_t effect_manager, std::uint32_t clump) = 0;
};

class ClumpObjectVisibilityRuntimeStub final : public IClumpObjectVisibilityRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t EffectManager() override { return 0u; }
    void RemoveClumpAndProcessAtomics(std::uint32_t, std::uint32_t) override {}
    void PerformOwnerAction2(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t WorldForClump(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint8_t RemoveClump(std::uint32_t,
                                            std::uint32_t) override { return 0u; }
    [[nodiscard]] bool OwnerAllowsClump(std::uint32_t,
                                         std::uint32_t) override { return false; }
    void PerformOwnerAction(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint8_t HandleEffectAddition(
        std::uint32_t, std::uint32_t) override { return 0u; }
};

class ClumpObjectVisibilityFacet final {
public:
    // IDA 0x60ace0/0x60ad70, native facet at object +40.
    [[nodiscard]] static std::uint8_t Activate(
        ClumpObjectState& state, IClumpObjectVisibilityRuntime& runtime);
    [[nodiscard]] static std::uint8_t Deactivate(
        ClumpObjectState& state, IClumpObjectVisibilityRuntime& runtime);
};

}  // namespace pal4
