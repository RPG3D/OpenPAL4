#include "pal4/string_buffer_copy_appender.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IStringBufferCopyAppendRuntime {
    std::vector<std::string> events;
    std::string source = std::string("ab\0c", 4);
    std::string tail_bytes = std::string("XY\0Z", 4);
    std::uint32_t tail_length = 3u;
    bool allocation_succeeds = true;
    std::string written;

    std::string_view CopySource(std::uint32_t address) override {
        Check(address == 0x2000u, "source buffer address");
        events.emplace_back("copy-source");
        return source;
    }
    std::uint32_t TailLength(std::uint32_t address) override {
        Check(address == 0x3000u, "tail object address");
        events.emplace_back("tail-length");
        return tail_length;
    }
    bool Allocate(std::uint32_t length) override {
        events.push_back("allocate:" + std::to_string(length));
        return allocation_succeeds;
    }
    std::string_view TailBytes(std::uint32_t address) override {
        Check(address == 0x3000u, "tail bytes address");
        events.emplace_back("tail-bytes");
        return tail_bytes;
    }
    void WriteDestination(std::uint32_t address,
                          std::string_view value) override {
        Check(address == 0x1000u, "output buffer address");
        events.emplace_back("write");
        written = value;
    }
    void ReleaseTemporary(std::string_view value) override {
        Check(value == written, "release temporary after destination copy");
        events.emplace_back("release");
    }
};

void SuccessfulAppend() {
    Runtime runtime;
    Check(pal4::StringBufferCopyAppender::CopyAndAppend(
              0x1000u, 0x2000u, 0x3000u, runtime) == 0x1000u &&
              runtime.written == std::string("ab\0cXY\0", 7) &&
              runtime.events == (std::vector<std::string>{
                  "copy-source", "tail-length", "allocate:7",
                  "tail-bytes", "write", "release"}),
          "explicit lengths preserve embedded NUL and return output address");
}

void NoAppend() {
    Runtime runtime;
    runtime.allocation_succeeds = false;
    Check(pal4::StringBufferCopyAppender::CopyAndAppend(
              0x1000u, 0x2000u, 0x3000u, runtime) == 0x1000u &&
              runtime.written == runtime.source &&
              runtime.events == (std::vector<std::string>{
                  "copy-source", "tail-length", "allocate:7",
                  "write", "release"}),
          "allocation failure copies original source to destination");

    runtime = Runtime{};
    runtime.tail_length = 0u;
    Check(pal4::StringBufferCopyAppender::CopyAndAppend(
              0x1000u, 0x2000u, 0x3000u, runtime) == 0x1000u &&
              runtime.written == runtime.source &&
              runtime.events == (std::vector<std::string>{
                  "copy-source", "tail-length", "write", "release"}),
          "zero tail bypasses allocation");
}

void InvalidLength() {
    Runtime runtime;
    runtime.tail_length = std::numeric_limits<std::uint32_t>::max();
    bool overflow = false;
    try {
        static_cast<void>(pal4::StringBufferCopyAppender::CopyAndAppend(
            0x1000u, 0x2000u, 0x3000u, runtime));
    } catch (const std::length_error&) {
        overflow = true;
    }
    Check(overflow && runtime.events == (std::vector<std::string>{
              "copy-source", "tail-length"}),
          "32-bit length overflow follows legacy length-error path");

    pal4::StringBufferCopyAppendRuntimeStub stub;
    Check(pal4::StringBufferCopyAppender::CopyAndAppend(
              0x1000u, 0x2000u, 0x3000u, stub) == 0x1000u,
          "native text stub links and forwards output address");
}

void CStringAppend() {
    Runtime runtime;
    runtime.tail_length = 2u;
    Check(pal4::StringBufferCopyAppender::CopyAndAppendCString(
              0x1000u, 0x2000u, 0x3000u, runtime) == 0x1000u &&
              runtime.written == std::string("ab\0cXY", 6) &&
              runtime.events == (std::vector<std::string>{
                  "copy-source", "tail-length", "allocate:6",
                  "tail-bytes", "write", "release"}),
          "C string append stops at first NUL in the tail");

    runtime = Runtime{};
    runtime.tail_length = 0u;
    Check(pal4::StringBufferCopyAppender::CopyAndAppendCString(
              0x1000u, 0x2000u, 0x3000u, runtime) == 0x1000u &&
              runtime.written == runtime.source &&
              runtime.events == (std::vector<std::string>{
                  "copy-source", "tail-length", "write", "release"}),
          "empty C string copies source and releases temporary");
}

}  // namespace

int main() {
    try {
        SuccessfulAppend();
        NoAppend();
        InvalidLength();
        CStringAppend();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
