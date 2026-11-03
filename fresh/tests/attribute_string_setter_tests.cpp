#include "pal4/attribute_string_setter.h"

#include <bit>
#include <cstdlib>
#include <iostream>
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

struct Runtime final : pal4::IAttributeStringSetterRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t source{0x3000u};
    std::uint32_t length{3u};
    std::uint32_t allocated_data{0x4000u};
    std::uint8_t allocate_result{1u};
    std::uint32_t cleanup_result{0x79u};
    std::uint32_t free_result{0x88u};
    bool throw_dispatch{};
    std::uint32_t seen_flag{};
    pal4::StringBufferState seen_text{};
    std::uint32_t formatted_buffer{};
    std::uint32_t formatted_count{};
    std::string formatted_pattern;
    double formatted_value{};

    std::uint32_t CStringLength(std::uint32_t address) override {
        calls.push_back("length:" + std::to_string(address));
        return length;
    }
    std::int32_t FormatFloat(std::uint32_t buffer,
                              std::uint32_t count, const char* pattern,
                              double value) override {
        calls.push_back("format-float");
        formatted_buffer = buffer;
        formatted_count = count;
        formatted_pattern = pattern;
        formatted_value = value;
        return 4;
    }
    std::uint8_t AllocateText(pal4::StringBufferState& text,
                              std::uint32_t size,
                              std::uint8_t mode) override {
        calls.push_back("allocate:" + std::to_string(size) + ":" +
                        std::to_string(mode));
        if (allocate_result != 0u) {
            text.data = allocated_data;
            text.capacity = size;
        }
        return allocate_result;
    }
    void Copy(std::uint32_t destination, std::uint32_t address,
              std::uint32_t size) override {
        calls.push_back("copy:" + std::to_string(destination) + ":" +
                        std::to_string(address) + ":" +
                        std::to_string(size));
        for (std::uint32_t i = 0u; i < size; ++i)
            bytes[destination + i] = bytes.at(address + i);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        bytes[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return bytes.at(address);
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return free_result;
    }
    void DispatchTemporary(
        std::uint32_t target, std::uint32_t key,
        pal4::AttributeTemporaryStringState& temporary,
        std::uint32_t slot) override {
        calls.push_back("dispatch-temporary:" + std::to_string(target) +
                        ":" + std::to_string(key) + ":" +
                        std::to_string(slot));
        seen_flag = temporary.flag_word;
        seen_text = temporary.text;
        if (throw_dispatch) throw std::runtime_error("dispatch");
    }
    void InitializeString(pal4::StringBufferState& text,
                          std::uint32_t address) override {
        calls.push_back("initialize:" + std::to_string(address));
        text = {allocated_data, length, length + 1u};
    }
    void DispatchString(std::uint32_t target, std::uint32_t key,
                        pal4::StringBufferState& text,
                        std::uint32_t slot) override {
        calls.push_back("dispatch-string:" + std::to_string(target) +
                        ":" + std::to_string(key) + ":" +
                        std::to_string(slot));
        seen_text = text;
        if (throw_dispatch) throw std::runtime_error("dispatch");
    }
    std::uint32_t CleanupString(pal4::StringBufferState& text) override {
        calls.push_back("cleanup-string");
        text = {};
        return cleanup_result;
    }
};

void TestTemporaryString() {
    Runtime runtime;
    runtime.bytes[0x3000u] = 'a';
    runtime.bytes[0x3001u] = 'b';
    runtime.bytes[0x3002u] = 'c';
    runtime.bytes[0x3fffu] = 2u;
    const pal4::AttributeValueState attribute{0u, 0x3000u, 0x55u};
    Check(pal4::AttributeStringSetter::SetString(
              attribute, 0x12abu, 0x12345600u, runtime) == 0x3fffu &&
              runtime.seen_flag == 0x123456abu &&
              runtime.seen_text.data == 0x4000u &&
              runtime.seen_text.length == 3u &&
              runtime.seen_text.capacity == 3u &&
              runtime.bytes.at(0x4000u) == 'a' &&
              runtime.bytes.at(0x4002u) == 'c' &&
              runtime.bytes.at(0x4003u) == 0u &&
              runtime.bytes.at(0x3fffu) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "length:12288", "allocate:3:1",
                  "copy:16384:12288:3", "write:16387:0",
                  "dispatch-temporary:4779:85:20", "read:16383",
                  "write:16383:1"},
          "temporary record copies text, forwards flag and decrements header");
}

