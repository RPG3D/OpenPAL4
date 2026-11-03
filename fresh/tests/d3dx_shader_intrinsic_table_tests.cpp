#include "pal4/d3dx_shader_intrinsic_table.h"

#include <cstddef>
#include <iostream>
#include <string_view>
#include <utility>

namespace {

int failures = 0;
void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        ++failures;
    }
}

}  // namespace

int main() {
    using Table = pal4::D3DXShaderIntrinsicTable;
    Check(Table::Name(0) == "D3DCOLORtoUBYTE4" &&
              Table::Opcode(0) == 0x59 &&
              Table::Name(93) == "transpose" &&
              Table::Opcode(93) == 0x58,
          "first and last rule identity");
    Check(Table::SlotWord(0, 0, 0) == 0x854f94 &&
              Table::SlotWord(0, 0, 1) == 0x20 &&
              Table::SlotWord(0, 0, 3) == 0x850fa0 &&
              Table::SlotWord(0, 1, 0) == 0x854f90 &&
              Table::CString(0x854f90) == "x",
          "snapshot pointer and slot mapping");
    Check(Table::FindRange("mul") ==
              std::pair<std::size_t, std::size_t>{42, 51} &&
              Table::FindRange("tex2D") ==
                  std::pair<std::size_t, std::size_t>{75, 77} &&
              Table::FindRange("transpose") ==
                  std::pair<std::size_t, std::size_t>{93, 94} &&
              Table::FindRange("missing").first ==
                  Table::FindRange("missing").second,
          "duplicate and absent name ranges");
    for (std::size_t index = 0; index < Table::kRuleCount; ++index) {
        Check(!Table::Name(index).empty(), "all rule names resolve");
        if (index != 0)
            Check(Table::Name(index - 1) <= Table::Name(index),
                  "rule names sorted");
    }
    Check(Table::Word(0) == 0 &&
              Table::CString(0).empty() &&
              Table::SlotWord(94, 0, 0) == 0 &&
              Table::SlotWord(0, 5, 0) == 0 &&
              Table::SlotWord(0, 0, 8) == 0,
          "out-of-range reads");
    return failures == 0 ? 0 : 1;
}
