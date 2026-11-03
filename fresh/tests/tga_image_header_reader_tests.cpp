#include "pal4/tga_image_header_reader.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ITgaImageHeaderRuntime {
    static constexpr std::uint32_t stream = 0x1000u;
    static constexpr std::uint32_t header = 0x2000u;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::array<std::uint8_t, 18u> input{
        6u, 1u, 2u, 0x34u, 0x12u, 0x78u, 0x56u, 8u,
        0xabu, 0x90u, 0xcdu, 0xabu, 5u, 0u, 3u, 0u, 8u, 0x3du};
    std::vector<std::string> events;
    std::uint32_t allocation{header};
    std::uint32_t read_result{1u};
    std::uint32_t notified_code{999u};
    std::uint32_t logged_line{};
    std::string logged_message;
    bool seek_failure{};
    bool has_callback{true};
    bool zeroed_before_read{};

    [[nodiscard]] std::uint32_t ReadWord(const std::uint32_t address) override {
        return static_cast<std::uint32_t>(ReadByte(address)) |
            (static_cast<std::uint32_t>(ReadByte(address + 1u)) << 8u) |
            (static_cast<std::uint32_t>(ReadByte(address + 2u)) << 16u) |
            (static_cast<std::uint32_t>(ReadByte(address + 3u)) << 24u);
    }
    [[nodiscard]] std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        for (unsigned index = 0u; index < 4u; ++index)
            WriteByte(address + index,
                      static_cast<std::uint8_t>(value >> (index * 8u)));
        if (address == stream + 8u) events.emplace_back("write-error");
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
    void Seek(const std::uint32_t opened, const std::int32_t offset,
              const std::int32_t origin) override {
        Check(opened == stream && offset == 0 && origin == 0,
              "seeks TGA stream to start");
        events.emplace_back("seek");
        WriteWord(stream + 4u, seek_failure ? 1u : 0u);
    }
    [[nodiscard]] std::uint32_t Allocate(
        const std::uint32_t size, const std::uint32_t flags) override {
        Check(size == 18u && flags == 0u,
              "allocates 18-byte zero-flags header");
        events.emplace_back("allocate");
        return allocation;
    }
    void Free(const std::uint32_t address) override {
        Check(address == stream || address == header,
              "frees a TGA stream or temporary header");
        events.emplace_back(address == stream ? "free-stream" : "free-header");
    }
    [[nodiscard]] std::uint32_t Read(
        const std::uint32_t opened, const std::uint32_t buffer,
        const std::uint32_t size, const std::uint32_t count) override {
        Check(opened == stream && buffer == header &&
                  size == 18u && count == 1u,
              "reads one 18-byte header item");
        zeroed_before_read = true;
        for (std::uint32_t index = 0u; index < 18u; ++index)
            zeroed_before_read &= ReadByte(header + index) == 0u;
        events.emplace_back("read");
        if (read_result != 0u)
            for (std::uint32_t index = 0u; index < 18u; ++index)
                WriteByte(header + index, input[index]);
        return read_result;
    }
    [[nodiscard]] bool HasErrorCallback(
        const std::uint32_t opened) override {
        Check(opened == stream, "checks stream callback");
        events.emplace_back("callback-check");
        return has_callback;
    }
    void NotifyError(const std::uint32_t opened,
                     const std::uint32_t code) override {
        Check(opened == stream, "notifies the TGA stream");
        notified_code = code;
        events.emplace_back("notify");
    }
    void LogError(const std::string_view file, const std::uint32_t line,
                  const std::string_view message) override {
        Check(file.find("TgaLoader\\tga.c") != std::string_view::npos,
              "logs the original TGA source file");
        logged_line = line;
        logged_message = message;
        events.emplace_back("log");
    }
};

using Events = std::vector<std::string>;

