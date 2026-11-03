#include "pal4/combat_event_property.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatEventPropertyRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t attribute_value = 0u;
    std::uint32_t attribute_property = 0u;
    std::uint32_t attribute_bits = 0u;
    float getter_result = 1.5f;
    int setter_result = -17;
    bool wait_result = true;
    bool change_property_on_get = false;
    int getter_count = 0;
    int setter_count = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read-byte:" + std::to_string(address));
        return bytes[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    float GetAsAttribute(pal4::EventAttributeValue& value,
                         std::uint32_t property) override {
        Check(value.vtable == 0x848108u, "attribute value vtable");
        calls.emplace_back("get");
        ++getter_count;
        attribute_value = value.value_bits;
        attribute_property = property;
        if (change_property_on_get)
            words[0x1000u + 8u] = 0x3300u;
        return getter_result;
    }
    int SetFromAttribute(pal4::EventAttributeValue& value,
                         std::uint32_t property,
                         std::uint32_t bits) override {
        Check(value.vtable == 0x848108u, "setter value vtable");
        calls.emplace_back("set");
        ++setter_count;
        attribute_value = value.value_bits;
        attribute_property = property;
        attribute_bits = bits;
        return setter_result;
    }
    bool ResetWaitable(std::uint32_t waitable) override {
        Check(waitable == 0x4400u, "waitable pointer");
        calls.emplace_back("reset");
        return wait_result;
    }
};

void Construction() {
    Runtime runtime;
    Check(pal4::CombatEventProperty::Construct(
              0x1000u, 0x2200u, 0x3300u, 0x7fu, runtime) == 0x1000u,
          "event property constructor returns self");
    Check(runtime.calls == std::vector<std::string>{
              "write:4100", "write:4104", "byte:4108", "write:4096"} &&
              runtime.words[0x1000u] == 0x8480f4u &&
              runtime.words[0x1000u + 4u] == 0x2200u &&
              runtime.words[0x1000u + 8u] == 0x3300u &&
              runtime.bytes[0x1000u + 12u] == 0x7fu,
          "constructor field values and native write order");
}

void AttributeCalls() {
    Runtime runtime;
    runtime.words[0x1000u + 4u] = 0x2200u;
    runtime.words[0x1000u + 8u] = 0x3300u;
    Check(pal4::CombatEventProperty::SetFloatProperty(0x1000u, -2.5f, runtime) == runtime.getter_result,
          "attribute getter preserves the native floating return value");
    Check(runtime.calls == std::vector<std::string>{"read:4100", "get"} &&
              runtime.attribute_property == 0x2200u &&
              runtime.attribute_value == std::bit_cast<std::uint32_t>(-2.5f) &&
              runtime.setter_count == 0,
          "set-float entry invokes getter with raw float bits");

    runtime.calls.clear();
    Check(pal4::CombatEventProperty::SetIntProperty(
              0x1000u, 0xdeadbeefu, 0x91u, runtime) == -17 &&
              runtime.calls == std::vector<std::string>{"read:4104", "set"} &&
              runtime.attribute_value == 0xdeadbeefu &&
              runtime.attribute_property == 0x3300u &&
              runtime.attribute_bits == 0x91u,
          "integer entry forwards raw value and setter result");

    runtime.calls.clear();
    runtime.words[0x1000u + 8u] = 0x2200u;
    runtime.change_property_on_get = true;
    runtime.getter_result = 1.5f;
    Check(pal4::CombatEventProperty::IncrementFloatProperty(
              0x1000u, 0x12345678u, 0.25f, runtime) == -17 &&
              runtime.calls == std::vector<std::string>{
                  "read:4104", "get", "read:4104", "set"} &&
              runtime.getter_count == 2 && runtime.setter_count == 2 &&
              runtime.attribute_value == 0x12345678u &&
              runtime.attribute_property == 0x3300u &&
              runtime.attribute_bits == std::bit_cast<std::uint32_t>(1.75f),
          "float increment rereads property and forwards sum bits");
}

void MultiplyAndWait() {
    Runtime runtime;
    runtime.words[0x5000u] = std::bit_cast<std::uint32_t>(3.0f);
    Check(!pal4::CombatEventProperty::MultiplyFloatProperty(
              0x1000u, 0x5000u, 2.0f, runtime) &&
              runtime.calls == std::vector<std::string>{"read-byte:4108"} &&
              runtime.words[0x5000u] == std::bit_cast<std::uint32_t>(3.0f),
          "disabled multiply does not touch value address");

    runtime.calls.clear();
    runtime.bytes[0x1000u + 12u] = 1u;
    Check(pal4::CombatEventProperty::MultiplyFloatProperty(
              0x1000u, 0x5000u, -2.0f, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "read-byte:4108", "read:20480", "write:20480"} &&
              runtime.words[0x5000u] == std::bit_cast<std::uint32_t>(-6.0f),
          "enabled multiply stores float product");

    runtime.calls.clear();
    runtime.words[0x6000u + 4u] = 0x4400u;
    Check(pal4::ConcurrencyEventWaitNode::Reset(0x6000u, runtime) &&
              runtime.calls == std::vector<std::string>{"read:24580", "reset"},
          "wait node forwards virtual reset result");
    runtime.calls.clear();
    runtime.wait_result = false;
    Check(!pal4::ConcurrencyEventWaitNode::Reset(0x6000u, runtime),
          "wait node preserves false result");
}

void NextState() {
    Check(pal4::CombatDamageState::Next(0u) == 1u &&
              pal4::CombatDamageState::Next(3u) == 4u &&
              pal4::CombatDamageState::Next(4u) == 0u &&
              pal4::CombatDamageState::Next(5u) == 6u &&
              pal4::CombatDamageState::Next(
                  std::numeric_limits<std::uint32_t>::max()) == 0u,
          "next-state helper wraps only four; other values increment as u32");
}

}  // namespace

int main() {
    try {
        Construction();
        AttributeCalls();
        MultiplyAndWait();
        NextState();
        std::cout << "combat event property tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
