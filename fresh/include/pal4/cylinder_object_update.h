#pragma once

#include "pal4/cylinder_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class ICylinderObjectUpdateRuntime {
public:
    virtual ~ICylinderObjectUpdateRuntime() = default;
    [[nodiscard]] virtual std::array<std::uint8_t, 4> QueryTimedFacet(
        CylinderObjectState& state, std::uint32_t facet_offset) = 0;
    [[nodiscard]] virtual std::int32_t Ftol(double value) = 0;
    virtual void ApplyTimedFacet(
        CylinderObjectState& state, std::uint32_t facet_offset,
        const std::array<std::uint8_t, 4>& bytes) = 0;
};

class CylinderObjectUpdateRuntimeStub final : public ICylinderObjectUpdateRuntime {
public:
    [[nodiscard]] std::array<std::uint8_t, 4> QueryTimedFacet(
        CylinderObjectState&, std::uint32_t) override { return {}; }
    [[nodiscard]] std::int32_t Ftol(double) override { return 0; }
    void ApplyTimedFacet(CylinderObjectState&, std::uint32_t,
                         const std::array<std::uint8_t, 4>&) override {}
};

class CylinderObjectUpdater final {
public:
    // IDA 0x60d3a0: saved ESI shifts the decompiler's stack variables.
    // Four byte outputs are RGBA; only alpha is decayed. The final model
    // callback is nullsub_2 and has no observable operation.
    static void Update(CylinderObjectState& state,
                       double delta_time,
                       ICylinderObjectUpdateRuntime& runtime);
};

}  // namespace pal4
