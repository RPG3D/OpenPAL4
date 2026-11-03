#include "pal4/tga_rle_data_loader.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ITgaRleRuntime {
    static constexpr std::uint32_t stream = 0x1000u;
    static constexpr std::uint32_t destination = 0x2000u;
    static constexpr std::uint32_t handle = 0x3000u;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint8_t> input;
    std::vector<std::uint32_t> reads;
    std::uint32_t cursor{};
    std::uint32_t fail_read_index{};
    std::uint32_t result_count{1u};
    bool cleared_error{};

    Runtime() {
        WriteWord(stream, handle);
        WriteWord(stream + 8u, 5u);
        WriteByte(stream + 26u, 5u);
        WriteByte(stream + 27u, 0u);
        WriteByte(stream + 30u, 24u);
    }
    [[nodiscard]] std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    [[nodiscard]] std::uint32_t ReadWord(const std::uint32_t address) override {
        return static_cast<std::uint32_t>(ReadByte(address)) |
            (static_cast<std::uint32_t>(ReadByte(address + 1u)) << 8u) |
            (static_cast<std::uint32_t>(ReadByte(address + 2u)) << 16u) |
            (static_cast<std::uint32_t>(ReadByte(address + 3u)) << 24u);
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        for (unsigned index = 0u; index < 4u; ++index)
            WriteByte(address + index,
                      static_cast<std::uint8_t>(value >> (index * 8u)));
        if (address == stream + 8u) cleared_error = value == 0u;
    }
    [[nodiscard]] std::uint32_t ReadItem(
        const std::uint32_t opened,
        const std::span<std::uint8_t> buffer) override {
        Check(opened == handle, "reads from stream handle");
        reads.push_back(static_cast<std::uint32_t>(buffer.size()));
        if (reads.size() == fail_read_index ||
            cursor + buffer.size() > input.size()) return 0u;
        for (auto& byte : buffer) byte = input[cursor++];
        return result_count;
    }
};

void TestNullAndEmptyOutput() {
    Runtime missing;
    Check(pal4::TgaRleDataLoader::Load(Runtime::stream, 0u,
                                        missing) == 1u &&
              missing.reads.empty() &&
              missing.ReadWord(Runtime::stream + 8u) == 5u,
          "null destination returns one without reads or error change");

    Runtime zero_width;
    zero_width.WriteByte(Runtime::stream + 26u, 0u);
    Check(pal4::TgaRleDataLoader::Load(Runtime::stream,
                                        Runtime::destination,
                                        zero_width) == 0u &&
              zero_width.reads.empty() && zero_width.cleared_error,
          "zero-width row clears stream error without file reads");
}

void TestLiteralAndRepeatPackets() {
    Runtime mixed;
    mixed.input = {1u, 1u, 2u, 3u, 4u, 5u, 6u,
                   0x82u, 7u, 8u, 9u};
    Check(pal4::TgaRleDataLoader::Load(Runtime::stream,
                                        Runtime::destination, mixed) == 0u &&
              mixed.reads == std::vector<std::uint32_t>({1u, 3u, 3u,
                                                        1u, 3u}) &&
              mixed.cursor == mixed.input.size() && mixed.cleared_error,
          "literal and repeated packets consume one pixel item per read");
    const std::vector<std::uint8_t> expected{
        1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u,
        7u, 8u, 9u, 7u, 8u, 9u};
    for (std::uint32_t index = 0u; index < expected.size(); ++index)
        Check(mixed.ReadByte(Runtime::destination + index) == expected[index],
              "decoded row bytes match source packets");

    Runtime truncated_packet;
    truncated_packet.WriteByte(Runtime::stream + 26u, 1u);
    truncated_packet.input = {0x7fu, 10u, 11u, 12u};
    Check(pal4::TgaRleDataLoader::Load(Runtime::stream,
                                        Runtime::destination,
                                        truncated_packet) == 0u &&
              truncated_packet.cursor == 4u &&
              truncated_packet.reads.size() == 2u &&
              truncated_packet.cleared_error,
          "scanline stops at width even when packet advertises more pixels");

    Runtime rgba;
    rgba.WriteByte(Runtime::stream + 26u, 2u);
    rgba.WriteByte(Runtime::stream + 30u, 32u);
    rgba.input = {0x81u, 1u, 2u, 3u, 4u};
    Check(pal4::TgaRleDataLoader::Load(Runtime::stream,
                                        Runtime::destination, rgba) == 0u &&
              rgba.reads == std::vector<std::uint32_t>({1u, 4u}) &&
              rgba.ReadByte(Runtime::destination + 7u) == 4u,
          "32-bit repeated pixel copies all four bytes");
}

void TestShortReads() {
    Runtime control;
    control.input = {0x84u, 1u, 2u, 3u};
    control.fail_read_index = 1u;
    Check(pal4::TgaRleDataLoader::Load(Runtime::stream,
                                        Runtime::destination, control) == 1u &&
              control.reads == std::vector<std::uint32_t>({1u}) &&
              control.ReadWord(Runtime::stream + 8u) == 5u,
          "short control read returns one without changing error");

    Runtime repeat;
    repeat.input = {0x84u, 1u, 2u, 3u};
    repeat.fail_read_index = 2u;
    Check(pal4::TgaRleDataLoader::Load(Runtime::stream,
                                        Runtime::destination, repeat) == 1u &&
              repeat.reads == std::vector<std::uint32_t>({1u, 3u}) &&
              repeat.ReadWord(Runtime::stream + 8u) == 5u,
          "short repeated-pixel read returns one");

    Runtime literal;
    literal.input = {1u, 1u, 2u, 3u, 4u, 5u, 6u};
    literal.fail_read_index = 3u;
    Check(pal4::TgaRleDataLoader::Load(Runtime::stream,
                                        Runtime::destination, literal) == 1u &&
              literal.reads == std::vector<std::uint32_t>({1u, 3u, 3u}) &&
              literal.ReadByte(Runtime::destination) == 1u &&
              literal.ReadWord(Runtime::stream + 8u) == 5u,
          "short literal read preserves already decoded output");

    Runtime extra_count;
    extra_count.WriteByte(Runtime::stream + 26u, 1u);
    extra_count.input = {0x80u, 1u, 2u, 3u};
    extra_count.result_count = 2u;
    Check(pal4::TgaRleDataLoader::Load(Runtime::stream,
                                        Runtime::destination, extra_count) == 0u,
          "native short-read test accepts any item count at least one");
}

}  // namespace

int main() {
    TestNullAndEmptyOutput();
    TestLiteralAndRepeatPackets();
    TestShortReads();
    return EXIT_SUCCESS;
}
