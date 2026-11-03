#include "pal4/string_buffer.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IStringDetachRuntime {
public:
    void ThrowLength() override {
        events.emplace_back("length-error");
    }
    void SetErrorFlag(pal4::StringBufferState& state,
                      std::uint8_t flag) override {
        events.emplace_back("error:" + std::to_string(flag));
        state = {};
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.emplace_back("allocate:" + std::to_string(bytes));
        allocation_size = bytes;
        return allocation_base;
    }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t length) override {
        events.emplace_back("copy:" + std::to_string(length));
        std::vector<std::uint8_t> copied;
        for (std::uint32_t i = 0u; i < length; ++i) {
            copied.push_back(bytes.at(source + i));
        }
        for (std::uint32_t i = 0u; i < length; ++i) {
            bytes[destination + i] = copied[i];
        }
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        events.emplace_back("write:" + std::to_string(address));
        bytes[address] = value;
    }
    void Free(std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
    }

    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::uint32_t allocation_base = 0x5000u;
    std::uint32_t allocation_size = 0u;
};

void CheckCopyAndOldRelease() {
    Runtime runtime;
    runtime.bytes[0x2000u] = 0u;
    runtime.bytes[0x2001u] = 'a';
    runtime.bytes[0x2002u] = 'b';
    runtime.bytes[0x2003u] = 'c';
    pal4::StringBuffer buffer({0x2001u, 3u, 8u});
    Check(buffer.ReallocateWithReferenceHeader(5u, runtime) ==
              0x5001u &&
              runtime.allocation_size == 33u &&
              runtime.bytes[0x5001u] == 'a' &&
              runtime.bytes[0x5002u] == 'b' &&
              runtime.bytes[0x5003u] == 'c' &&
              runtime.bytes[0x5000u] == 0u &&
              runtime.bytes[0x5004u] == 0u &&
              buffer.State().data == 0x5001u &&
              buffer.State().capacity == 31u &&
              buffer.State().length == 3u,
          "rounded allocation copies old bytes and writes both terminators");
    Check(runtime.events == std::vector<std::string>{
              "allocate:33", "copy:3", "read:8192", "free:8192",
              "write:20480", "write:20484"},
          "copy and old-header release precede new state writes");
}

void CheckSharedHeaderRetained() {
    Runtime runtime;
    runtime.bytes[0x2000u] = 3u;
    pal4::StringBuffer buffer({0x2001u, 0u, 8u});
    Check(buffer.ReallocateWithReferenceHeader(32u, runtime) ==
              0x5001u &&
              runtime.allocation_size == 65u &&
              runtime.bytes[0x2000u] == 2u &&
              buffer.State().capacity == 63u &&
              buffer.State().length == 0u &&
              runtime.events == std::vector<std::string>{
                  "allocate:65", "read:8192", "write:8192",
                  "write:20480", "write:20481"},
          "ordinary reference decrements without freeing the shared header");
}

void CheckEmptyAndOverflowAllocation() {
    Runtime runtime;
    pal4::StringBuffer below({0u, 0u, 0u});
    Check(below.ReallocateWithReferenceHeader(2u, runtime) ==
              0x5001u &&
              below.State().length == 0u &&
              below.State().capacity == 31u,
          "empty source retains zero length");

    runtime.events.clear();
    pal4::StringBuffer overflow({0u, 0u, 0u});
    Check(overflow.ReallocateWithReferenceHeader(0xfffffffdu,
                                                 runtime) == 0x5001u &&
              runtime.allocation_size == 0u &&
              overflow.State().capacity == 0xfffffffdu &&
              overflow.State().length == 0u,
          "signed negative allocation size becomes zero after cap fallback");
}

