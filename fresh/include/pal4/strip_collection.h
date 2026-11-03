#pragma once

#include "pal4/strip_object.h"

#include <cstdint>

namespace pal4 {

class IStripCollectionRuntime {
public:
    virtual ~IStripCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t HandleEvent(
        StripObjectState& state, std::uint32_t owner_document,
        std::uint32_t first, std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void Release(std::uint32_t object) = 0;
};

class StripCollection final {
public:
    // IDA 0x61a180, 0x61a3c0, 0x61a430; owner facet +48.
    [[nodiscard]] static bool Update(
        StripObjectState& state, std::uint32_t requested,
        IStripCollectionRuntime& runtime);
    [[nodiscard]] static std::uint8_t Clear(
        StripObjectState& state, IStripCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Get(
        const StripObjectState& state, std::uint32_t index,
        IStripCollectionRuntime& runtime);
};

}  // namespace pal4
