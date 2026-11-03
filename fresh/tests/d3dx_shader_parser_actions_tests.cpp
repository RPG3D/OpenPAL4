#include "pal4/d3dx_shader_parser_actions.h"

#include <array>
#include <cstdint>
#include <iostream>

int main() {
    int failures = 0;
    const auto check = [&](bool condition, const char* name) {
        if (!condition) { std::cerr << name << '\n'; ++failures; }
    };
    pal4::D3DXShaderParserAction action{999, 9};
    check(!pal4::D3DXShaderParserActions::Lookup(0, action) &&
          action.opcode == 999 && action.operand_count == 9,
          "rule zero is invalid and does not mutate output");
    check(!pal4::D3DXShaderParserActions::Lookup(294, action),
          "rule beyond switch is invalid");
    check(pal4::D3DXShaderParserActions::Lookup(1, action) &&
          action.opcode == 0 && action.operand_count == 0,
          "first production");
    check(pal4::D3DXShaderParserActions::Lookup(28, action) &&
          action.opcode == 29 && action.operand_count == 1,
          "reserved opcode gap");
    check(pal4::D3DXShaderParserActions::Lookup(243, action) &&
          action.opcode == 253 && action.operand_count == 1,
          "nonsequential production mapping");
    check(pal4::D3DXShaderParserActions::Lookup(293, action) &&
          action.opcode == 296 && action.operand_count == 0,
          "last production");

    std::array<bool, 297> seen{};
    std::array<std::uint32_t, 6> counts{};
    std::uint32_t checksum = 0;
    for (std::uint32_t rule = 1;
         rule <= pal4::D3DXShaderParserActions::kRuleCount; ++rule) {
        if (!pal4::D3DXShaderParserActions::Lookup(rule, action) ||
            action.opcode >= seen.size() ||
            action.operand_count >= counts.size()) {
            check(false, "all productions have valid actions");
            continue;
        }
        check(!seen[action.opcode], "opcodes are unique");
        seen[action.opcode] = true;
        ++counts[action.operand_count];
        checksum += rule * (action.opcode * 7u + action.operand_count);
    }
    check(checksum == 59833347u, "IDA action-order checksum");
    check(counts == std::array<std::uint32_t, 6>{84, 118, 70, 15, 3, 3},
          "IDA operand-count distribution");
    for (std::uint32_t opcode = 0; opcode < seen.size(); ++opcode) {
        const bool reserved = opcode == 27 || opcode == 28 ||
                              opcode == 126 || opcode == 127;
        check(seen[opcode] != reserved, "all nonreserved opcodes mapped");
    }
    return failures == 0 ? 0 : 1;
}
