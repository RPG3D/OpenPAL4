#include "pal4/script_enum_string_assignment.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IScriptEnumStringRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool allocation_succeeds = true;
    bool detach_clears = false;
    std::uint32_t detach_result = 0x1234u;

    [[noreturn]] void ThrowRange() override {
        calls.push_back("range");
        throw 1;
    }
    std::uint32_t DetachAndReset(
        pal4::ScriptEnumStringState& state) override {
        calls.push_back("detach");
        if (detach_clears) state.text = {};
        return detach_result;
    }
    bool AllocateText(pal4::ScriptEnumStringState& state,
                      std::uint32_t length, std::uint8_t mode) override {
        calls.push_back("alloc:" + std::to_string(length) + ":" +
                        std::to_string(mode));
        if (allocation_succeeds) {
            state.text.data = 0x8000u;
            state.text.capacity = length;
        }
        return allocation_succeeds;
    }
    void SetTextSize(pal4::ScriptEnumStringState& state,
                     std::uint32_t length) override {
        calls.push_back("size:" + std::to_string(length));
        state.text.length = length;
    }
    std::uint32_t EmptyStringAddress() override {
        calls.push_back("empty");
        return 0x7000u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return bytes.count(address) != 0u ? bytes.at(address) : 0xffu;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        bytes[address] = value;
    }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t length) override {
        calls.push_back("copy:" + std::to_string(destination) + ":" +
                        std::to_string(source) + ":" +
                        std::to_string(length));
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
};

void TestSharedAssignment() {
    pal4::ScriptEnumStringState destination{0x55u, {0x5000u, 2u, 3u}};
    const pal4::ScriptEnumStringState source{0x66u, {0x3000u, 4u, 8u}};
    Runtime runtime;
    runtime.bytes[0x2fffu] = 2u;
    runtime.bytes[0x4fffu] = 1u;
    Check(pal4::ScriptEnumStringAssignment::Assign(
              destination, source, runtime) == 3u &&
              destination.status == 0x55u &&
              destination.text.data == 0x3000u &&
              destination.text.length == 4u &&
              destination.text.capacity == 8u &&
              runtime.bytes[0x2fffu] == 3u &&
              runtime.bytes[0x4fffu] == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read:12287", "read:20479", "write:20479:0",
                  "read:12287", "write:12287:3"},
          "share path decrements old reference and increments new reference");

    destination = {0x55u, {0x5000u, 2u, 3u}};
    runtime.calls.clear();
    runtime.bytes[0x4fffu] = 0u;
    (void)pal4::ScriptEnumStringAssignment::Assign(
        destination, source, runtime);
    Check(runtime.calls[2] == "free:20479",
          "zero old reference frees the original header");
}

void TestCopyAndFailure() {
    pal4::ScriptEnumStringState destination{0x11u, {0x5000u, 2u, 4u}};
    const pal4::ScriptEnumStringState source{0x22u, {0x3000u, 4u, 5u}};
    Runtime runtime;
    runtime.bytes[0x2fffu] = 0xfeu;
    Check(pal4::ScriptEnumStringAssignment::Assign(
              destination, source, runtime) == 4u &&
              destination.text.data == 0x8000u &&
              destination.text.length == 4u &&
              runtime.calls == std::vector<std::string>{
                  "read:12287", "alloc:4:1", "copy:32768:12288:4",
                  "write:32772:0"},
          "high source reference byte forces copying and null termination");

    destination = {0x11u, {0x5000u, 2u, 4u}};
    runtime.calls.clear();
    runtime.allocation_succeeds = false;
    Check(pal4::ScriptEnumStringAssignment::Assign(
              destination, source, runtime) == 0u &&
              destination.text.data == 0x5000u &&
              destination.text.length == 2u &&
              runtime.calls == std::vector<std::string>{
                  "read:12287", "alloc:4:1"},
          "copy allocation failure leaves the existing state intact");

    const pal4::ScriptEnumStringState empty{0u, {0u, 0u, 0u}};
    runtime.calls.clear();
    runtime.allocation_succeeds = true;
    Check(pal4::ScriptEnumStringAssignment::Assign(
              destination, empty, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "alloc:0:1", "empty", "copy:32768:28672:0",
                  "write:32768:0"},
          "empty string fallback is read only after successful allocation");
}

void TestSelfAssignment() {
    pal4::ScriptEnumStringState state{0u, {0x3000u, 3u, 5u}};
    Runtime runtime;
    Check(pal4::ScriptEnumStringAssignment::Assign(state, state, runtime) ==
              0x34u &&
              runtime.calls == std::vector<std::string>{
                  "detach", "detach"},
          "self-assignment invokes the detach routine twice");

    state = {0u, {0x3000u, 3u, 5u}};
    runtime.calls.clear();
    runtime.detach_clears = true;
    Check(pal4::ScriptEnumStringAssignment::Assign(state, state, runtime) ==
              0x34u &&
              runtime.calls == std::vector<std::string>{
                  "detach", "copy:3:0:0", "alloc:3:0", "size:3",
                  "detach"},
          "self-detach length change follows the original unsigned arithmetic");
}

}  // namespace

int main() {
    TestSharedAssignment();
    TestCopyAndFailure();
    TestSelfAssignment();
    pal4::ScriptEnumStringState state;
    pal4::ScriptEnumStringRuntimeStub stub;
    Check(pal4::ScriptEnumStringAssignment::Assign(state, state, stub) == 0u,
          "default string assignment runtime links");
    std::cout << "4 script enum string cases passed\n";
}
