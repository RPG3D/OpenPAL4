#pragma once

#include <cstdint>

#include "pal4/static_character_buffers.h"

namespace pal4 {

class ICeguiWideBufferRuntime {
public:
    virtual ~ICeguiWideBufferRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint16_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint16_t value) = 0;
};

class CeguiWideBuffer final {
public:
    [[nodiscard]] static std::uint32_t Insert(
        StaticCharacterBufferState& state, std::uint32_t position,
        std::uint32_t count, std::uint32_t fill_address,
        ICeguiWideBufferRuntime& runtime);

    [[nodiscard]] static std::uint32_t CopyTo(
        StaticCharacterBufferState& state, std::uint32_t destination,
        std::uint32_t source, ICeguiWideBufferRuntime& runtime);
};

}  // namespace pal4
