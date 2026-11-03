#pragma once

#include "pal4/clump_object.h"

#include <cstdint>

namespace pal4 {

class IClumpObjectCollectionRuntime {
public:
    virtual ~IClumpObjectCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t HandleEvent(
        ClumpObjectState& state, std::uint32_t xml_offset,
        std::uint32_t context, std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void Release(std::uint32_t object) = 0;
};

class ClumpObjectCollectionRuntimeStub final
    : public IClumpObjectCollectionRuntime {
public:
    [[nodiscard]] std::uint32_t HandleEvent(
        ClumpObjectState&, std::uint32_t,
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    void Free(std::uint32_t) override {}
    void Release(std::uint32_t) override {}
};

class ClumpObjectCollectionFacet final {
public:
    // IDA 0x60b860.
    [[nodiscard]] static bool Update(
        ClumpObjectState& state, std::uint32_t requested,
        IClumpObjectCollectionRuntime& runtime);
    // IDA 0x60bab0. Native char return is the old end pointer's low byte.
    [[nodiscard]] static std::uint8_t Clear(
        ClumpObjectState& state, IClumpObjectCollectionRuntime& runtime);
    // IDA 0x60bb20.
    [[nodiscard]] static std::uint32_t Get(
        const ClumpObjectState& state, std::uint32_t index,
        IClumpObjectCollectionRuntime& runtime);
};

}  // namespace pal4
