#pragma once

#include "pal4/cylinder_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class ICylinderObjectColorRuntime {
public:
    virtual ~ICylinderObjectColorRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t SetColor(
        std::uint32_t model, const std::array<std::uint8_t, 4>& color) = 0;
};

class CylinderObjectColorRuntimeStub final : public ICylinderObjectColorRuntime {
public:
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t SetColor(
        std::uint32_t, const std::array<std::uint8_t, 4>&) override {
        return 0u;
    }
};

class CylinderObjectColor final {
public:
    // IDA 0x60d890/0x60d8d0, object +44 facet.
    [[nodiscard]] static std::uint8_t Get(
        const CylinderObjectState& state,
        std::array<std::uint8_t, 4>& output,
        ICylinderObjectColorRuntime& runtime);
    [[nodiscard]] static std::uint8_t Set(
        const CylinderObjectState& state,
        const std::array<std::uint8_t, 4>& color,
        ICylinderObjectColorRuntime& runtime);
};

}  // namespace pal4
