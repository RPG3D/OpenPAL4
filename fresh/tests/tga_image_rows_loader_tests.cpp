#include "pal4/tga_image_rows_loader.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
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

struct Runtime final : pal4::ITgaImageRowsRuntime {
    static constexpr std::uint32_t stream = 0x1000u;
    static constexpr std::uint32_t pixels = 0x2000u;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::vector<std::uint32_t> rle_destinations;
    std::uint32_t read_result{2u};
    std::uint32_t rle_fail_call{};
    std::uint32_t resized{0x3000u};
    std::uint32_t notified_code{999u};
    std::uint32_t logged_line{};
    std::string logged_message;
    std::uint32_t swap_address{};
    std::uint32_t swap_limit{};
    std::uint32_t swap_stride{};
    bool seek_failure{};
    bool callback{true};

    Runtime() {
        WriteByte(stream + 12u, 3u);
        WriteHalf(stream + 18u, 2u);
        WriteByte(stream + 20u, 8u);
        WriteByte(stream + 14u, 2u);
        WriteHalf(stream + 26u, 4u);
        WriteByte(stream + 30u, 24u);
        WriteWord(stream + 4u, 47u);
        WriteWord(stream + 8u, 6u);
        events.clear();
    }
    void WriteHalf(const std::uint32_t address, const std::uint16_t value) {
        WriteByte(address, static_cast<std::uint8_t>(value));
        WriteByte(address + 1u, static_cast<std::uint8_t>(value >> 8u));
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
        if (address == stream + 14u) events.emplace_back("write-type");
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        for (unsigned index = 0u; index < 4u; ++index)
            WriteByte(address + index,
                      static_cast<std::uint8_t>(value >> (index * 8u)));
        if (address == stream + 8u) events.emplace_back("write-error");
    }
    void Seek(const std::uint32_t opened, const std::int32_t offset,
              const std::int32_t origin) override {
        Check(opened == stream && origin == 0,
              "seeks from start of file");
        events.emplace_back("seek");
        WriteWord(stream + 4u, seek_failure ?
                  static_cast<std::uint32_t>(offset - 1) :
                  static_cast<std::uint32_t>(offset));
    }
    [[nodiscard]] std::uint32_t Read(
        const std::uint32_t opened, const std::uint32_t buffer,
        const std::uint32_t size, const std::uint32_t count) override {
        Check(opened == stream && buffer == pixels && size ==
                  static_cast<std::uint32_t>(ReadByte(stream + 30u)) * 4u / 8u &&
                  count == 2u,
              "raw read takes base pointer, stride and requested row count");
        events.emplace_back("read");
        return read_result;
    }
    [[nodiscard]] std::uint32_t LoadRleRow(
        const std::uint32_t opened,
        const std::uint32_t destination) override {
        Check(opened == stream, "loads RLE row from stream");
        rle_destinations.push_back(destination);
        events.emplace_back("rle-row");
        return rle_destinations.size() == rle_fail_call ? 1u : 0u;
    }
    void SwapBytes(const std::uint32_t data, const std::uint32_t limit,
                   const std::uint32_t stride) override {
        swap_address = data;
        swap_limit = limit;
        swap_stride = stride;
        events.emplace_back("swap");
    }
    [[nodiscard]] std::uint32_t Reallocate(
        const std::uint32_t address, const std::uint32_t size,
        const std::uint32_t flags) override {
        Check(address == pixels && size == 3u && flags == 0u,
              "15/16-bit branch reallocates using row count plus half");
        events.emplace_back("reallocate");
        return resized;
    }
    [[nodiscard]] bool HasErrorCallback(
        const std::uint32_t opened) override {
        Check(opened == stream, "checks stream callback");
        events.emplace_back("callback-check");
        return callback;
    }
    void NotifyError(const std::uint32_t opened,
                     const std::uint32_t code) override {
        Check(opened == stream, "notifies stream error callback");
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

void TestGuardsAndSeek() {
    Runtime null_stream;
    null_stream.events.clear();
    Check(pal4::TgaImageRowsLoader::Load(0u, Runtime::pixels, 2u, 2u, 0u,
                                         null_stream) == 0u &&
              null_stream.events.empty(), "null stream has no side effects");
    Runtime null_pixels;
    Check(pal4::TgaImageRowsLoader::Load(Runtime::stream, 0u, 2u, 2u, 0u,
                                         null_pixels) == 0u &&
              null_pixels.events.empty(), "null destination has no effects");

    Runtime failed;
    failed.WriteWord(Runtime::stream + 4u, 0u);
    failed.events.clear();
    failed.seek_failure = true;
    failed.callback = false;
    Check(pal4::TgaImageRowsLoader::Load(Runtime::stream, Runtime::pixels,
                                         2u, 2u, 0u, failed) == 0u &&
              failed.events == Events({"seek", "callback-check", "log",
                                       "write-error"}) &&
              failed.logged_line == 296u &&
              failed.logged_message == "Seek failed" &&
              failed.notified_code == 999u &&
              failed.ReadWord(Runtime::stream + 8u) == 4u,
          "seek failure logs line 296 without absent callback");
}

void TestRawRows() {
    Runtime raw;
    Check(pal4::TgaImageRowsLoader::Load(Runtime::stream, Runtime::pixels,
                                         2u, 2u, 0x20u, raw) == 2u &&
              raw.events == Events({"read", "swap", "write-error"}) &&
              raw.swap_address == Runtime::pixels + 24u &&
              raw.swap_limit == 24u && raw.swap_stride == 3u &&
              raw.ReadWord(Runtime::stream + 8u) == 0u,
          "raw rows swap from first-row offset and clear error");

    Runtime seek;
    seek.WriteWord(Runtime::stream + 4u, 0u);
    seek.events.clear();
    Check(pal4::TgaImageRowsLoader::Load(Runtime::stream, Runtime::pixels,
                                         2u, 2u, 0u, seek) == 2u &&
              seek.events == Events({"seek", "read", "write-error"}) &&
              seek.ReadWord(Runtime::stream + 4u) == 47u,
          "mispositioned raw input seeks to computed file offset");

    Runtime short_read;
    short_read.read_result = 1u;
    Check(pal4::TgaImageRowsLoader::Load(Runtime::stream, Runtime::pixels,
                                         2u, 2u, 0u, short_read) == 1u &&
              short_read.events == Events({"read", "callback-check",
                                           "notify", "log", "write-error"}) &&
              short_read.logged_line == 310u &&
              short_read.ReadWord(Runtime::stream + 8u) == 5u,
          "short raw read returns completed count and stores error five");
}

void TestRleCounting() {
    Runtime overrun;
    overrun.WriteByte(Runtime::stream + 14u, 10u);
    overrun.events.clear();
    Check(pal4::TgaImageRowsLoader::Load(Runtime::stream, Runtime::pixels,
                                         2u, 2u, 0u, overrun) == 3u &&
              overrun.rle_destinations ==
                  std::vector<std::uint32_t>({Runtime::pixels + 24u,
                                              Runtime::pixels + 36u,
                                              Runtime::pixels + 48u}) &&
              overrun.ReadByte(Runtime::stream + 14u) == 2u &&
              overrun.logged_line == 310u &&
              overrun.ReadWord(Runtime::stream + 8u) == 5u,
          "native RLE loop reads one extra row and reports count mismatch");

    Runtime last_row_fails;
    last_row_fails.WriteByte(Runtime::stream + 14u, 10u);
    last_row_fails.events.clear();
    last_row_fails.rle_fail_call = 3u;
    Check(pal4::TgaImageRowsLoader::Load(Runtime::stream, Runtime::pixels,
                                         2u, 2u, 0x20u,
                                         last_row_fails) == 2u &&
              last_row_fails.events == Events({"rle-row", "rle-row",
                                               "rle-row", "write-type",
                                               "swap", "write-error"}) &&
              last_row_fails.swap_limit == 24u &&
              last_row_fails.ReadWord(Runtime::stream + 8u) == 0u,
          "failed extra RLE call leaves requested row count as success");

    Runtime first_row_fails;
    first_row_fails.WriteByte(Runtime::stream + 14u, 9u);
    first_row_fails.events.clear();
    first_row_fails.rle_fail_call = 1u;
    Check(pal4::TgaImageRowsLoader::Load(Runtime::stream, Runtime::pixels,
                                         2u, 2u, 0u,
                                         first_row_fails) == 0u &&
              first_row_fails.ReadByte(Runtime::stream + 14u) == 1u &&
              first_row_fails.logged_line == 310u,
          "early RLE failure returns count and normalizes image type");
}

void TestSixteenBitBranch() {
    for (const auto depth : {15u, 16u}) {
        Runtime failed;
        failed.WriteByte(Runtime::stream + 30u,
                         static_cast<std::uint8_t>(depth));
        failed.WriteWord(Runtime::stream + 4u, 18u + 3u + 2u +
                         2u * (4u * depth / 8u));
        failed.events.clear();
        failed.resized = 0u;
        Check(pal4::TgaImageRowsLoader::Load(Runtime::stream,
                                             Runtime::pixels, 2u, 2u, 0u,
                                             failed) == 0u &&
                  failed.events == Events({"read", "reallocate",
                                           "callback-check", "notify", "log",
                                           "write-error"}) &&
                  failed.logged_line == 323u &&
                  failed.ReadWord(Runtime::stream + 8u) == 2u,
              "failed 15/16-bit row realloc reports line 323");

        Runtime unsafe;
        unsafe.WriteByte(Runtime::stream + 30u,
                         static_cast<std::uint8_t>(depth));
        unsafe.WriteWord(Runtime::stream + 4u, 18u + 3u + 2u +
                         2u * (4u * depth / 8u));
        unsafe.events.clear();
        bool threw = false;
        try {
            (void)pal4::TgaImageRowsLoader::Load(Runtime::stream,
                                                  Runtime::pixels, 2u, 2u,
                                                  0u, unsafe);
        } catch (const std::runtime_error& error) {
            threw = std::string_view(error.what()).find("0x5f98bf") !=
                    std::string_view::npos;
        }
        Check(threw && unsafe.events == Events({"read", "reallocate"}),
              "nonterminating conversion is a controlled exception");
    }
}

}  // namespace

int main() {
    TestGuardsAndSeek();
    TestRawRows();
    TestRleCounting();
    TestSixteenBitBranch();
    return EXIT_SUCCESS;
}
