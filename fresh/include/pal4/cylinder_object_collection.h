#pragma once

#include "pal4/cylinder_object.h"

#include <cstdint>

namespace pal4 {

class ICylinderObjectCollectionRuntime {
public:
    virtual ~ICylinderObjectCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t HandleEvent(
        CylinderObjectState& state, std::uint32_t xml,
        std::uint32_t context, std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void Release(std::uint32_t object) = 0;
};

class CylinderObjectCollectionRuntimeStub final
    : public ICylinderObjectCollectionRuntime {
public:
    [[nodiscard]] std::uint32_t HandleEvent(
        CylinderObjectState&, std::uint32_t, std::uint32_t,
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    void Free(std::uint32_t) override {}
    void Release(std::uint32_t) override {}
};

class CylinderObjectCollection final {
public:
    // IDA 0x60d910/0x60db40/0x60db90, object +48 facet.
    [[nodiscard]] static bool Update(
        CylinderObjectState& state, std::uint32_t requested,
        ICylinderObjectCollectionRuntime& runtime);
    [[nodiscard]] static std::uint8_t Clear(
        CylinderObjectState& state,
        ICylinderObjectCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Get(
        const CylinderObjectState& state, std::uint32_t index,
        ICylinderObjectCollectionRuntime& runtime);
};

}  // namespace pal4
