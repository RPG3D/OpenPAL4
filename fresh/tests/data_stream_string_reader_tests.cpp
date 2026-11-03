#include "pal4/data_stream_string_reader.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IDataStreamStringRuntime {
    std::uint32_t length = 0u;
    std::string bytes;
    std::string result;
    std::vector<std::string> events;
    bool throw_on_initialize = false;
    void ReadData(const std::uint32_t context, void* output,
                  const std::uint32_t size) override {
        Check(context == 0x1000u, "both reads use original context");
        if (events.empty()) {
            Check(size == 4u, "first read requests a 32-bit length");
            std::memcpy(output, &length, 4u);
            events.emplace_back("read-size");
        } else {
            Check(size == length, "second read uses native length");
            std::memcpy(output, bytes.data(), size);
            events.emplace_back("read-bytes");
        }
    }
    [[nodiscard]] char* Allocate(const std::uint32_t size) override {
        Check(size == length, "allocates exact native length");
        events.emplace_back("allocate");
        return new char[size];
    }
    void InitializeString(const std::uint32_t output,
                          const char* text,
                          const std::size_t count) override {
        Check(output == 0x2000u, "initializes requested string buffer");
        events.emplace_back("initialize");
        result.assign(text, count);
        if (throw_on_initialize) throw std::runtime_error("test failure");
    }
    void Free(char* text) override {
        events.emplace_back("free");
        delete[] text;
    }
};

}  // namespace

int main() {
    using Reader = pal4::DataStreamStringReader;
    Runtime runtime;
    Reader::Read(0u, 0x2000u, runtime);
    Check(runtime.events.empty(), "null context skips all work");
    runtime.length = 6u;
    runtime.bytes = "hello!";
    Reader::Read(0x1000u, 0x2000u, runtime);
    Check(runtime.result == "hello" &&
              runtime.events == std::vector<std::string>({
                  "read-size", "allocate", "read-bytes", "initialize", "free"}),
          "last byte is forced to NUL before string initialization");
    runtime.events.clear();
    runtime.length = 5u;
    runtime.bytes = std::string("a\0bcd", 5u);
    Reader::Read(0x1000u, 0x2000u, runtime);
    Check(runtime.result == "a", "embedded NUL limits copied string length");
    runtime.events.clear();
    runtime.length = 0u;
    bool length_threw = false;
    try {
        Reader::Read(0x1000u, 0x2000u, runtime);
    } catch (const std::length_error&) {
        length_threw = true;
    }
    Check(length_threw && runtime.events ==
              std::vector<std::string>({"read-size"}),
          "zero native length is reported before invalid buffer access");
    runtime.events.clear();
    runtime.length = 2u;
    runtime.bytes = "x!";
    runtime.throw_on_initialize = true;
    bool init_threw = false;
    try {
        Reader::Read(0x1000u, 0x2000u, runtime);
    } catch (const std::runtime_error&) {
        init_threw = true;
    }
    Check(init_threw && runtime.events.back() == "free",
          "owned temporary is released on initialization failure");
    pal4::DataStreamStringRuntimeStub stub;
    Reader::Read(0u, 0u, stub);
    return EXIT_SUCCESS;
}
