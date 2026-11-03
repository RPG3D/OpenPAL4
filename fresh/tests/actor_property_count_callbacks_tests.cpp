#include "pal4/actor_property_count_callbacks.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kObject = 0x1000u;
constexpr std::uint32_t kOwner = 0x2000u;

struct Runtime final : pal4::IActorPropertyCountCallbackRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t allows_result = 0u;
    std::int32_t value_result = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    std::uint32_t OwnerAllows(std::uint32_t owner,
                              std::uint32_t offset,
                              std::int32_t first,
                              std::int32_t mode) override {
        Check(owner == kOwner && offset == 8u && first == 41,
              "owner allows virtual arguments");
        calls.push_back("allows:" + std::to_string(mode));
        return allows_result;
    }
    std::int32_t OwnerValue(std::uint32_t owner,
                            std::uint32_t offset,
                            std::int32_t first,
                            std::int32_t mode) override {
        Check(owner == kOwner && offset == 148u && first == 41,
              "owner value virtual arguments");
        calls.push_back("value:" + std::to_string(mode));
        return value_result;
    }
};

void Filters() {
    using Filter = std::uint8_t (*)(
        std::uint32_t, std::int32_t, std::int32_t,
        pal4::IActorPropertyCountCallbackRuntime&);
    struct Case { Filter filter; std::int32_t denied_a, denied_b, allowed; };
    const Case cases[] = {
        {pal4::ActorPropertyCountCallbacks::AllowsExceptThreeFour,
         3, 4, 2},
        {pal4::ActorPropertyCountCallbacks::AllowsExceptTwoFour,
         2, 4, 3}};
    for (const auto& item : cases) {
        Runtime r;
        Check(item.filter(kObject, 41, item.denied_a, r) == 0u &&
                  item.filter(kObject, 41, item.denied_b, r) == 0u &&
                  r.calls.empty(),
              "denied modes short circuit before owner read");
        r.words[kObject + 8u] = 0u;
        Check(item.filter(kObject, 41, item.allowed, r) == 1u &&
                  r.calls == std::vector<std::string>{"read:4104"},
              "missing owner allows callback");
        r = Runtime{};
        r.words[kObject + 8u] = kOwner;
        r.allows_result = 0x1234u;
        Check(item.filter(kObject, 41, item.allowed, r) == 0x34u &&
                  r.calls == std::vector<std::string>{
                      "read:4104", "allows:" +
                          std::to_string(item.allowed)},
              "owner virtual result is truncated to low byte");
        r.calls.clear();
        r.allows_result = 0x100u;
        Check(item.filter(kObject, 41, item.allowed, r) == 0u,
              "zero low byte is returned unchanged");
    }
}

void Values() {
    Check(pal4::ActorPropertyCountCallbacks::Constant98() == 98 &&
              pal4::ActorPropertyCountCallbacks::Constant99() == 99,
          "fixed callback values");
    Runtime r;
    r.words[kObject + 8u] = kOwner;
    r.value_result = 7;
    Check(pal4::ActorPropertyCountCallbacks::ReadOwnerValue(
              kObject, 41, 2, r) == 7 &&
              r.calls == std::vector<std::string>{
                  "read:4104", "value:2"},
          "ordinary owner value forwards virtual result");
    r.calls.clear();
    r.value_result = 0x40000000;
    Check(pal4::ActorPropertyCountCallbacks::ReadOwnerValue(
              kObject, 41, 3, r) ==
                  static_cast<std::int32_t>(0x80000000u) &&
              r.calls == std::vector<std::string>{
                  "read:4104", "value:3"},
          "mode three doubles with 32-bit wrap");
}

void Constructor() {
    Runtime r;
    Check(pal4::ActorPropertyCountCallbacks::Construct(
              kObject, r) == kObject &&
              r.bytes.at(kObject + 4u) == 0u &&
              r.words.at(kObject + 8u) == 0u &&
              r.words.at(kObject + 12u) == 0u &&
              r.words.at(kObject) == 0x846290u &&
              r.calls == std::vector<std::string>{
                  "byte:4100", "write:4104", "write:4108",
                  "write:4096"},
          "callback constructor field order and vtable");
}

}  // namespace

int main() {
    try {
        Filters();
        Values();
        Constructor();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
