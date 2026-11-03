#include "pal4/compact_string_buffer.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <new>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICompactStringRuntime {
    std::map<std::uint32_t, std::uint8_t> memory;
    std::vector<std::string> calls;
    std::uint32_t next{0x5000u};
    bool fail_allocate{};

    Runtime() {
        WriteWord(pal4::CompactStringBuffer::kEmptyBlock, 0u);
        WriteWord(pal4::CompactStringBuffer::kEmptyBlock + 4u, 0u);
        WriteByte(pal4::CompactStringBuffer::kEmptyBlock + 8u, 0u);
        calls.clear();
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        if (fail_allocate) return 0u;
        const auto result = next;
        next += 0x100u;
        return result;
    }
    std::uint32_t Free(const std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return 0xdeadu;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return static_cast<std::uint32_t>(memory[address]) |
               (static_cast<std::uint32_t>(memory[address + 1u]) << 8u) |
               (static_cast<std::uint32_t>(memory[address + 2u]) << 16u) |
               (static_cast<std::uint32_t>(memory[address + 3u]) << 24u);
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        calls.push_back("byte:" + std::to_string(address));
        return memory[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        for (std::uint32_t i = 0u; i < 4u; ++i)
            memory[address + i] = static_cast<std::uint8_t>(value >> (i * 8u));
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        memory[address] = value;
    }
    void Copy(const std::uint32_t destination, const std::uint32_t source,
              const std::uint32_t bytes) override {
        calls.push_back("copy:" + std::to_string(destination) + ":" +
                        std::to_string(source) + ":" + std::to_string(bytes));
        std::vector<std::uint8_t> data;
        for (std::uint32_t i = 0u; i < bytes; ++i)
            data.push_back(memory[source + i]);
        for (std::uint32_t i = 0u; i < bytes; ++i)
            memory[destination + i] = data[i];
    }
    void Text(const std::uint32_t address, const std::string& text) {
        for (std::size_t i = 0u; i < text.size(); ++i)
            memory[address + static_cast<std::uint32_t>(i)] =
                static_cast<std::uint8_t>(text[i]);
    }
    std::string TextAt(const std::uint32_t address,
                       const std::uint32_t length) {
        std::string result;
        for (std::uint32_t i = 0u; i < length; ++i)
            result.push_back(static_cast<char>(memory[address + i]));
        return result;
    }
};

void TestInitializeAndReallocate() {
    Runtime runtime;
    pal4::CompactStringBuffer::Initialize(0x1000u, 0u, 0u, runtime);
    Check(runtime.ReadWord(0x1000u) ==
              pal4::CompactStringBuffer::kEmptyBlock &&
              runtime.calls.empty(),
          "zero capacity uses shared empty block without allocation");
    Check(pal4::CompactStringBuffer::Reallocate(0x1000u, 0u, runtime) ==
              pal4::CompactStringBuffer::kEmptyBlock,
          "empty block remains unchanged when capacity is sufficient");

    pal4::CompactStringBuffer::Initialize(0x1100u, 3u, 10u, runtime);
    const auto old_block = runtime.ReadWord(0x1100u);
    Check(old_block == 0x5000u && runtime.ReadWord(old_block) == 3u &&
              runtime.ReadWord(old_block + 4u) == 10u &&
              runtime.memory[old_block + 11u] == 0u &&
              runtime.calls.front() == "allocate:24",
          "initialize writes compact header and trailing terminator");
    runtime.Text(old_block + 8u, "abc");
    runtime.calls.clear();
    Check(pal4::CompactStringBuffer::Reallocate(0x1100u, 8u, runtime) ==
              old_block && runtime.calls.empty(),
          "reallocate below capacity is a no-op returning old block");
    Check(pal4::CompactStringBuffer::Reallocate(0x1100u, 20u, runtime) ==
              0xdeadu && runtime.ReadWord(0x1100u) == 0x5100u &&
              runtime.ReadWord(0x5100u) == 3u &&
              runtime.ReadWord(0x5104u) == 20u &&
              runtime.TextAt(0x5108u, 3u) == "abc" &&
              runtime.calls == std::vector<std::string>{
                  "allocate:32", "copy:20744:20488:3", "free:20480"},
          "growth copies old bytes, swaps block and returns free result");
}

