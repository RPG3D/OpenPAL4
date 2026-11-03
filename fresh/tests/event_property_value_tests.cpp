#include "pal4/event_property_value.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
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

struct Runtime final : pal4::IEventPropertyValueRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocation{0x5000u};
    std::uint32_t float_bits{0xbf800000u};
    std::int32_t integer{-7};
    std::uint32_t string_record{0x6000u};
    bool throw_on_enum{};

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        return allocation;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    void IncrementTempOff() override { calls.push_back("inc_tmpoff"); }
    void DecrementTempOff() override { calls.push_back("dec_tmpoff"); }
    std::uint32_t FloatBits(const std::uint32_t source) override {
        calls.push_back("float:" + std::to_string(source));
        return float_bits;
    }
    std::int32_t IntegerValue(const std::uint32_t source) override {
        calls.push_back("integer:" + std::to_string(source));
        return integer;
    }
    std::uint32_t StringRecord(const std::uint32_t source) override {
        calls.push_back("string:" + std::to_string(source));
        return string_record;
    }
    std::uint32_t SetValue(const std::uint32_t property,
                           const std::uint32_t key,
                           const std::uint32_t value) override {
        calls.push_back("set:" + std::to_string(property) + ":" +
                        std::to_string(key) + ":" +
                        std::to_string(value));
        return 0xdeadu;
    }
    void InitializeString(pal4::StringBufferState& temporary,
                          const std::uint32_t text) override {
        calls.push_back("init_string:" + std::to_string(text));
        temporary = {text, 4u, 5u};
    }
    void SetEnumValue(const std::uint32_t property,
                      const std::uint32_t key,
                      const pal4::StringBufferState& temporary) override {
        calls.push_back("set_enum:" + std::to_string(property) + ":" +
                        std::to_string(key) + ":" +
                        std::to_string(temporary.data) + ":" +
                        std::to_string(temporary.length));
        if (throw_on_enum) throw std::runtime_error("enum setter");
    }
    std::uint32_t CleanupString(
        pal4::StringBufferState& temporary) override {
        calls.push_back("cleanup_string:" +
                        std::to_string(temporary.data));
        temporary = {};
        return 0x999u;
    }
};

void TestConstructors() {
    using Method = std::uint32_t (*)(
        std::uint32_t, std::uint32_t,
        pal4::IEventPropertyValueRuntime&);
    const std::array<std::pair<Method, std::uint32_t>, 6> methods{{
        {&pal4::EventPropertyValue::CreateFloat, 0x8498d8u},
        {&pal4::EventPropertyValue::CreateInt, 0x8498e8u},
        {&pal4::EventPropertyValue::CreateString, 0x8498f8u},
        {&pal4::EventPropertyValue::CreateEnum, 0x849908u},
        {&pal4::EventPropertyValue::CreateBool, 0x849918u},
        {&pal4::EventPropertyValue::CreateUnknown, 0x849928u}}};
    for (const auto& [method, vtable] : methods) {
        Runtime runtime;
        Check(method(0x4000u, 0xabu, runtime) == 0xabu &&
                  runtime.words.at(0x4004u) == 0x5000u &&
                  runtime.words.at(0x5000u) == vtable &&
                  runtime.words.at(0x5004u) == 0xabu &&
                  runtime.calls == std::vector<std::string>{
                      "allocate:8", "write:20480:8689816", "inc_tmpoff",
                      "write:20480:" + std::to_string(vtable),
                      "write:20484:171", "write:16388:20480"},
              "property creator uses base vtable before specific vtable");
        runtime.allocation = 0u;
        runtime.calls.clear();
        Check(method(0x4000u, 0xcdu, runtime) == 0u &&
                  runtime.words.at(0x4004u) == 0u &&
                  runtime.calls == std::vector<std::string>{
                      "allocate:8", "write:16388:0"},
              "failed property allocation clears owner child pointer");
    }
}

