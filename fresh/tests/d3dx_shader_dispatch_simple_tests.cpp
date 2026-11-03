#include "pal4/d3dx_shader_dispatch_simple.h"

#include <array>
#include <cstdint>
#include <iostream>

namespace {

struct Runtime final : pal4::ID3DXShaderDispatchSimpleRuntime {
    std::uint32_t append_first = 0;
    std::uint32_t append_second = 0;
    std::uint32_t usage_destination = 0;
    std::uint32_t usage_incoming = 0;
    std::uint32_t append_count = 0;
    std::uint32_t usage_count = 0;
    std::uint32_t scope_pop_count = 0;

    std::uint32_t AppendList(std::uint32_t first,
                             std::uint32_t second) override {
        append_first = first;
        append_second = second;
        ++append_count;
        return 0x3333;
    }
    void MergeUsage(std::uint32_t destination,
                    std::uint32_t incoming) override {
        usage_destination = destination;
        usage_incoming = incoming;
        ++usage_count;
    }
    void PopScope() override { ++scope_pop_count; }
};

int failures = 0;
void Check(bool condition, const char* label) {
    if (!condition) { std::cerr << label << '\n'; ++failures; }
}

}  // namespace

int main() {
    Runtime runtime;
    constexpr std::array<std::uint32_t, 2> arguments{11, 22};
    std::uint32_t result = 0xbeef;
    Check(pal4::D3DXShaderDispatchSimple::Execute(
              2, arguments, runtime, result) && result == 11,
          "pass-through handler keeps first argument");
    Check(pal4::D3DXShaderDispatchSimple::Execute(
              263, arguments, runtime, result) && result == 0x3333 &&
          runtime.append_first == 11 && runtime.append_second == 22,
          "shared list handler appends both arguments");
    Check(pal4::D3DXShaderDispatchSimple::Execute(
              129, arguments, runtime, result) && result == 11 &&
          runtime.usage_destination == 11 &&
          runtime.usage_incoming == 22,
          "usage merge keeps the first argument");
    Check(pal4::D3DXShaderDispatchSimple::Execute(
              155, arguments, runtime, result) && result == 22,
          "second-argument handler discards first argument");
    Check(pal4::D3DXShaderDispatchSimple::Execute(
              157, arguments, runtime, result) && result == 0 &&
          runtime.scope_pop_count == 1,
          "scope handler pops and yields null");
    Check(pal4::D3DXShaderDispatchSimple::Execute(
              279, arguments, runtime, result) && result == 0,
          "empty production yields null");
    result = 0xbeef;
    Check(!pal4::D3DXShaderDispatchSimple::Execute(
              294, arguments, runtime, result) && result == 0xbeef,
          "other dispatch handlers remain unclaimed");
    Check(!pal4::D3DXShaderDispatchSimple::Execute(
              3, std::span<const std::uint32_t>{}, runtime, result),
          "short argument list cannot access missing operand");

    Runtime census;
    std::uint32_t handled = 0;
    for (std::uint32_t opcode = 0; opcode <= 296; ++opcode) {
        std::uint32_t value = 0;
        if (pal4::D3DXShaderDispatchSimple::Execute(
                opcode, arguments, census, value)) ++handled;
    }
    Check(handled == 119 && census.append_count == 9 &&
          census.usage_count == 3 && census.scope_pop_count == 4,
          "all 119 shared-opcode entries are covered once");
    return failures == 0 ? 0 : 1;
}
