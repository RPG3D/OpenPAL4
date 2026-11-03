#pragma once

#include <cstdint>

namespace pal4 {

struct D3DXShaderParserAction {
    std::uint16_t opcode = 0;
    std::uint8_t operand_count = 0;
};

class D3DXShaderParserActions final {
public:
    static constexpr std::uint32_t kRuleCount = 293;

    // IDA 0x691ad0: production rule numbers are one-based.
    [[nodiscard]] static bool Lookup(std::uint32_t rule,
                                     D3DXShaderParserAction& action) noexcept;
};

}  // namespace pal4
