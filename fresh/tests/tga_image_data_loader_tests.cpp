#include "pal4/tga_image_data_loader.h"

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

struct Runtime final : pal4::ITgaImageDataRuntime {
    static constexpr std::uint32_t stream = 0x1000u;
    static constexpr std::uint32_t data_slot = 0x2000u;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::uint32_t allocation{0x3000u};
    std::uint32_t resized{0x4000u};
    std::uint32_t read_result{1u};
    std::uint32_t notified_code{999u};
    std::uint32_t logged_line{};
    std::string logged_message;
    std::uint32_t swapped_size{};
    std::uint32_t swapped_stride{};
    bool seek_failure{};
    bool callback{true};

    Runtime() {
        WriteByte(stream + 12u, 3u);
        WriteHalf(stream + 18u, 4u);
        WriteByte(stream + 20u, 24u);
        WriteWord(stream + 4u, 21u);
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
    void WriteByte(const std::uint32_t address, const std::uint8_t value) {
        bytes[address] = value;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        for (unsigned index = 0u; index < 4u; ++index)
            WriteByte(address + index,
                      static_cast<std::uint8_t>(value >> (index * 8u)));
        if (address == stream + 8u) events.emplace_back("write-error");
        if (address == data_slot) events.emplace_back("write-data");
    }
    void Seek(const std::uint32_t opened, const std::int32_t offset,
              const std::int32_t origin) override {
        Check(opened == stream && offset == 21 && origin == 0,
              "seeks past 18-byte header and three-byte ID");
        events.emplace_back("seek");
        WriteWord(stream + 4u, seek_failure ? 20u : 21u);
    }
    [[nodiscard]] std::uint32_t Allocate(
        const std::uint32_t size, const std::uint32_t flags) override {
        Check(size == static_cast<std::uint32_t>(ReadByte(stream + 20u)) *
                          (ReadByte(stream + 18u) |
                           (ReadByte(stream + 19u) << 8u)) / 8u &&
                  flags == 0u,
              "allocates width times color-map depth divided by eight");
        events.emplace_back("allocate");
        return allocation;
    }
    [[nodiscard]] std::uint32_t Reallocate(
        const std::uint32_t address, const std::uint32_t size,
        const std::uint32_t flags) override {
        Check(address == allocation && size == 1u && flags == 0u,
              "15/16-bit branch reallocates to one byte");
        events.emplace_back("reallocate");
        return resized;
    }
    [[nodiscard]] std::uint32_t Read(
        const std::uint32_t opened, const std::uint32_t buffer,
        const std::uint32_t size, const std::uint32_t count) override {
        Check(opened == stream && buffer == allocation &&
                  size == static_cast<std::uint32_t>(ReadByte(stream + 20u)) *
                              (ReadByte(stream + 18u) |
                               (ReadByte(stream + 19u) << 8u)) / 8u &&
                  count == 1u && ReadWord(data_slot) == allocation,
              "reads one calculated-size item after storing pointer");
        events.emplace_back("read");
        return read_result;
    }
    void SwapBytes(const std::uint32_t data, const std::uint32_t limit,
                   const std::uint32_t stride) override {
        Check(data == allocation, "swaps allocated color-map data");
        swapped_size = limit;
        swapped_stride = stride;
        events.emplace_back("swap");
    }
    [[nodiscard]] bool HasErrorCallback(
        const std::uint32_t opened) override {
        Check(opened == stream, "checks stream callback");
        events.emplace_back("callback-check");
        return callback;
    }
    void NotifyError(const std::uint32_t opened,
                     const std::uint32_t code) override {
        Check(opened == stream, "notifies the stream");
        notified_code = code;
        events.emplace_back("notify");
    }
    void LogError(const std::string_view file, const std::uint32_t line,
                  const std::string_view message) override {
        Check(file.find("TgaLoader\\tga.c") != std::string_view::npos,
              "logs the TGA source file");
        logged_line = line;
        logged_message = message;
        events.emplace_back("log");
    }
};

using Events = std::vector<std::string>;

void TestGuardsAndSeek() {
    Runtime null_stream;
    Check(pal4::TgaImageDataLoader::Load(0u, Runtime::data_slot, 0u,
                                         null_stream) == 0u &&
              null_stream.events.empty(), "null stream has no side effects");
    Runtime empty;
    empty.WriteHalf(Runtime::stream + 18u, 0u);
    Check(pal4::TgaImageDataLoader::Load(Runtime::stream,
                                         Runtime::data_slot, 0u, empty) == 0u &&
              empty.events.empty(), "zero byte count has no side effects");
    Runtime fractional;
    fractional.WriteHalf(Runtime::stream + 18u, 1u);
    fractional.WriteByte(Runtime::stream + 20u, 1u);
    Check(pal4::TgaImageDataLoader::Load(Runtime::stream,
                                         Runtime::data_slot, 0u,
                                         fractional) == 0u &&
              fractional.events.empty(), "sub-byte size truncates to zero");

    Runtime seek;
    seek.WriteWord(Runtime::stream + 4u, 0u);
    seek.seek_failure = true;
    seek.callback = false;
    Check(pal4::TgaImageDataLoader::Load(Runtime::stream,
                                         Runtime::data_slot, 0u, seek) == 0u &&
              seek.events == Events({"seek", "callback-check", "log",
                                     "write-error"}) &&
              seek.logged_line == 193u &&
              seek.logged_message == "Seek failed" &&
              seek.ReadWord(Runtime::stream + 8u) == 4u,
          "seek failure logs line 193 and stores error four");
}

void TestReadAndSwap() {
    Runtime read;
    read.read_result = 2u;
    Check(pal4::TgaImageDataLoader::Load(Runtime::stream,
                                         Runtime::data_slot, 0u, read) == 0u &&
              read.events == Events({"allocate", "write-data", "read",
                                     "callback-check", "notify", "log",
                                     "write-error"}) &&
              read.notified_code == 5u && read.logged_line == 204u &&
              read.ReadWord(Runtime::data_slot) == read.allocation &&
              read.ReadWord(Runtime::stream + 8u) == 5u,
          "image-data read requires exactly one item");

    Runtime null_allocation;
    null_allocation.allocation = 0u;
    null_allocation.read_result = 0u;
    Check(pal4::TgaImageDataLoader::Load(Runtime::stream,
                                         Runtime::data_slot, 0u,
                                         null_allocation) == 0u &&
              null_allocation.events[2u] == "read" &&
              null_allocation.logged_line == 204u,
          "null allocation is still passed to read");

    for (const auto depth : {24u, 32u}) {
        Runtime swapped;
        swapped.WriteByte(Runtime::stream + 20u,
                          static_cast<std::uint8_t>(depth));
        Check(pal4::TgaImageDataLoader::Load(Runtime::stream,
                                             Runtime::data_slot, 0x120u,
                                             swapped) == 1u &&
                  swapped.events == Events({"allocate", "write-data", "read",
                                            "swap", "write-error"}) &&
                  swapped.swapped_size == depth * 4u / 8u &&
                  swapped.swapped_stride == depth / 8u &&
                  swapped.ReadWord(Runtime::stream + 8u) == 0u,
              "24/32-bit data swaps red and blue when flag bit five is set");
    }
    Runtime plain;
    Check(pal4::TgaImageDataLoader::Load(Runtime::stream,
                                         Runtime::data_slot, 0u, plain) == 1u &&
              plain.events == Events({"allocate", "write-data", "read",
                                     "write-error"}),
          "24-bit data without bit five skips swap");
    Runtime gray;
    gray.WriteByte(Runtime::stream + 20u, 8u);
    Check(pal4::TgaImageDataLoader::Load(Runtime::stream,
                                         Runtime::data_slot, 0x20u, gray) == 1u &&
              gray.events == plain.events,
          "eight-bit data never swaps channels");
}

void TestFifteenAndSixteenBitPaths() {
    for (const auto depth : {15u, 16u}) {
        Runtime failed;
        failed.WriteByte(Runtime::stream + 20u,
                         static_cast<std::uint8_t>(depth));
        failed.resized = 0u;
        Check(pal4::TgaImageDataLoader::Load(Runtime::stream,
                                             Runtime::data_slot, 0u,
                                             failed) == 0u &&
                  failed.events == Events({"allocate", "write-data", "read",
                                           "reallocate", "write-data",
                                           "callback-check", "notify", "log",
                                           "write-error"}) &&
                  failed.ReadWord(Runtime::data_slot) == 0u &&
                  failed.notified_code == 2u && failed.logged_line == 216u &&
                  failed.ReadWord(Runtime::stream + 8u) == 2u,
              "failed 15/16-bit realloc reports out of memory");

        Runtime unsafe;
        unsafe.WriteByte(Runtime::stream + 20u,
                         static_cast<std::uint8_t>(depth));
        bool threw = false;
        try {
            (void)pal4::TgaImageDataLoader::Load(Runtime::stream,
                                                  Runtime::data_slot, 0u,
                                                  unsafe);
        } catch (const std::runtime_error& error) {
            threw = std::string_view(error.what()).find("0x5f9513") !=
                    std::string_view::npos;
        }
        Check(threw && unsafe.ReadWord(Runtime::data_slot) == unsafe.resized &&
                  unsafe.events == Events({"allocate", "write-data", "read",
                                           "reallocate", "write-data"}),
              "nonterminating native branch becomes explicit exception");
    }
}

}  // namespace

int main() {
    TestGuardsAndSeek();
    TestReadAndSwap();
    TestFifteenAndSixteenBitPaths();
    return EXIT_SUCCESS;
}