void TestReleaseAndFailure() {
    const pal4::AttributeValueState attribute{0u, 0x3000u, 0x55u};
    for (const std::uint8_t reference : {0u, 0xffu}) {
        Runtime runtime;
        runtime.length = 0u;
        runtime.bytes[0x3fffu] = reference;
        Check(pal4::AttributeStringSetter::SetString(
                  attribute, 0x1000u, 0u, runtime) == 0x88u &&
                  runtime.calls.back() == "free:16383",
              "zero and ff headers call native free and forward its result");
    }

    Runtime failure;
    failure.allocate_result = 0u;
    Check(pal4::AttributeStringSetter::SetString(
              attribute, 0x1000u, 0u, failure) == 0u &&
              failure.seen_text.data == 0u &&
              failure.calls == std::vector<std::string>{
                  "length:12288", "allocate:3:1",
                  "dispatch-temporary:4096:85:20"},
          "allocation failure still dispatches empty temporary and returns null");

    Runtime throwing;
    throwing.length = 0u;
    throwing.bytes[0x3fffu] = 1u;
    throwing.throw_dispatch = true;
    bool caught = false;
    try {
        (void)pal4::AttributeStringSetter::SetString(
            attribute, 0x1000u, 0u, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.bytes.at(0x3fffu) == 0u &&
              throwing.calls.back() == "write:16383:0",
          "temporary is released on virtual dispatch exception");
}

void TestStringBufferPath() {
    Runtime runtime;
    const pal4::AttributeValueState attribute{0u, 0x3000u, 0x55u};
    Check(pal4::AttributeStringSetter::SetStringFromString(
              attribute, 0x1000u, runtime) == 0x79u &&
              runtime.seen_text.data == 0x4000u &&
              runtime.seen_text.length == 3u &&
              runtime.calls == std::vector<std::string>{
                  "initialize:12288", "dispatch-string:4096:85:20",
                  "cleanup-string"},
          "string buffer initializer, virtual setter and cleanup order");

    runtime.calls.clear();
    runtime.throw_dispatch = true;
    bool caught = false;
    try {
        (void)pal4::AttributeStringSetter::SetStringFromString(
            attribute, 0x1000u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() == "cleanup-string",
          "string buffer is cleaned after virtual setter exception");

    pal4::AttributeStringSetterRuntimeStub stub;
    Check(pal4::AttributeStringSetter::SetStringFromString(
              attribute, 0u, stub) == 0u,
          "native string boundary has a linkable stub");
}

void TestFloatFormattingPaths() {
    Runtime runtime;
    const auto buffer = pal4::AttributeStringSetter::kFormatBuffer;
    runtime.bytes[buffer] = '1';
    runtime.bytes[buffer + 1u] = '.';
    runtime.bytes[buffer + 2u] = '5';
    runtime.bytes[0x3fffu] = 1u;
    const pal4::AttributeValueState attribute{
        0u, std::bit_cast<std::uint32_t>(1.5f), 0x55u};
    Check(pal4::AttributeStringSetter::SetFloatToString(
              attribute, 0x12abu, 0u, runtime) == 0x3fffu &&
              runtime.calls.front() == "format-float" &&
              runtime.calls[1] ==
                  "length:" + std::to_string(buffer) &&
              runtime.formatted_buffer == buffer &&
              runtime.formatted_count == 0x1ffu &&
              runtime.formatted_pattern == "%f" &&
              runtime.formatted_value == 1.5 &&
              runtime.seen_flag == 0xabu &&
              runtime.bytes.at(0x4002u) == '5',
          "first float setter formats before copying 16-byte temporary");

    runtime.calls.clear();
    Check(pal4::AttributeStringSetter::SetFloatToStringBuffer(
              attribute, 0x1000u, runtime) == 0x79u &&
              runtime.calls == std::vector<std::string>{
                  "format-float",
                  "initialize:" + std::to_string(buffer),
                  "dispatch-string:4096:85:20", "cleanup-string"},
          "second float setter formats before 12-byte string lifecycle");
}

}  // namespace

int main() {
    TestTemporaryString();
    TestReleaseAndFailure();
    TestStringBufferPath();
    TestFloatFormattingPaths();
}