void CheckTruncatedCopyAndSharedRelease() {
    Runtime runtime;
    runtime.bytes[0x2000u] = 2u;
    for (std::uint32_t i = 0u; i < 40u; ++i) {
        runtime.bytes[0x2001u + i] = static_cast<std::uint8_t>('a' + i % 26u);
    }
    pal4::StringBuffer buffer({0x2001u, 40u, 63u});
    Check(buffer.ReallocateWithReferenceHeader(2u, runtime) == 0x5001u &&
              buffer.State().length == 31u &&
              buffer.State().capacity == 31u &&
              runtime.bytes[0x5001u] == 'a' &&
              runtime.bytes[0x501fu] == 'e' &&
              runtime.bytes[0x5020u] == 0u &&
              runtime.bytes[0x2000u] == 1u,
          "copy and final length clip to new capacity");
    Check(runtime.events == std::vector<std::string>{
              "allocate:33", "copy:31", "read:8192", "write:8192",
              "write:20480", "write:20512"},
          "truncated copy leaves shared header allocated");
}

void CheckDetachNoopBranches() {
    Runtime runtime;
    pal4::StringBuffer null_text({0u, 3u, 8u});
    Check(null_text.DetachAndReset(runtime) == 0u &&
              null_text.State().length == 3u && runtime.events.empty(),
          "null data exits without changing other fields");

    for (const std::uint8_t reference : {0u, 0xffu}) {
        runtime.events.clear();
        runtime.bytes[0x2000u] = reference;
        pal4::StringBuffer buffer({0x2001u, 3u, 8u});
        Check(buffer.DetachAndReset(runtime) == 0x2001u &&
                  buffer.State().data == 0x2001u &&
                  buffer.State().length == 3u &&
                  runtime.events == std::vector<std::string>{"read:8192"},
              "zero and ff references skip detach and preserve object");
    }
}

void CheckDetachSharedText() {
    Runtime runtime;
    runtime.bytes[0x2000u] = 3u;
    runtime.bytes[0x2001u] = 'a';
    runtime.bytes[0x2002u] = 'b';
    runtime.bytes[0x2003u] = 'c';
    runtime.bytes[0x2004u] = 0u;
    pal4::StringBuffer buffer({0x2001u, 9u, 16u});
    Check(buffer.DetachAndReset(runtime) == 3u &&
              runtime.bytes[0x2000u] == 2u &&
              buffer.State().data == 0x5001u &&
              buffer.State().length == 3u &&
              buffer.State().capacity == 31u &&
              runtime.bytes[0x5001u] == 'a' &&
              runtime.bytes[0x5002u] == 'b' &&
              runtime.bytes[0x5003u] == 'c' &&
              runtime.bytes[0x5004u] == 0u,
          "shared text detaches using strlen, new allocation, copy and size");
    Check(runtime.events[0] == "read:8192" &&
              runtime.events[1] == "write:8192" &&
              runtime.events[6] == "error:1" &&
              runtime.events[7] == "allocate:33",
          "reference decrement and text scan precede allocation");
}

void CheckDetachEmptySharedText() {
    Runtime runtime;
    runtime.bytes[0x2000u] = 2u;
    runtime.bytes[0x2001u] = 0u;
    pal4::StringBuffer buffer({0x2001u, 8u, 16u});
    Check(buffer.DetachAndReset(runtime) == 0u &&
              runtime.bytes[0x2000u] == 1u &&
              buffer.State().data == 0u &&
              buffer.State().length == 0u &&
              buffer.State().capacity == 0u &&
              runtime.events == std::vector<std::string>{
                  "read:8192", "write:8192", "read:8193",
                  "error:1"},
          "empty shared text resets without allocating");
}

}  // namespace

int main() {
    CheckCopyAndOldRelease();
    CheckSharedHeaderRetained();
    CheckEmptyAndOverflowAllocation();
    CheckTruncatedCopyAndSharedRelease();
    CheckDetachNoopBranches();
    CheckDetachSharedText();
    CheckDetachEmptySharedText();
    std::cout << "7 string buffer reallocation/detach test cases passed\n";
}
