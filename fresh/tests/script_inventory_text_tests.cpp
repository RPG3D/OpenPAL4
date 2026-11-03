#include "pal4/script_inventory_text.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IScriptInventoryTextRuntime {
    std::string buffer;
    std::string description = "Sword";
    std::vector<bool> allocation_results;
    std::size_t allocation_index = 0u;
    std::vector<std::string> calls;
    std::int32_t value = 0;
    std::uint32_t entry = 0u;
    pal4::InventoryDescription record{0x2000u, 5u};

    void SetErrorFlag(pal4::ScriptInventoryTextState&,
                      std::uint8_t flag) override {
        calls.push_back("flag:" + std::to_string(flag));
    }
    void RegisterAtExit(std::uint32_t callback) override {
        calls.push_back("exit:" + std::to_string(callback));
    }
    std::int32_t InvokeValue(std::uint32_t callback_object,
                             std::uint32_t argument) override {
        calls.push_back("invoke:" + std::to_string(callback_object) + ":" +
                        std::to_string(argument));
        return value;
    }
    std::uint32_t FindEntry(std::int32_t query,
                            std::uint32_t category) override {
        calls.push_back("find:" + std::to_string(query) + ":" +
                        std::to_string(category));
        return entry;
    }
    pal4::InventoryDescription GetDescription(
        std::uint32_t address) override {
        calls.push_back("description:" + std::to_string(address));
        return record;
    }
    bool AllocateText(pal4::StringBufferState& text,
                      std::uint32_t length, std::uint8_t mode) override {
        calls.push_back("alloc:" + std::to_string(length) + ":" +
                        std::to_string(mode));
        const auto success = allocation_index < allocation_results.size()
            ? allocation_results[allocation_index++] : true;
        if (success) {
            text.data = 0x1000u;
            text.capacity = length;
            if (mode == 1u) buffer.resize(length);
            else if (buffer.size() < length) buffer.resize(length);
        }
        return success;
    }
    void CopyLiteral(std::uint32_t destination, const char* source,
                     std::uint32_t length) override {
        calls.push_back("literal:" + std::string(source, length));
        buffer.replace(destination - 0x1000u, length, source, length);
    }
    void CopyDescription(std::uint32_t destination, std::uint32_t source,
                         std::uint32_t length) override {
        calls.push_back("copy:" + std::to_string(source) + ":" +
                        std::to_string(length));
        buffer.replace(destination - 0x1000u, length,
                       description.data(), length);
    }
    void SetTextSize(pal4::StringBufferState& text,
                     std::uint32_t length) override {
        calls.push_back("size:" + std::to_string(length));
        text.length = length;
        buffer.resize(length);
    }
    [[noreturn]] void ThrowLength() override {
        calls.push_back("overflow");
        throw std::length_error("length");
    }
};

struct ValueRuntime final : pal4::IScriptEventValueRuntime {
    const char* source = nullptr;
    std::uint32_t read_argument = 0u;
    std::uint32_t invoked_callback = 0u;
    std::int32_t invoked_value = 0;

    const char* ReadCString(std::uint32_t argument_record) override {
        read_argument = argument_record;
        return source;
    }
    std::int32_t InvokeParsedValue(std::uint32_t callback_object,
                                   std::int32_t value) override {
        invoked_callback = callback_object;
        invoked_value = value;
        return value + 100;
    }
};

struct CleanupRuntime final : pal4::IScriptInventoryTextCleanupRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        bytes[address] = value;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
};

void TestMissingEntryAndGuard() {
    pal4::ScriptInventoryTextState state;
    state.guard = 0x80u;
    Runtime runtime;
    Check(pal4::ScriptInventoryText::Format(state, 0u, 0x1234u, runtime) ==
              pal4::ScriptInventoryText::kBufferAddress &&
              state.guard == 0x81u && state.first_byte == 0x34u &&
              runtime.buffer == "0..." &&
              runtime.calls == std::vector<std::string>{
                  "flag:0", "exit:4385776", "find:0:7", "alloc:1:1",
                  "literal:0", "size:1", "alloc:4:0",
                  "literal:...", "size:4"},
          "null value callback formats zero and missing record as ellipsis");
    const auto old_count = runtime.calls.size();
    (void)pal4::ScriptInventoryText::Format(state, 0u, 0x77u, runtime);
    Check(runtime.calls.size() == old_count + 7u &&
              state.first_byte == 0x34u && runtime.buffer == "0...",
          "repeat formatting retains the original global registration byte");
}

