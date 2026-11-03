#include "pal4/tga_color_map_loader.h"

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
    static constexpr std::uint32_t metadata = 0x2000u;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::uint32_t allocation{0x3000u};
    std::uint32_t read_result{1u};
    std::uint32_t notified_code{999u};
    std::uint32_t logged_line{};
    std::string logged_message;
    bool seek_failure{};
    bool callback{true};

    Runtime() {
        WriteByte(stream + 12u, 7u);
        WriteWord(stream + 8u, 6u);
        events.clear();
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
        if (address == stream + 8u) events.emplace_back("write-error");
        if (address == metadata) events.emplace_back("write-map");
    }
    void Seek(const std::uint32_t opened, const std::int32_t offset,
              const std::int32_t origin) override {
        Check(opened == stream && offset == 18 && origin == 0,
              "seeks to the byte after the TGA header");
        events.emplace_back("seek");
        WriteWord(stream + 4u, seek_failure ? 17u : 18u);
    }
    [[nodiscard]] std::uint32_t Allocate(
        const std::uint32_t size, const std::uint32_t flags) override {
        Check(size == 7u && flags == 0u,
              "allocates the ID-length byte count with zero flags");
        events.emplace_back("allocate");
        return allocation;
    }
    void Free(const std::uint32_t address) override {
        Check(address == metadata,
              "native failure path passes metadata, not allocated map");
        events.emplace_back("free-metadata");
    }
    [[nodiscard]] std::uint32_t Read(
        const std::uint32_t opened, const std::uint32_t buffer,
        const std::uint32_t size, const std::uint32_t count) override {
        Check(opened == stream && buffer == allocation &&
                  size == 7u && count == 1u,
              "reads one ID-length item into the allocation");
        Check(ReadWord(metadata) == allocation,
              "metadata points at allocation before read");
        events.emplace_back("read");
        return read_result;
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
              "logs the original TGA source file");
        logged_line = line;
        logged_message = message;
        events.emplace_back("log");
    }
};

using Events = std::vector<std::string>;

void TestGuardsAndSeek() {
    Runtime null_stream;
    Check(pal4::TgaColorMapLoader::Load(0u, Runtime::metadata,
                                        null_stream) == 0u &&
              null_stream.events.empty(), "null stream has no effects");
    Runtime no_id;
    no_id.WriteByte(Runtime::stream + 12u, 0u);
    Check(pal4::TgaColorMapLoader::Load(Runtime::stream, Runtime::metadata,
                                        no_id) == 0u &&
              no_id.events.empty(), "zero ID length skips seek and allocation");

    Runtime seek;
    seek.seek_failure = true;
    seek.callback = false;
    Check(pal4::TgaColorMapLoader::Load(Runtime::stream, Runtime::metadata,
                                        seek) == 0u &&
              seek.events == Events({"seek", "callback-check", "log",
                                     "write-error"}) &&
              seek.notified_code == 999u && seek.logged_line == 159u &&
              seek.logged_message == "Seek failed" &&
              seek.ReadWord(Runtime::stream + 8u) == 4u,
          "failed seek reports line 159 and stores error four");
}

void TestReadSuccessAndFailure() {
    Runtime good;
    good.read_result = 2u;
    Check(pal4::TgaColorMapLoader::Load(Runtime::stream, Runtime::metadata,
                                        good) == 0u &&
              good.events == Events({"seek", "allocate", "write-map",
                                     "read", "write-error"}) &&
              good.ReadWord(Runtime::metadata) == good.allocation &&
              good.ReadWord(Runtime::stream + 8u) == 0u,
          "any nonzero read count succeeds without freeing map");

    Runtime failed;
    failed.read_result = 0u;
    Check(pal4::TgaColorMapLoader::Load(Runtime::stream, Runtime::metadata,
                                        failed) == 0u &&
              failed.events == Events({"seek", "allocate", "write-map",
                                       "read", "free-metadata",
                                       "callback-check", "notify", "log",
                                       "write-error"}) &&
              failed.ReadWord(Runtime::metadata) == failed.allocation &&
              failed.notified_code == 5u && failed.logged_line == 170u &&
              failed.logged_message == "Read failed" &&
              failed.ReadWord(Runtime::stream + 8u) == 5u,
          "read failure frees metadata address before error reporting");

    Runtime no_allocation;
    no_allocation.allocation = 0u;
    no_allocation.read_result = 0u;
    Check(pal4::TgaColorMapLoader::Load(Runtime::stream, Runtime::metadata,
                                        no_allocation) == 0u &&
              no_allocation.ReadWord(Runtime::metadata) == 0u &&
              no_allocation.logged_line == 170u,
          "null allocation still flows through file read");
}

}  // namespace

int main() {
    TestGuardsAndSeek();
    TestReadSuccessAndFailure();
    return EXIT_SUCCESS;
}
