#pragma once

#include <cstdint>

namespace pal4 {

class D3DXShaderDispatchTable final {
public:
    // IDA 0x690665/0x6914d5. Maps grammar opcodes to dispatcher labels.
    [[nodiscard]] static std::uint32_t HandlerAddress(
        std::uint32_t opcode) noexcept;
};

}  // namespace pal4
