#include "pal4/attribute_value_conversion.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <limits>
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

struct Runtime final : pal4::IAttributeValueRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint32_t getter_result{0x3000u};
    double double_result{-1.25};
    float parsed_float{2.5f};
    std::int32_t parsed_int{-7};
    std::int32_t format_result{12};
    std::int32_t setter_result{-19};
    std::uint32_t formatted_buffer{};
    std::uint32_t formatted_count{};
    std::string formatted_pattern;
    double formatted_double{};
    std::int32_t formatted_int{};
    std::uint32_t set_value{};
    std::uint32_t call_value{0x4567u};
    std::int64_t converted_long{-9};
    std::string resolved_text{"mode"};

    void CallMethod(std::uint32_t target,
                    pal4::AttributeValueState& attribute,
                    std::uint32_t slot) override {
        calls.push_back("call:" + std::to_string(target) + ":" +
                        std::to_string(slot));
        attribute.value = call_value;
    }
    std::int32_t CallSetter(std::uint32_t target,
                            pal4::AttributeValueState& attribute,
                            std::uint32_t slot) override {
        calls.push_back("setter:" + std::to_string(target) + ":" +
                        std::to_string(attribute.value) + ":" +
                        std::to_string(slot));
        return setter_result;
    }
    const char* ResolveCString(std::uint32_t address) override {
        calls.push_back("resolve:" + std::to_string(address));
        return resolved_text.c_str();
    }
    std::int64_t FloatToLong(float value) override {
        calls.push_back("ftol:" + std::to_string(value));
        return converted_long;
    }
    double GetDouble(std::uint32_t target, std::uint32_t key,
                     std::uint32_t slot) override {
        calls.push_back("double:" + std::to_string(target) + ":" +
                        std::to_string(key) + ":" +
                        std::to_string(slot));
        return double_result;
    }
    std::uint32_t GetWord(std::uint32_t target, std::uint32_t key,
                          std::uint32_t slot) override {
        calls.push_back("get:" + std::to_string(target) + ":" +
                        std::to_string(key) + ":" +
                        std::to_string(slot));
        return getter_result;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::int32_t FormatDouble(std::uint32_t buffer,
                              std::uint32_t count, const char* pattern,
                              double value) override {
        calls.push_back("format-double");
        formatted_buffer = buffer;
        formatted_count = count;
        formatted_pattern = pattern;
        formatted_double = value;
        return format_result;
    }
    std::int32_t FormatInt(std::uint32_t buffer,
                           std::uint32_t count, const char* pattern,
                           std::int32_t value) override {
        calls.push_back("format-int");
        formatted_buffer = buffer;
        formatted_count = count;
        formatted_pattern = pattern;
        formatted_int = value;
        return format_result;
    }
    float ParseFloat(std::uint32_t text) override {
        calls.push_back("parse-float:" + std::to_string(text));
        return parsed_float;
    }
    std::int32_t ParseInt(std::uint32_t text) override {
        calls.push_back("parse-int:" + std::to_string(text));
        return parsed_int;
    }
    std::int32_t SetWord(std::uint32_t target, std::uint32_t key,
                         std::uint32_t value,
                         std::uint32_t slot) override {
        calls.push_back("set:" + std::to_string(target) + ":" +
                        std::to_string(key) + ":" +
                        std::to_string(slot));
        set_value = value;
        return setter_result;
    }
};

struct PropertyRuntime final : pal4::IPropertyAccessRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint32_t dispatch_slot{};
    std::int32_t dispatch_index{-1};
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteDword(std::uint32_t, std::uint32_t) override {}
    float ReadFloat(std::uint32_t) override { return 0.0f; }
    void WriteFloat(std::uint32_t, float) override {}
    std::int32_t DispatchByIndex(const pal4::PropertyCollectionState&,
                                 std::uint32_t, std::int32_t index,
                                 std::uint32_t slot) override {
        dispatch_index = index;
        dispatch_slot = slot;
        return -23;
    }
};

