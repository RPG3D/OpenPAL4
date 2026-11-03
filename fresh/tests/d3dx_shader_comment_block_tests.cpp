#include "pal4/d3dx_shader_comment_block.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>

namespace {

void Check(const bool condition, const char* const message) {
    if (condition) return;
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::exit(1);
}

void TestConstructorAndEmptySerialization() {
    pal4::D3DXShaderCommentBlock block(0x464e4946u);
    Check(block.Tag() == 0x464e4946u && block.DataSize() == 0u &&
              block.SizeInDwords() == 2u,
          "constructor initializes tag, size and empty chain");
    std::array<std::uint32_t, 2> output{};
    Check(block.Serialize(output) == 0 && output[0] == 0x0001fffeu &&
              output[1] == 0x464e4946u,
          "empty comment contains D3D comment token and tag");
}

void TestAppendDedupAndLayout() {
    pal4::D3DXShaderCommentBlock block(0x11223344u);
    const std::array<std::uint8_t, 3> first{1, 2, 3};
    const std::array<std::uint8_t, 2> second{4, 5};
    std::uint32_t offset = 99;
    Check(block.Append(first.data(), first.size(), 0, &offset) == 0 &&
              offset == 0 && block.DataSize() == 3u,
          "copied fragment starts at offset zero");
    Check(block.Append(first.data(), first.size(), 0, &offset) == 0 &&
              offset == 0 && block.DataSize() == 3u,
          "matching deduplicated fragment reuses offset");
    Check(block.Append(second.data(), second.size(), 1u, &offset) == 0 &&
              offset == 4u && block.DataSize() == 6u,
          "borrowed fragment aligns by default");
    Check(block.Append(first.data(), first.size(), 5u, &offset) == 0 &&
              offset == 6u && block.DataSize() == 9u,
          "flag four suppresses alignment and flag one suppresses dedup");
    Check(block.SizeInDwords() == 5u, "data byte count rounds to words");
    std::array<std::uint32_t, 6> output{};
    Check(block.Serialize(output, 6) == 0 && output[0] == 0x0005fffeu &&
              output[1] == 0x11223344u,
          "explicit larger word count is encoded in token");
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(output.data() + 2);
    const std::array<std::uint8_t, 16> expected{
        1, 2, 3, 0xab, 4, 5, 1, 2, 3, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab};
    Check(std::memcmp(bytes, expected.data(), expected.size()) == 0,
          "serialization preserves alignment and AB trailing fill");
}

void TestStringsAndBounds() {
    pal4::D3DXShaderCommentBlock block(7);
    std::uint32_t offset = 99;
    Check(block.Append("abc", UINT32_MAX, 5u, &offset) == 0 &&
              offset == 0u && block.DataSize() == 4u,
          "sentinel length includes string terminator");
    Check(block.Append(nullptr, 1u, 0) < 0 &&
              block.Append(nullptr, UINT32_MAX, 0) < 0,
          "nonzero null input is rejected");
    std::array<std::uint32_t, 3> output{9, 9, 9};
    Check(block.Serialize(output, 2u) < 0 && output[0] == 9u,
          "too-small word count leaves output unchanged");
    Check(block.Serialize(std::span(output).first(2)) < 0 && output[0] == 9u,
          "short output leaves output unchanged");
    Check(block.Serialize(output, 0x8001u) < 0 && output[0] == 9u,
          "comment token size limit is enforced");
    pal4::D3DXShaderCommentBlock oversized(8);
    const std::uint8_t borrowed = 1;
    Check(oversized.Append(&borrowed, UINT32_MAX - 1u, 1u) == 0 &&
              oversized.Serialize(output) < 0 && output[0] == 9u,
          "wrapped native byte count cannot overrun bounded output");
}

}  // namespace

int main() {
    TestConstructorAndEmptySerialization();
    TestAppendDedupAndLayout();
    TestStringsAndBounds();
}