void TestDescriptionAndAllocationFailures() {
    pal4::ScriptInventoryTextState state;
    Runtime runtime;
    runtime.entry = 0x3000u;
    runtime.value = -42;
    Check(pal4::ScriptInventoryText::Format(state, 0x9000u, 0x55u,
                                             runtime) ==
              pal4::ScriptInventoryText::kBufferAddress &&
              runtime.buffer == "-42 Sword" && state.text.length == 9u &&
              runtime.calls[2] == "invoke:36864:85" &&
              runtime.calls[3] == "find:-42:7" &&
              runtime.calls[10] == "description:12288" &&
              runtime.calls[12] == "copy:8192:5",
          "value callback, category lookup and record text use observed order");

    pal4::ScriptInventoryTextState failed_state;
    failed_state.guard = 1u;
    failed_state.text = {0x1000u, 3u, 8u};
    Runtime failed_runtime;
    failed_runtime.buffer = "old";
    failed_runtime.allocation_results = {false, true};
    (void)pal4::ScriptInventoryText::Format(
        failed_state, 0u, 0u, failed_runtime);
    Check(failed_runtime.buffer == "old..." &&
              failed_state.text.length == 6u,
          "failed decimal replacement leaves old text for the append path");

    pal4::ScriptInventoryTextState partial_state;
    partial_state.guard = 1u;
    Runtime partial_runtime;
    partial_runtime.entry = 0x3000u;
    partial_runtime.value = 5;
    partial_runtime.allocation_results = {true, false, true};
    (void)pal4::ScriptInventoryText::Format(
        partial_state, 0x9000u, 0u, partial_runtime);
    Check(partial_runtime.buffer == "5Sword" &&
              partial_state.text.length == 6u,
          "failed space append does not prevent description append");

    pal4::ScriptInventoryTextState last_state;
    last_state.guard = 1u;
    Runtime last_runtime;
    last_runtime.entry = 0x3000u;
    last_runtime.value = 5;
    last_runtime.allocation_results = {true, true, false};
    (void)pal4::ScriptInventoryText::Format(
        last_state, 0x9000u, 0u, last_runtime);
    Check(last_runtime.buffer == "5 " && last_state.text.length == 2u &&
              last_runtime.calls.back() == "alloc:7:0",
          "failed description allocation leaves the decimal and space intact");
}

void TestZeroLengthAndOverflow() {
    pal4::ScriptInventoryTextState state;
    state.guard = 1u;
    Runtime runtime;
    runtime.entry = 0x3000u;
    runtime.value = 8;
    runtime.record.length = 0u;
    (void)pal4::ScriptInventoryText::Format(state, 0x9000u, 0u, runtime);
    Check(runtime.buffer == "8 " &&
              runtime.calls.back() == "description:12288",
          "zero-length description does not request text allocation");

    pal4::ScriptInventoryTextState overflow_state;
    overflow_state.guard = 1u;
    overflow_state.text.length =
        std::numeric_limits<std::uint32_t>::max() - 2u;
    Runtime overflow_runtime;
    overflow_runtime.allocation_results = {false};
    bool threw = false;
    try {
        (void)pal4::ScriptInventoryText::Format(
            overflow_state, 0u, 0u, overflow_runtime);
    } catch (const std::length_error&) {
        threw = true;
    }
    Check(threw && overflow_runtime.calls.back() == "overflow",
          "append checks unsigned maximum even after initial allocation fails");
}

void TestParsedValueForwarding() {
    ValueRuntime runtime;
    runtime.source = "  -37suffix";
    Check(pal4::ScriptInventoryText::ForwardParsedValue(
              0u, 0x1234u, runtime) == -37 &&
              runtime.read_argument == 0x1234u &&
              runtime.invoked_callback == 0u,
          "without callback, atoi result remains in the return register");
    Check(pal4::ScriptInventoryText::ForwardParsedValue(
              0x5000u, 0x2000u, runtime) == 63 &&
              runtime.invoked_callback == 0x5000u &&
              runtime.invoked_value == -37,
          "non-null callback receives the parsed signed integer");
    runtime.source = nullptr;
    Check(pal4::ScriptInventoryText::ForwardParsedValue(
              0x5000u, 0x2000u, runtime) == 100 &&
              runtime.invoked_value == 0,
          "null string data falls back to an empty C string");
}

void TestGlobalCleanup() {
    pal4::ScriptInventoryTextState state{0x77u, 0x81u,
                                         {0x3000u, 4u, 8u}};
    CleanupRuntime runtime;
    runtime.bytes[0x2fffu] = 2u;
    pal4::ScriptInventoryText::CleanupGlobal(state, runtime);
    Check(state.first_byte == 0x77u && state.guard == 0x81u &&
              state.text.data == 0u && state.text.length == 0u &&
              state.text.capacity == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read:12287", "write:12287:1"},
          "global cleanup decrements a shared string and clears its fields");

    state.text = {0x3000u, 4u, 8u};
    runtime.calls.clear();
    runtime.bytes[0x2fffu] = 0xffu;
    pal4::ScriptInventoryText::CleanupGlobal(state, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "read:12287", "free:12287"},
          "sentinel reference frees the allocation header");

    runtime.calls.clear();
    pal4::ScriptInventoryText::CleanupGlobal(state, runtime);
    Check(runtime.calls.empty(), "empty global cleanup only resets fields");
}

}  // namespace

int main() {
    TestMissingEntryAndGuard();
    TestDescriptionAndAllocationFailures();
    TestZeroLengthAndOverflow();
    TestParsedValueForwarding();
    TestGlobalCleanup();
    pal4::ScriptInventoryTextState state;
    pal4::ScriptInventoryTextRuntimeStub stub;
    Check(pal4::ScriptInventoryText::Format(state, 0u, 0u, stub) ==
              pal4::ScriptInventoryText::kBufferAddress,
          "default inventory text runtime links");
    pal4::ScriptEventValueRuntimeStub value_stub;
    Check(pal4::ScriptInventoryText::ForwardParsedValue(
              0u, 0u, value_stub) == 0,
          "default parsed value runtime links");
    pal4::ScriptInventoryTextCleanupRuntimeStub cleanup_stub;
    pal4::ScriptInventoryText::CleanupGlobal(state, cleanup_stub);
    std::cout << "6 script inventory text cases passed\n";
}
