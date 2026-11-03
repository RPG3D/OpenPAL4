#include "pal4/tga_file_stream.h"

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

struct Runtime final : pal4::ITgaFileStreamRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::uint32_t allocation{0x1000u};
    std::uint32_t handle{0x5000u};
    std::uint32_t position{17u};
    std::uint32_t read_result{3u};
    std::uint32_t notified_stream{};
    std::uint32_t notified_error{};
    std::uint32_t log_line{};
    std::string log_message;

    [[nodiscard]] std::uint32_t Allocate(
        const std::uint32_t size, const std::uint32_t flags) override {
        Check(size == 40u && flags == 0u,
              "file-stream allocation uses 40 bytes and zero flags");
        events.emplace_back("allocate");
        return allocation;
    }
    [[nodiscard]] std::uint32_t Free(
        const std::uint32_t address) override {
        Check(address == 0x1000u, "frees the file-stream object");
        events.emplace_back("free");
        return 0x77u;
    }
    [[nodiscard]] std::uint32_t OpenHandle(
        const std::uint32_t filename,
        const std::uint32_t mode) override {
        Check(filename == 0x2000u && mode == 0x3000u,
              "open forwards filename and mode pointers");
        events.emplace_back("open-handle");
        return handle;
    }
    void CloseHandle(const std::uint32_t value) override {
        Check(value == 0x5000u, "close uses stored file handle");
        events.emplace_back("close-handle");
    }
    void SeekHandle(const std::uint32_t value,
                    const std::int32_t offset,
                    const std::int32_t origin) override {
        Check(value == 0x5000u && offset == -3 && origin == 2,
              "seek forwards handle, offset and origin");
        events.emplace_back("seek");
    }
    [[nodiscard]] std::uint32_t Position(
        const std::uint32_t value) override {
        Check(value == 0x5000u, "position query uses stored handle");
        events.emplace_back("position");
        return position;
    }
    [[nodiscard]] std::uint32_t ReadHandle(
        const std::uint32_t buffer, const std::uint32_t size,
        const std::uint32_t count, const std::uint32_t value) override {
        Check(buffer == 0x7000u && size == 4u && count == 3u &&
                  value == 0x5000u,
              "read forwards buffer, element width, count and handle");
        events.emplace_back("read");
        return read_result;
    }
    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override { return words[address]; }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
        events.emplace_back("word");
    }
    [[nodiscard]] std::uint8_t ReadByte(
        const std::uint32_t address) override { return bytes[address]; }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
        events.emplace_back("byte");
    }
    void NotifyError(const std::uint32_t stream,
                     const std::uint32_t code) override {
        notified_stream = stream;
        notified_error = code;
        events.emplace_back("notify");
    }
    void LogError(const std::string_view file,
                  const std::uint32_t line,
                  const std::string_view message) override {
        Check(file.find("TgaLoader\\tga.c") != std::string_view::npos,
              "error log retains original TGA source path");
        log_line = line;
        log_message = message;
        events.emplace_back("log");
    }
};

void TestErrorMessages() {
    constexpr std::string_view expected[]{
        "Success", "Error", "Out of memory", "Failed to open file",
        "Seek failed", "Read failed", "Write failed", "Unknown sub-format"};
    for (std::uint8_t code = 0u; code < 8u; ++code)
        Check(pal4::TgaFileStream::ErrorMessage(code) == expected[code],
              "IDA error table entry matches exact text");
    Check(pal4::TgaFileStream::ErrorMessage(8u) == "Error" &&
              pal4::TgaFileStream::ErrorMessage(255u) == "Error",
          "out-of-range byte codes map to generic error");
}

void TestOpenAndClose() {
    Runtime allocation_failure;
    allocation_failure.allocation = 0u;
    Check(pal4::TgaFileStream::Open(
              0x2000u, 0x3000u, allocation_failure) == 0u &&
              allocation_failure.events ==
                  (std::vector<std::string>{"allocate", "log"}) &&
              allocation_failure.log_line == 36u &&
              allocation_failure.log_message == "Out of memory",
          "allocation failure logs error two before opening a handle");

    Runtime open_failure;
    open_failure.handle = 0u;
    Check(pal4::TgaFileStream::Open(
              0x2000u, 0x3000u, open_failure) == 0u &&
              open_failure.notified_stream == 0x1000u &&
              open_failure.notified_error == 3u &&
              open_failure.log_line == 44u &&
              open_failure.log_message == "Failed to open file" &&
              open_failure.words[0x1008u] == 3u,
          "failed handle open notifies, logs, marks error and frees record");
    Check(open_failure.events == (std::vector<std::string>{
              "allocate", "word", "open-handle", "notify", "log",
              "word", "free"}),
          "open failure preserves native callback and free order");

    Runtime success;
    success.words[0x1004u] = 0xfeedu;
    Check(pal4::TgaFileStream::Open(
              0x2000u, 0x3000u, success) == 0x1000u &&
              success.words[0x1000u] == 0x5000u &&
              success.words[0x1004u] == 0u &&
              success.words[0x1008u] == 0u,
          "successful open stores handle and clears selected fields");
    Check(pal4::TgaFileStream::Close(0x1000u, success) == 0x77u &&
              success.events[success.events.size() - 2u] == "close-handle" &&
              success.events.back() == "free",
          "close forwards stored handle then allocator result");
    const auto count = success.events.size();
    Check(pal4::TgaFileStream::Close(0u, success) == 0u &&
              success.events.size() == count,
          "null close performs no operations");
}

void TestSeekReadAndByteSwap() {
    Runtime runtime;
    runtime.words[0x1000u] = 0x5000u;
    Check(pal4::TgaFileStream::Seek(
              0x1000u, -3, 2, runtime) == 17u &&
              runtime.words[0x1004u] == 17u &&
              runtime.events ==
                  (std::vector<std::string>{"seek", "position", "word"}),
          "seek updates cached position after the handle call");

    runtime.events.clear();
    runtime.position = 29u;
    Check(pal4::TgaFileStream::Read(
              0x1000u, 0x7000u, 4u, 3u, runtime) == 3u &&
              runtime.words[0x1004u] == 29u &&
              runtime.events ==
                  (std::vector<std::string>{"read", "position", "word"}),
          "read returns item count and refreshes cached position");

    for (std::uint32_t index = 0u; index < 12u; ++index)
        runtime.bytes[0x8000u + index] = static_cast<std::uint8_t>(index);
    runtime.events.clear();
    pal4::TgaFileStream::SwapBytes(0x8000u, 5u, 4u, runtime);
    Check(runtime.bytes[0x8000u] == 2u &&
              runtime.bytes[0x8002u] == 0u &&
              runtime.bytes[0x8004u] == 6u &&
              runtime.bytes[0x8006u] == 4u &&
              runtime.bytes[0x8008u] == 8u &&
              runtime.events.size() == 4u,
          "byte limit five swaps offsets zero and four only");
    pal4::TgaFileStream::SwapBytes(0x8000u, 0u, 0u, runtime);
    bool rejected = false;
    try { pal4::TgaFileStream::SwapBytes(0x8000u, 1u, 0u, runtime); }
    catch (const std::invalid_argument&) { rejected = true; }
    Check(rejected, "positive limit with zero stride rejects native infinite loop");
}

}  // namespace

int main() {
    TestErrorMessages();
    TestOpenAndClose();
    TestSeekReadAndByteSwap();
}
