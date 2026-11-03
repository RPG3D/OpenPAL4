#include "pal4/csb_buffer_reader.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICsbBufferRuntime {
    std::vector<std::string> calls;
    std::uint32_t context = 0x5000u;
    std::uint32_t allocation = 0x9000u;
    std::uint32_t read_size = 17u;

    std::uint32_t CreateContext(const std::uint32_t type,
                                const std::uint32_t access,
                                const char* source) override {
        Check(type == 2u && access == 1u &&
                  std::string(source) == "TimeScript[OnTime]",
              "file context uses type two and read access");
        calls.emplace_back("open");
        return context;
    }
    void ReadSize(const std::uint32_t input,
                  std::uint32_t& output) override {
        Check(input == context, "header read uses active context");
        calls.emplace_back("read-size");
        output = read_size;
    }
    std::uint32_t Allocate(const std::uint32_t size) override {
        Check(size == read_size, "allocation uses four-byte header value");
        calls.emplace_back("allocate");
        return allocation;
    }
    void ReadBytes(const std::uint32_t input,
                   const std::uint32_t destination,
                   const std::uint32_t size) override {
        Check(input == context && destination == allocation &&
                  size == read_size,
              "payload read uses allocated address and header size");
        calls.emplace_back("read-data");
    }
    void CloseContext(const std::uint32_t input,
                      const std::uint32_t output) override {
        Check(input == context && output == 0u,
              "close receives context and null output");
        calls.emplace_back("close");
    }
};

void TestOpenFailure() {
    Runtime runtime;
    runtime.context = 0u;
    std::uint32_t size = 0xaaaau;
    std::uint32_t data = 0xbbbbu;
    Check(!pal4::CsbBufferReader::Read(
              "TimeScript[OnTime]", size, data, runtime) &&
              size == 0xaaaau && data == 0xbbbbu &&
              runtime.calls == std::vector<std::string>{"open"},
          "open failure preserves outputs and skips reads");
}

void TestReadSequence() {
    for (const auto allocated : {0x9000u, 0u}) {
        Runtime runtime;
        runtime.allocation = allocated;
        std::uint32_t size = 0u;
        std::uint32_t data = 0u;
        Check(pal4::CsbBufferReader::Read(
                  "TimeScript[OnTime]", size, data, runtime) &&
                  size == 17u && data == allocated &&
                  runtime.calls == std::vector<std::string>{
                      "open", "read-size", "allocate", "read-data", "close"},
              "successful context follows exact read and close order");
    }

    Runtime empty;
    empty.read_size = 0u;
    std::uint32_t size = 9u;
    std::uint32_t data = 0u;
    Check(pal4::CsbBufferReader::Read(
              "TimeScript[OnTime]", size, data, empty) &&
              size == 0u && data == 0x9000u,
          "zero-length payload still allocates, reads and succeeds");
}

}  // namespace

int main() {
    TestOpenFailure();
    TestReadSequence();
}