void TestGetAndFormat() {
    Runtime runtime;
    pal4::AttributeValueState attribute{0x1234u, 0x2000u, 0x55u};
    Check(pal4::AttributeValueConversion::CallMethodAndReturnProperty(
              attribute, 0x8000u, runtime) == 0x4567u &&
              runtime.calls == std::vector<std::string>{"call:32768:8"},
          "slot eight runs before returning the updated property field");

    runtime.calls.clear();
    Check(pal4::AttributeValueConversion::DoubleToString(
              attribute, 0x8000u, runtime) == 12 &&
              runtime.calls == std::vector<std::string>{
                  "double:32768:85:16", "format-double"} &&
              runtime.formatted_buffer ==
                  pal4::AttributeValueConversion::kFormatBuffer &&
              runtime.formatted_count == 0x1ffu &&
              runtime.formatted_pattern == "%f" &&
              runtime.formatted_double == -1.25 &&
              attribute.value ==
                  pal4::AttributeValueConversion::kFormatBuffer,
          "double conversion uses virtual getter and shared format buffer");

    runtime.getter_result = 0xfffffffeu;
    runtime.calls.clear();
    Check(pal4::AttributeValueConversion::IntegerToString(
              attribute, 0x8000u, runtime) == 12 &&
              runtime.formatted_pattern == "%d" &&
              runtime.formatted_int == -2 &&
              runtime.calls == std::vector<std::string>{
                  "get:32768:85:16", "format-int"},
          "integer conversion preserves signed 32-bit interpretation");
    Check(pal4::AttributeValueConversion::ByteToString(
              attribute, 0x8000u, runtime) == 12 &&
              runtime.formatted_int == 254,
          "byte conversion masks the virtual getter to eight bits");

    runtime.getter_result = 0x3000u;
    runtime.words[0x3004u] = 0u;
    runtime.words[0x3000u] = 0xfedcba98u;
    Check(pal4::AttributeValueConversion::GetString(
              attribute, 0x8000u, runtime) ==
              pal4::AttributeValueConversion::kEmptyLogMessage &&
              attribute.value ==
                  pal4::AttributeValueConversion::kEmptyLogMessage,
          "null string data falls back to shared empty message");
    runtime.words[0x3004u] = 0x4000u;
    Check(pal4::AttributeValueConversion::GetString(
              attribute, 0x8000u, runtime) == 0x4000u &&
              pal4::AttributeValueConversion::GetInteger(
                  attribute, 0x8000u, runtime) == 0xfedcba98u &&
              attribute.value == 0xfedcba98u,
          "record string and raw integer reads use distinct offsets");
}

void TestSetFromString() {
    Runtime runtime;
    const pal4::AttributeValueState attribute{0u, 0x7000u, 0x55u};
    Check(pal4::AttributeValueConversion::SetFloatFromString(
              attribute, 0x8000u, runtime) == -19 &&
              runtime.set_value == std::bit_cast<std::uint32_t>(2.5f) &&
              runtime.calls == std::vector<std::string>{
                  "parse-float:28672", "set:32768:85:20"},
          "float setter forwards the converted float's raw bits");
    runtime.calls.clear();
    Check(pal4::AttributeValueConversion::SetIntFromString(
              attribute, 0x8000u, runtime) == -19 &&
              runtime.set_value == static_cast<std::uint32_t>(-7) &&
              runtime.calls == std::vector<std::string>{
                  "parse-int:28672", "set:32768:85:20"},
          "integer setter forwards signed parse bits and virtual return");
    runtime.calls.clear();
    Check(pal4::AttributeValueConversion::SetBoolFromString(
              attribute, 0x8000u, runtime) == -19 &&
              runtime.set_value == 0u,
          "negative integer parses as false");
    runtime.parsed_int = 1;
    Check(pal4::AttributeValueConversion::SetBoolFromString(
              attribute, 0x8000u, runtime) == -19 &&
              runtime.set_value == 1u,
          "strictly positive integer parses as true");
}

void TestStub() {
    pal4::AttributeValueRuntimeStub stub;
    pal4::AttributeValueState attribute{0u, 4u, 8u};
    Check(pal4::AttributeValueConversion::IntegerToString(
              attribute, 0u, stub) == 0 &&
              attribute.value ==
                  pal4::AttributeValueConversion::kFormatBuffer,
          "native boundary has a linkable no-op stub");
}

void TestPropertyBridge() {
    Runtime runtime;
    PropertyRuntime property_runtime;
    pal4::AttributeValueState attribute{0u, 0x7000u, 0x55u};
    const pal4::PropertyCollectionState collection{0u, 0x5000u, 1};
    property_runtime.words[0x5004u] = 0x6000u;
    property_runtime.words[0x5008u] = 4u;
    property_runtime.bytes[0x6000u] = 'm';
    property_runtime.bytes[0x6001u] = 'o';
    property_runtime.bytes[0x6002u] = 'd';
    property_runtime.bytes[0x6003u] = 'e';
    Check(pal4::AttributeValueConversion::SetEnumFromString(
              attribute, collection, 0x8000u, runtime,
              property_runtime) == -23 &&
              property_runtime.dispatch_index == 0 &&
              property_runtime.dispatch_slot == 20u &&
              runtime.calls == std::vector<std::string>{"resolve:28672"},
          "enum setter composes named collection lookup");

    runtime.calls.clear();
    Check(pal4::AttributeValueConversion::SetFromAttribute(
              attribute, 0x8000u, 0x1234u, runtime) == -19 &&
              attribute.value == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "setter:32768:4660:12"},
          "property setter records input then invokes slot twelve");
    runtime.call_value = std::bit_cast<std::uint32_t>(-2.5f);
    Check(pal4::AttributeValueConversion::GetAsAttribute(
              attribute, 0x8000u, runtime) == -2.5 &&
              attribute.value == runtime.call_value,
          "attribute getter widens the float written by slot eight");
}

