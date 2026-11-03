#include "pal4/event_property_binding.h"
#include "pal4/string_buffer.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEventPropertyBindingRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t word_value{0x6200u};
    double double_value{1.5};
    std::uint32_t name_record{0x6000u};
    std::uint32_t config_string{0x7400u};
    std::uint32_t parsed_integer{0x12345678u};

    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t ReadValue(const std::uint32_t source,
                            const std::uint32_t key,
                            const std::uint32_t slot) override {
        calls.push_back("value:" + std::to_string(source) + ":" +
                        std::to_string(key) + ":" +
                        std::to_string(slot));
        return word_value;
    }
    double ReadDouble(const std::uint32_t source,
                      const std::uint32_t key,
                      const std::uint32_t slot) override {
        calls.push_back("double:" + std::to_string(source) + ":" +
                        std::to_string(key) + ":" +
                        std::to_string(slot));
        return double_value;
    }
    std::uint32_t NameRecord(const std::uint32_t source,
                             const std::uint32_t slot) override {
        calls.push_back("name:" + std::to_string(source) + ":" +
                        std::to_string(slot));
        return name_record;
    }
    std::uint8_t WriteFloat(const std::uint32_t target,
                            const std::uint32_t name,
                            const std::uint32_t bits) override {
        calls.push_back("float_out:" + std::to_string(target) + ":" +
                        std::to_string(name) + ":" +
                        std::to_string(bits));
        return 0xa1u;
    }
    std::uint8_t WriteInteger(const std::uint32_t target,
                              const std::uint32_t name,
                              const std::uint32_t value) override {
        calls.push_back("integer_out:" + std::to_string(target) + ":" +
                        std::to_string(name) + ":" +
                        std::to_string(value));
        return 0xb2u;
    }
    std::uint8_t WriteString(const std::uint32_t target,
                             const std::uint32_t name,
                             const std::uint32_t value) override {
        calls.push_back("string_out:" + std::to_string(target) + ":" +
                        std::to_string(name) + ":" +
                        std::to_string(value));
        return 0xc3u;
    }
    std::uint32_t ConfigString(const std::uint32_t target,
                               const std::uint32_t name) override {
        calls.push_back("config:" + std::to_string(target) + ":" +
                        std::to_string(name));
        return config_string;
    }
    std::uint32_t ParseFloatBits(const std::uint32_t text) override {
        calls.push_back("parse_float:" + std::to_string(text));
        return 0xc0600000u;
    }
    std::uint32_t ParseInteger(const std::uint32_t text) override {
        calls.push_back("parse_integer:" + std::to_string(text));
        return parsed_integer;
    }
    std::uint32_t SetSourceValue(const std::uint32_t source,
                                 const std::uint32_t key,
                                 const std::uint32_t value,
                                 const std::uint32_t slot) override {
        calls.push_back("set_source:" + std::to_string(source) + ":" +
                        std::to_string(key) + ":" +
                        std::to_string(value) + ":" +
                        std::to_string(slot));
        return 0xdefu;
    }
};

Runtime MakeRuntime() {
    Runtime runtime;
    runtime.words[0x5004u] = 0x8000u;
    runtime.words[0x5008u] = 0x44u;
    runtime.words[0x6004u] = 0x7100u;
    runtime.words[0x6204u] = 0x7200u;
    runtime.words[0x6300u] = 0x7300u;
    return runtime;
}

void TestFloatAndIntegers() {
    auto runtime = MakeRuntime();
    Check(pal4::EventPropertyBinding::WriteFloat(
              0x5000u, 0x9000u, runtime) == 0xa1u &&
              runtime.calls == std::vector<std::string>{
                  "read:20488", "double:36864:68:16",
                  "name:36864:4", "read:24580", "read:20484",
                  "float_out:32768:28928:1069547520"},
          "float writer narrows double and routes name/key/target");
    runtime.double_value = 1.0000000001;
    runtime.calls.clear();
    Check(pal4::EventPropertyBinding::WriteFloat(
              0x5000u, 0x9000u, runtime) == 0xa1u &&
              runtime.calls.back() == "float_out:32768:28928:1065353216",
          "float writer stores narrowed IEEE bits");

    runtime.word_value = 0x1234u;
    runtime.calls.clear();
    Check(pal4::EventPropertyBinding::WriteByte(
              0x5000u, 0x9000u, runtime) == 0xb2u &&
              runtime.calls == std::vector<std::string>{
                  "read:20488", "value:36864:68:16",
                  "name:36864:4", "read:24580", "read:20484",
                  "integer_out:32768:28928:52"},
          "byte writer masks source to low eight bits");
    runtime.calls.clear();
    Check(pal4::EventPropertyBinding::WriteInteger(
              0x5000u, 0x9000u, runtime) == 0xb2u &&
              runtime.calls.back() == "integer_out:32768:28928:4660",
          "integer writer preserves full source word");
}