void TestNullAndEarlyFailures() {
    Runtime missing;
    Check(pal4::TgaImageHeaderReader::Read(0u, missing) == 0u &&
              missing.events.empty(), "null stream does nothing");

    Runtime seek;
    seek.seek_failure = true;
    Check(pal4::TgaImageHeaderReader::Read(Runtime::stream, seek) == 0u &&
              seek.events == Events({"seek", "callback-check", "notify",
                                     "log", "write-error"}) &&
              seek.notified_code == 4u && seek.logged_line == 94u &&
              seek.logged_message == "Seek failed" &&
              seek.ReadWord(Runtime::stream + 8u) == 4u,
          "seek failure reports error four without allocation");

    Runtime allocation;
    allocation.allocation = 0u;
    allocation.has_callback = false;
    Check(pal4::TgaImageHeaderReader::Read(Runtime::stream, allocation) == 0u &&
              allocation.events == Events({"seek", "allocate",
                                           "callback-check", "log",
                                           "write-error"}) &&
              allocation.notified_code == 999u &&
              allocation.logged_line == 100u &&
              allocation.logged_message == "Out of memory" &&
              allocation.ReadWord(Runtime::stream + 8u) == 2u,
          "allocation failure skips absent callback");

    Runtime read;
    read.read_result = 0u;
    Check(pal4::TgaImageHeaderReader::Read(Runtime::stream, read) == 0u &&
              read.zeroed_before_read &&
              read.events == Events({"seek", "allocate", "read",
                                    "free-header", "callback-check",
                                    "notify", "log", "write-error"}) &&
              read.notified_code == 5u && read.logged_line == 108u &&
              read.ReadWord(Runtime::stream + 8u) == 5u,
          "read failure frees header before reporting error five");
}

void TestFieldsAndSupportedDepths() {
    for (const auto depth : {8u, 15u, 16u, 24u, 32u}) {
        Runtime good;
        good.input[1u] = 0u;
        good.input[16u] = static_cast<std::uint8_t>(depth);
        Check(pal4::TgaImageHeaderReader::Read(Runtime::stream, good) == 0u &&
                  good.zeroed_before_read &&
                  good.events == Events({"seek", "allocate", "read",
                                         "free-header", "write-error"}) &&
                  good.ReadWord(Runtime::stream + 8u) == 0u,
              "accepted depth frees temporary header and clears error");
        Check(good.ReadByte(Runtime::stream + 12u) == 6u &&
                  good.ReadByte(Runtime::stream + 13u) == 0u &&
                  good.ReadByte(Runtime::stream + 14u) == 2u &&
                  good.ReadByte(Runtime::stream + 16u) == 0x34u &&
                  good.ReadByte(Runtime::stream + 17u) == 0x12u &&
                  good.ReadByte(Runtime::stream + 18u) == 0x78u &&
                  good.ReadByte(Runtime::stream + 19u) == 0x56u &&
                  good.ReadByte(Runtime::stream + 20u) == 8u &&
                  good.ReadByte(Runtime::stream + 22u) == 0xabu &&
                  good.ReadByte(Runtime::stream + 23u) == 0x90u &&
                  good.ReadByte(Runtime::stream + 24u) == 0xcdu &&
                  good.ReadByte(Runtime::stream + 25u) == 0xabu &&
                  good.ReadByte(Runtime::stream + 26u) == 5u &&
                  good.ReadByte(Runtime::stream + 28u) == 3u &&
                  good.ReadByte(Runtime::stream + 30u) == depth &&
                  good.ReadByte(Runtime::stream + 31u) == 13u &&
                  good.ReadByte(Runtime::stream + 32u) == 1u &&
                  good.ReadByte(Runtime::stream + 33u) == 1u,
              "parses little-endian geometry and descriptor fields");
    }
    Runtime palette;
    Check(pal4::TgaImageHeaderReader::Read(Runtime::stream, palette) == 0u &&
              palette.ReadWord(Runtime::stream + 8u) == 0u,
          "palette with eight-bit indices is accepted");
}

void TestUnsupportedFormats() {
    Runtime palette;
    palette.input[16u] = 24u;
    Check(pal4::TgaImageHeaderReader::Read(Runtime::stream, palette) == 0u &&
              palette.logged_line == 128u &&
              palette.notified_code == 7u &&
              palette.ReadWord(Runtime::stream + 8u) == 7u &&
              palette.events == Events({"seek", "allocate", "read",
                                        "callback-check", "notify", "log",
                                        "write-error", "free-stream",
                                        "free-header"}),
          "non-eight-bit palette reports line 128 and frees stream first");

    Runtime depth;
    depth.input[1u] = 0u;
    depth.input[16u] = 12u;
    Check(pal4::TgaImageHeaderReader::Read(Runtime::stream, depth) == 0u &&
              depth.logged_line == 140u &&
              depth.logged_message == "Unknown sub-format" &&
              depth.events == palette.events,
          "unsupported pixel depth reports line 140 with same release order");
}

}  // namespace

int main() {
    TestNullAndEarlyFailures();
    TestFieldsAndSupportedDepths();
    TestUnsupportedFormats();
    return EXIT_SUCCESS;
}
