#pragma once

#include "pal4/clump_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IClumpObjectUpdateRuntime {
public:
    virtual ~IClumpObjectUpdateRuntime() = default;
    [[nodiscard]] virtual std::array<std::uint8_t,4> QueryTimedFacet(
        ClumpObjectState& state, std::uint32_t facet_offset) = 0;
    [[nodiscard]] virtual std::int32_t Ftol(double value) = 0;
    virtual void ApplyTimedFacet(ClumpObjectState& state,
                                 std::uint32_t facet_offset,
                                 const std::array<std::uint8_t, 4>& bytes) = 0;
    virtual void UpdateManager(std::uint32_t manager, float delta_time) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FindSkinHierarchy(
        std::uint32_t clump) = 0;
    virtual void AddAnimationTime(std::uint32_t interpolator,
                                  float amount) = 0;
    virtual void SetAnimationTime(std::uint32_t interpolator,
                                  float time) = 0;
    virtual void UpdateHierarchyMatrices(std::uint32_t hierarchy) = 0;
};

class ClumpObjectUpdateRuntimeStub final : public IClumpObjectUpdateRuntime {
public:
    [[nodiscard]] std::array<std::uint8_t,4> QueryTimedFacet(
        ClumpObjectState&, std::uint32_t) override {
        return {};
    }
    [[nodiscard]] std::int32_t Ftol(double) override { return 0; }
    void ApplyTimedFacet(ClumpObjectState&, std::uint32_t,
                         const std::array<std::uint8_t, 4>&) override {}
    void UpdateManager(std::uint32_t, float) override {}
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] float ReadFloat(std::uint32_t) override { return 0.0f; }
    [[nodiscard]] std::uint32_t FindSkinHierarchy(std::uint32_t) override {
        return 0u;
    }
    void AddAnimationTime(std::uint32_t, float) override {}
    void SetAnimationTime(std::uint32_t, float) override {}
    void UpdateHierarchyMatrices(std::uint32_t) override {}
};

}  // namespace pal4