void TestStringsAndFallbacks() {
    auto runtime = MakeRuntime();
    Check(pal4::EventPropertyBinding::WriteString(
              0x5000u, 0x9000u, runtime) == 0xc3u &&
              runtime.calls == std::vector<std::string>{
                  "read:20488", "value:36864:68:16", "read:25092",
                  "name:36864:4", "read:24580", "read:20484",
                  "string_out:32768:28928:29184"},
          "string record reads value data at offset four");
    runtime.word_value = 0x6300u;
    runtime.calls.clear();
    Check(pal4::EventPropertyBinding::WritePointerString(
              0x5000u, 0x9000u, runtime) == 0xc3u &&
              runtime.calls == std::vector<std::string>{
                  "read:20488", "value:36864:68:16", "read:25344",
                  "name:36864:4", "read:24580", "read:20484",
                  "string_out:32768:28928:29440"},
          "pointer string reads source address directly");
    runtime.words[0x6004u] = 0u;
    runtime.words[0x6204u] = 0u;
    runtime.word_value = 0x6200u;
    runtime.calls.clear();
    const auto empty = pal4::StringBuffer::EmptyLogMessageAddress();
    Check(pal4::EventPropertyBinding::WriteString(
              0x5000u, 0x9000u, runtime) == 0xc3u &&
              runtime.calls.back() ==
                  "string_out:32768:" + std::to_string(empty) + ":" +
                      std::to_string(empty),
          "record string and name both use shared empty fallback");
    runtime.words[0x6300u] = 0u;
    runtime.word_value = 0x6300u;
    runtime.calls.clear();
    Check(pal4::EventPropertyBinding::WritePointerString(
              0x5000u, 0x9000u, runtime) == 0xc3u &&
              runtime.calls.back() ==
                  "string_out:32768:" + std::to_string(empty) + ":0",
          "pointer string preserves null value without fallback");

    pal4::EventPropertyBindingRuntimeStub stub;
    Check(stub.ReadWord(0u) == 0u &&
              stub.ReadValue(0u, 0u, 16u) == 0u &&
              stub.ReadDouble(0u, 0u, 16u) == 0.0 &&
              stub.NameRecord(0u, 4u) == 0u,
          "native property binding boundary has a linkable stub");
}

void TestConfigFloat() {
    auto runtime = MakeRuntime();
    Check(pal4::EventPropertyBinding::SetFloatFromConfig(
              0x5000u, 0x9000u, runtime) == 0xdefu &&
              runtime.calls == std::vector<std::string>{
                  "name:36864:4", "read:24580", "read:20484",
                  "config:32768:28928", "parse_float:29696",
                  "read:20488", "set_source:36864:68:3227516928:20"},
          "config float lookup parses and writes source slot twenty");
    runtime.config_string = 0u;
    runtime.calls.clear();
    Check(pal4::EventPropertyBinding::SetFloatFromConfig(
              0x5000u, 0x9000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "name:36864:4", "read:24580", "read:20484",
                  "config:32768:28928"},
          "missing config text skips parse and setter");
    runtime.words[0x6004u] = 0u;
    runtime.config_string = 0x7400u;
    runtime.calls.clear();
    Check(pal4::EventPropertyBinding::SetFloatFromConfig(
              0x5000u, 0x9000u, runtime) == 0xdefu &&
              runtime.calls[3] == "config:32768:" + std::to_string(
                  pal4::StringBuffer::EmptyLogMessageAddress()),
          "config lookup uses shared empty name when source name is null");
}

void TestConfigIntegerAndBool() {
    auto runtime = MakeRuntime();
    Check(pal4::EventPropertyBinding::SetIntegerFromConfig(
              0x5000u, 0x9000u, runtime) == 0xdefu &&
              runtime.calls == std::vector<std::string>{
                  "name:36864:4", "read:24580", "read:20484",
                  "config:32768:28928", "parse_integer:29696",
                  "read:20488", "set_source:36864:68:305419896:20"},
          "config integer forwards the complete parsed word");
    runtime.calls.clear();
    Check(pal4::EventPropertyBinding::SetBoolFromConfig(
              0x5000u, 0x9000u, runtime) == 0xdefu &&
              runtime.calls.back() == "set_source:36864:68:305419777:20",
          "config bool changes only the low byte of a positive value");
    runtime.parsed_integer = 0x80001234u;
    runtime.calls.clear();
    Check(pal4::EventPropertyBinding::SetBoolFromConfig(
              0x5000u, 0x9000u, runtime) == 0xdefu &&
              runtime.calls.back() == "set_source:36864:68:2147488256:20",
          "config bool retains negative value high bytes and clears low byte");
    runtime.parsed_integer = 0u;
    runtime.calls.clear();
    Check(pal4::EventPropertyBinding::SetBoolFromConfig(
              0x5000u, 0x9000u, runtime) == 0xdefu &&
              runtime.calls.back() == "set_source:36864:68:0:20",
          "config bool zero is false");
    runtime.config_string = 0u;
    runtime.calls.clear();
    Check(pal4::EventPropertyBinding::SetBoolFromConfig(
              0x5000u, 0x9000u, runtime) == 0u &&
              runtime.calls.size() == 4u &&
              runtime.calls.back() == "config:32768:28928",
          "missing config text skips integer parser and setter");
    runtime.words[0x6004u] = 0u;
    runtime.config_string = 0x7400u;
    runtime.calls.clear();
    Check(pal4::EventPropertyBinding::SetIntegerFromConfig(
              0x5000u, 0x9000u, runtime) == 0xdefu &&
              runtime.calls[3] == "config:32768:" + std::to_string(
                  pal4::StringBuffer::EmptyLogMessageAddress()),
          "config integer uses shared empty source name");
    pal4::EventPropertyBindingRuntimeStub stub;
    Check(stub.ParseInteger(0u) == 0u,
          "integer parser boundary has a linkable stub");
}

}  // namespace

int main() {
    TestFloatAndIntegers();
    TestStringsAndFallbacks();
    TestConfigFloat();
    TestConfigIntegerAndBool();
}