void TestNumericPropertyBridge() {
    Runtime runtime;
    pal4::AttributeValueState attribute{0u, 0u, 0x55u};
    runtime.double_result = 3.75;
    pal4::AttributeValueConversion::GetFloatFromProperty(
        attribute, 0x8000u, runtime);
    Check(std::bit_cast<float>(attribute.value) == 3.75f,
          "double property narrows to stored float");
    runtime.getter_result = 0x1234fffeu;
    Check(pal4::AttributeValueConversion::GetBoolFromProperty(
              attribute, 0x8000u, runtime) == 0xfeu &&
              std::bit_cast<float>(attribute.value) == 254.0f,
          "bool property uses low byte then converts it to float");
    runtime.getter_result = 0xfffffffeu;
    Check(pal4::AttributeValueConversion::GetIntFromProperty(
              attribute, 0x8000u, runtime) == -2 &&
              std::bit_cast<float>(attribute.value) == -2.0f,
          "signed integer property converts to float and returns original");

    runtime.getter_result = 0x3000u;
    runtime.words[0x3004u] = 0u;
    runtime.words[0x3000u] = 0x4000u;
    runtime.parsed_float = -1.5f;
    pal4::AttributeValueConversion::GetFloatFromStringProperty(
        attribute, 0x8000u, runtime);
    Check(std::bit_cast<float>(attribute.value) == -1.5f &&
              runtime.calls.back() == "parse-float:8657252",
          "string record null data uses the shared empty message");
    runtime.calls.clear();
    pal4::AttributeValueConversion::GetFloatFromDirectStringProperty(
        attribute, 0x8000u, runtime);
    Check(std::bit_cast<float>(attribute.value) == -1.5f &&
              runtime.calls == std::vector<std::string>{
                  "get:32768:85:16", "read:12288",
                  "parse-float:16384"},
          "direct string record dereferences pointer at offset zero");
}

void TestNumericSetters() {
    Runtime runtime;
    pal4::AttributeValueState attribute{
        0u, std::bit_cast<std::uint32_t>(-1.5f), 0x55u};
    Check(pal4::AttributeValueConversion::SetProperty(
              attribute, 0x8000u, runtime) == -19 &&
              runtime.set_value == attribute.value,
          "raw property setter forwards stored word");
    runtime.calls.clear();
    Check(pal4::AttributeValueConversion::SetFloat(
              attribute, 0x8000u, runtime) == -19 &&
              runtime.set_value == static_cast<std::uint32_t>(-9) &&
              runtime.calls == std::vector<std::string>{
                  "ftol:-1.500000", "set:32768:85:20"},
          "float setter forwards low word of CRT conversion");
    Check(pal4::AttributeValueConversion::SetBool(
              attribute, 0x8000u, runtime) == -19 &&
              runtime.set_value == 0u,
          "negative float is false");
    attribute.value = std::bit_cast<std::uint32_t>(0.0f);
    (void)pal4::AttributeValueConversion::SetBool(
        attribute, 0x8000u, runtime);
    Check(runtime.set_value == 0u, "zero float is false");
    attribute.value = std::bit_cast<std::uint32_t>(
        std::numeric_limits<float>::quiet_NaN());
    (void)pal4::AttributeValueConversion::SetBool(
        attribute, 0x8000u, runtime);
    Check(runtime.set_value == 0u, "unordered float is false");
    attribute.value = std::bit_cast<std::uint32_t>(0.5f);
    (void)pal4::AttributeValueConversion::SetBool(
        attribute, 0x8000u, runtime);
    Check(runtime.set_value == 1u, "positive float is true");
}

}  // namespace

int main() {
    TestGetAndFormat();
    TestSetFromString();
    TestPropertyBridge();
    TestNumericPropertyBridge();
    TestNumericSetters();
    TestStub();
}
