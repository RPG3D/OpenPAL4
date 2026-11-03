#pragma once

#include "pal4/cylinder_object.h"

#include <cstdint>

namespace pal4 {

class ICylinderObjectVisibilityRuntime {
public:
    virtual ~ICylinderObjectVisibilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FindWorld(
        std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::uint8_t RemoveClump(
        std::uint32_t world, std::uint32_t clump) = 0;
};

class CylinderObjectVisibilityRuntimeStub final
    : public ICylinderObjectVisibilityRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t FindWorld(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t RemoveClump(
        std::uint32_t, std::uint32_t) override { return 0u; }
};

class CylinderObjectVisibilityFacet final {
public:
    // IDA 0x60d500, object +20 facet.
    [[nodiscard]] static std::uint8_t Deactivate(
        CylinderObjectState& state, ICylinderObjectVisibilityRuntime& runtime);
};

}  // namespace pal4