void TestScalarSetters() {
    Runtime runtime;
    runtime.words[0x4004u] = 0x5000u;
    Check(pal4::EventPropertyValue::SetFloatFromFloat(
              0x4000u, 3u, 0x7000u, runtime) == 0xdeadu &&
              runtime.calls == std::vector<std::string>{
                  "read:16388", "float:28672", "set:20480:3:3212836864"},
          "float setter forwards raw narrowed bits through slot twenty");
    runtime.calls.clear();
    Check(pal4::EventPropertyValue::SetIntFromInt(
              0x4000u, 4u, 0x7000u, runtime) == 0xdeadu &&
              runtime.calls == std::vector<std::string>{
                  "read:16388", "integer:28672", "set:20480:4:4294967289"},
          "integer setter forwards signed bits unchanged");
    runtime.calls.clear();
    Check(pal4::EventPropertyValue::SetStringFromString(
              0x4000u, 5u, 0x7000u, runtime) == 0xdeadu &&
              runtime.calls == std::vector<std::string>{
                  "read:16388", "string:28672", "set:20480:5:24576"},
          "string setter forwards the source record address");
    runtime.calls.clear();
    Check(pal4::EventPropertyValue::SetBoolFromInt(
              0x4000u, 6u, 0x7000u, runtime) == 0xdeadu &&
              runtime.calls == std::vector<std::string>{
                  "read:16388", "integer:28672", "set:20480:6:28672"},
          "nonpositive integer clears only the source pointer low byte");
    runtime.integer = 2;
    runtime.calls.clear();
    Check(pal4::EventPropertyValue::SetBoolFromInt(
              0x4000u, 6u, 0x7000u, runtime) == 0xdeadu &&
              runtime.calls.back() == "set:20480:6:28673",
          "positive integer sets only the source pointer low byte");
    runtime.calls.clear();
    Check(pal4::EventPropertyValue::SetBoolFromInt(
              0x4000u, 6u, 0x70abu, runtime) == 0xdeadu &&
              runtime.calls.back() == "set:20480:6:28673",
          "bool setter discards the original pointer low byte");
}

void TestEnumAndBase() {
    Runtime runtime;
    runtime.words[0x4004u] = 0x5000u;
    runtime.words[0x6004u] = 0x7100u;
    Check(pal4::EventPropertyValue::SetEnumFromString(
              0x4000u, 9u, 0x7000u, runtime) == 0x999u &&
              runtime.calls == std::vector<std::string>{
                  "string:28672", "read:24580", "init_string:28928",
                  "read:16388", "set_enum:20480:9:28928:4",
                  "cleanup_string:28928"},
          "enum setter constructs temporary string and returns cleanup result");
    runtime.words[0x6004u] = 0u;
    runtime.calls.clear();
    Check(pal4::EventPropertyValue::SetEnumFromString(
              0x4000u, 10u, 0x7000u, runtime) == 0x999u &&
              runtime.calls[2] == "init_string:" + std::to_string(
                  pal4::StringBuffer::EmptyLogMessageAddress()),
          "enum setter uses shared empty text for null source pointer");
    runtime.throw_on_enum = true;
    runtime.calls.clear();
    bool caught = false;
    try {
        (void)pal4::EventPropertyValue::SetEnumFromString(
            0x4000u, 10u, 0x7000u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() ==
              "cleanup_string:" + std::to_string(
                  pal4::StringBuffer::EmptyLogMessageAddress()),
          "enum callback exception releases temporary string");

    runtime.calls.clear();
    Check(pal4::EventPropertyValue::GetType() == 3u,
          "property type query returns IDA constant three");
    pal4::EventPropertyValue::Cleanup(0x5000u, runtime);
    Check(runtime.words.at(0x5000u) == 0x849898u &&
              runtime.calls == std::vector<std::string>{
                  "write:20480:8689816", "dec_tmpoff"},
          "base cleanup restores vtable and decrements temp counter");

    pal4::EventPropertyValueRuntimeStub stub;
    Check(stub.Allocate(8u) == 0u && stub.ReadWord(0u) == 0u &&
              stub.FloatBits(0u) == 0u && stub.IntegerValue(0u) == 0 &&
              stub.StringRecord(0u) == 0u,
          "native property value boundary has a linkable stub");
}

}  // namespace

int main() {
    TestConstructors();
    TestScalarSetters();
    TestEnumAndBase();
}
