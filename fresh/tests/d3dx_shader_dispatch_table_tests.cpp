#include "pal4/d3dx_shader_dispatch_table.h"

#include <cstdint>
#include <iostream>

namespace {

void Check(bool condition, const char* name, int& failures) {
    if (!condition) {
        std::cerr << name << '\n';
        ++failures;
    }
}

}  // namespace

int main() {
    int failures = 0;
    const auto handler = pal4::D3DXShaderDispatchTable::HandlerAddress;
    Check(handler(0) == 0x690676 && handler(1) == 0x6906a0,
          "first two allocation handlers", failures);
    Check(handler(2) == 0x6906b9 && handler(3) == 0x6906c1 &&
          handler(4) == 0x690f1f,
          "pass-through, list and empty-production handlers", failures);
    Check(handler(27) == 0x691484 && handler(126) == 0x691484,
          "jump-table holes use default error handler", failures);
    Check(handler(109) == 0x690cd5 && handler(202) == 0x691143 &&
          handler(208) == 0x691179 && handler(210) == 0x69118b,
          "declaration and function-call handlers", failures);
    Check(handler(217) == 0x6911d5 && handler(254) == 0x6912e4 &&
          handler(296) == 0x691476,
          "operator, conditional and last assembly handlers", failures);
    Check(handler(297) == 0x691484 && handler(0xffffffffu) == 0x691484,
          "out-of-range opcodes use default error handler", failures);

    std::uint32_t hash = 2166136261u;
    for (std::uint32_t opcode = 0; opcode < 297; ++opcode) {
        const auto offset = handler(opcode) - 0x690000u;
        hash = (hash ^ (offset & 0xffu)) * 16777619u;
        hash = (hash ^ (offset >> 8)) * 16777619u;
    }
    Check(hash == 0x2aad5248u,
          "all 297 IDA jump-table entries match the captured sequence",
          failures);
    return failures == 0 ? 0 : 1;
}
