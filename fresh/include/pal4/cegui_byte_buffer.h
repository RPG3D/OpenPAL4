#pragma once

#include <cstdint>

#include "pal4/static_character_buffers.h"

namespace pal4 {

class ICeguiByteBufferRuntime {
public:
    virtual ~ICeguiByteBufferRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
};

class CeguiByteBuffer final {
public:
    [[nodiscard]] static std::uint32_t Insert(
        StaticCharacterBufferState& state, std::uint32_t position,
        std::uint32_t count, std::uint32_t fill_address,
        ICeguiByteBufferRuntime& runtime);
};

}  // namespace pal4