void TestAssignAndAppend() {
    Runtime runtime;
    pal4::CompactStringBuffer::Initialize(0x1000u, 0u, 100u, runtime);
    runtime.Text(0x2000u, "x");
    runtime.calls.clear();
    Check(pal4::CompactStringBuffer::Assign(
              0x1000u, 0x2000u, 1u, runtime) == 0x1000u &&
              runtime.ReadWord(0x1000u) == 0x5100u &&
              runtime.ReadWord(0x5100u) == 1u &&
              runtime.ReadWord(0x5104u) == 1u &&
              runtime.TextAt(0x5108u, 1u) == "x" &&
              runtime.calls == std::vector<std::string>{
                  "allocate:16", "copy:20744:8192:1", "free:20480"},
          "assign shrinks excessive capacity into a new block");
    runtime.Text(0x2010u, "abc");
    runtime.calls.clear();
    Check(pal4::CompactStringBuffer::Assign(
              0x1000u, 0x2010u, 3u, runtime) == 0x1000u &&
              runtime.ReadWord(0x1000u) == 0x5200u &&
              runtime.ReadWord(0x5204u) == 3u &&
              runtime.TextAt(0x5208u, 3u) == "abc",
          "assign grows when requested size exceeds capacity");
    runtime.Text(0x2020u, "de");
    runtime.calls.clear();
    Check(pal4::CompactStringBuffer::Append(
              0x1000u, 0x2020u, 2u, runtime) == 0x1000u &&
              runtime.ReadWord(0x1000u) == 0x5300u &&
              runtime.ReadWord(0x5300u) == 5u &&
              runtime.ReadWord(0x5304u) == 8u &&
              runtime.TextAt(0x5308u, 5u) == "abcde" &&
              runtime.memory[0x530du] == 0u,
          "append grows by new length plus old capacity and terminates");
    runtime.Text(0x2030u, "f");
    runtime.calls.clear();
    Check(pal4::CompactStringBuffer::Append(
              0x1000u, 0x2030u, 1u, runtime) == 0x1000u &&
              runtime.ReadWord(0x1000u) == 0x5300u &&
              runtime.TextAt(0x5308u, 6u) == "abcdef" &&
              runtime.calls == std::vector<std::string>{
                  "copy:21261:8240:1"},
          "append within capacity copies in place");
    runtime.calls.clear();
    Check(pal4::CompactStringBuffer::Assign(
              0x1000u, 0x2000u, 0u, runtime) == 0x1000u &&
              runtime.ReadWord(0x1000u) == 0x5300u &&
              runtime.ReadWord(0x5300u) == 0u &&
              runtime.memory[0x5308u] == 0u &&
              runtime.calls == std::vector<std::string>{
                  "copy:21256:8192:0"},
          "zero-length assignment reuses a small block");
    pal4::CompactStringBuffer::Initialize(0x1100u, 0u, 100u, runtime);
    runtime.calls.clear();
    Check(pal4::CompactStringBuffer::Assign(
              0x1100u, 0x2000u, 0u, runtime) == 0x1100u &&
              runtime.ReadWord(0x1100u) ==
                  pal4::CompactStringBuffer::kEmptyBlock &&
              runtime.calls.back() == "free:21504",
          "zero-length assignment releases excessive capacity");
}

void TestAllocationFailure() {
    Runtime runtime;
    runtime.fail_allocate = true;
    bool caught = false;
    try {
        pal4::CompactStringBuffer::Initialize(0x1000u, 1u, 1u, runtime);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && runtime.calls ==
              std::vector<std::string>{"allocate:16"},
          "failed allocation leaves object untouched");
    pal4::CompactStringRuntimeStub stub;
    Check(stub.Allocate(1u) == 0u && stub.Free(1u) == 0u &&
              stub.ReadWord(1u) == 0u,
          "compact string native boundary has a linkable stub");
}

void TestFindChar() {
    Runtime runtime;
    const auto object = 0x1000u;
    const auto block = 0x5000u;
    runtime.WriteWord(object, block);
    runtime.WriteWord(block, 6u);
    runtime.Text(block + 8u, "abacad");
    runtime.WriteByte(block + 14u, 0u);
    Check(pal4::CompactStringBuffer::FindChar(object, 'a', 0u, runtime) == 0u &&
              pal4::CompactStringBuffer::FindChar(object, 'a', 1u, runtime) == 2u &&
              pal4::CompactStringBuffer::FindChar(object, 'd', 3u, runtime) == 5u,
          "find character starts at requested offset and returns first match");
    runtime.calls.clear();
    Check(pal4::CompactStringBuffer::FindChar(object, 'a', 6u, runtime) ==
              0xffffffffu && runtime.calls.empty(),
          "out-of-range start does not read bytes");
    runtime.WriteByte(block + 10u, 0u);
    runtime.calls.clear();
    Check(pal4::CompactStringBuffer::FindChar(object, 'd', 0u, runtime) ==
              0xffffffffu &&
              runtime.calls == std::vector<std::string>{
                  "byte:20488", "byte:20489", "byte:20490"},
          "search stops at the first zero byte before a later match");
    runtime.calls.clear();
    Check(pal4::CompactStringBuffer::FindChar(object, 0u, 0u, runtime) ==
              0xffffffffu,
          "zero byte is a terminator, not a match");
}

}  // namespace

int main() {
    TestInitializeAndReallocate();
    TestAssignAndAppend();
    TestAllocationFailure();
    TestFindChar();
}
