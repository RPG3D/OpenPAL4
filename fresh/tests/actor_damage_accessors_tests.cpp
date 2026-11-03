#include "pal4/actor_damage_accessors.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kActor = 0x1000u;
constexpr std::uint32_t kVtable = 0x8000u;

struct Runtime final : pal4::IActorDamageAccessorRuntime {
    std::map<std::uint32_t, std::uint32_t> words{{kActor, kVtable}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::int32_t int_current = 0;
    double float_current = 0.0;
    std::uint32_t get_offset = 0u;
    std::uint32_t set_offset = 0u;
    std::uint32_t last_index = 0u;
    std::int32_t int_set_value = 0;
    std::uint32_t float_set_bits = 0u;

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
    std::int32_t CallIntGet(std::uint32_t actor,
                            std::uint32_t vtable,
                            std::uint32_t offset,
                            std::uint32_t index) override {
        Check(actor == kActor && vtable == kVtable,
              "integer getter captured vtable");
        calls.emplace_back("get_int");
        get_offset = offset;
        last_index = index;
        words[kActor] = 0x9000u;
        return int_current;
    }
    std::int32_t CallIntSet(std::uint32_t actor,
                            std::uint32_t vtable,
                            std::uint32_t offset,
                            std::uint32_t index,
                            std::int32_t value) override {
        Check(actor == kActor && vtable == kVtable &&
                  words.at(kActor) == 0x9000u,
              "integer setter uses original vtable");
        calls.emplace_back("set_int");
        set_offset = offset;
        last_index = index;
        int_set_value = value;
        return 0x1234;
    }
    double CallFloatGet(std::uint32_t actor,
                        std::uint32_t vtable,
                        std::uint32_t offset,
                        std::uint32_t index) override {
        Check(actor == kActor && vtable == kVtable,
              "float getter captured vtable");
        calls.emplace_back("get_float");
        get_offset = offset;
        last_index = index;
        words[kActor] = 0x9000u;
        return float_current;
    }
    std::int32_t CallFloatSet(std::uint32_t actor,
                              std::uint32_t vtable,
                              std::uint32_t offset,
                              std::uint32_t index,
                              std::uint32_t bits) override {
        Check(actor == kActor && vtable == kVtable &&
                  words.at(kActor) == 0x9000u,
              "float setter uses original vtable");
        calls.emplace_back("set_float");
        set_offset = offset;
        last_index = index;
        float_set_bits = bits;
        return 0x2345;
    }
};

using WordGetter = std::uint32_t (*)(
    std::uint32_t, std::uint32_t,
    pal4::IActorDamageAccessorRuntime&);
using FloatGetter = double (*)(
    std::uint32_t, std::int32_t,
    pal4::IActorDamageAccessorRuntime&);
using WordAdder = std::int32_t (*)(
    std::uint32_t, std::uint32_t, std::int32_t,
    pal4::IActorDamageAccessorRuntime&);
using FloatAdder = std::int32_t (*)(
    std::uint32_t, std::uint32_t, float,
    pal4::IActorDamageAccessorRuntime&);

void Constructor() {
    Runtime r;
    Check(pal4::ActorDamageAccessors::Construct(kActor, r) == kActor &&
              r.bytes.at(kActor + 4u) == 0u &&
              r.words.at(kActor + 8u) == 0u &&
              r.words.at(kActor) == 0x845e70u &&
              r.calls == std::vector<std::string>{
                  "byte:4100", "write:4104", "write:4096"},
          "callback object constructor order");
}

void CountAccessors() {
    Runtime r;
    Check(pal4::ActorDamageAccessors::ConstructCountCallback(
              kActor, r) == kActor &&
              r.bytes.at(kActor + 4u) == 0u &&
              r.words.at(kActor + 8u) == 0u &&
              r.words.at(kActor) == 0x846130u &&
              r.calls == std::vector<std::string>{
                  "byte:4100", "write:4104", "write:4096"},
          "count callback constructor layout and order");
    Check(pal4::ActorDamageAccessors::GetPropertyCount() == 9u,
          "property count constant");
    r = Runtime{};
    r.words[kActor + 672u] = 17u;
    r.words[kActor + 668u] = 23u;
    Check(pal4::ActorDamageAccessors::GetActorCount672(kActor, r) == 17u &&
              pal4::ActorDamageAccessors::GetActorCount668(kActor, r) ==
                  23u &&
              r.calls == std::vector<std::string>{
                  "read:4768", "read:4764"},
          "count getter addresses and values");
    Check(pal4::ActorDamageAccessors::SetActorCount672(
              kActor, 0xffffffffu, r) == 0xffffffffu &&
              r.words.at(kActor + 672u) == 0xffffffffu &&
              r.words.at(kActor + 668u) == 23u &&
              r.calls.back() == "write:4768",
          "count setter writes only target field and forwards value");

    using StateGetter = std::uint32_t (*)(
        std::uint32_t, pal4::IActorDamageAccessorRuntime&);
    const StateGetter getters[] = {
        pal4::ActorDamageAccessors::GetCountState632,
        pal4::ActorDamageAccessors::GetCountState636,
        pal4::ActorDamageAccessors::GetCountState640,
        pal4::ActorDamageAccessors::GetCountState644,
        pal4::ActorDamageAccessors::GetCountState648,
        pal4::ActorDamageAccessors::GetCountState652,
        pal4::ActorDamageAccessors::GetCountState656,
        pal4::ActorDamageAccessors::GetCountState660,
        pal4::ActorDamageAccessors::GetCountState664};
    for (std::uint32_t i = 0u; i < 9u; ++i) {
        r = Runtime{};
        const auto address = kActor + 632u + 4u * i;
        r.words[address] = 0x90000000u + i;
        Check(getters[i](kActor, r) == 0x90000000u + i &&
                  r.calls == std::vector<std::string>{
                      "read:" + std::to_string(address)},
              "count state getter offset and value");
    }
}

void DirectFloatAdders() {
    using Adder = double (*)(std::uint32_t, std::int32_t, float,
                             pal4::IActorDamageAccessorRuntime&);
    struct Case { Adder add; std::uint32_t base, maximum; };
    const Case cases[] = {
        {pal4::ActorDamageAccessors::AddDirectFloat484, 484u, 5u},
        {pal4::ActorDamageAccessors::AddDirectFloat508, 508u, 6u},
        {pal4::ActorDamageAccessors::AddDirectFloat536, 536u, 1u}};
    for (const auto& item : cases) {
        Runtime r;
        Check(item.add(kActor, -1, 0.2f, r) == -1.0 &&
                  item.add(kActor,
                           static_cast<std::int32_t>(item.maximum + 1u),
                           0.2f, r) ==
                      static_cast<double>(item.maximum + 1u) &&
                  r.calls.empty(),
              "direct float adder invalid index has no memory access");
        r = Runtime{};
        const auto address = kActor + item.base + 4u * item.maximum;
        r.words[address] = std::bit_cast<std::uint32_t>(0.1f);
        const auto sum = static_cast<double>(0.1f) +
                         static_cast<double>(0.2f);
        Check(item.add(kActor,
                       static_cast<std::int32_t>(item.maximum),
                       0.2f, r) == sum &&
                  r.words.at(address) == std::bit_cast<std::uint32_t>(
                      static_cast<float>(sum)) &&
                  sum != static_cast<double>(static_cast<float>(sum)) &&
                  r.calls == std::vector<std::string>{
                      "read:" + std::to_string(address),
                      "write:" + std::to_string(address)},
              "direct float adder returns wide sum and stores narrow sum");
    }
}

void WordGetters() {
    struct Case { WordGetter get; std::uint32_t base, max; };
    const Case cases[] = {
        {pal4::ActorDamageAccessors::GetWord344, 344u, 7u},
        {pal4::ActorDamageAccessors::GetWord464, 464u, 4u},
        {pal4::ActorDamageAccessors::GetWord392, 392u, 5u},
        {pal4::ActorDamageAccessors::GetWord544, 544u, 9u}};
    for (const auto& item : cases) {
        Runtime r;
        r.words[kActor + item.base] = 0x1111u;
        r.words[kActor + item.base + 4u * item.max] = 0x2222u;
        Check(item.get(kActor, 0u, r) == 0x1111u &&
                  item.get(kActor, item.max, r) == 0x2222u &&
                  item.get(kActor, item.max + 1u, r) ==
                      item.max + 1u &&
                  item.get(kActor, 0xffffffffu, r) == 0xffffffffu &&
                  r.calls.size() == 2u,
              "word getter base, last index and fallback");
    }
}

void FloatGetters() {
    struct Case { FloatGetter get; std::uint32_t base, max; };
    const Case cases[] = {
        {pal4::ActorDamageAccessors::GetFloat376, 376u, 3u},
        {pal4::ActorDamageAccessors::GetFloat416, 416u, 5u},
        {pal4::ActorDamageAccessors::GetFloat440, 440u, 5u},
        {pal4::ActorDamageAccessors::GetFloat584, 584u, 4u},
        {pal4::ActorDamageAccessors::GetFloat604, 604u, 6u}};
    for (const auto& item : cases) {
        Runtime r;
        r.words[kActor + item.base] =
            std::bit_cast<std::uint32_t>(1.25f);
        r.words[kActor + item.base + 4u * item.max] =
            std::bit_cast<std::uint32_t>(-2.5f);
        Check(item.get(kActor, 0, r) == 1.25 &&
                  item.get(kActor, static_cast<std::int32_t>(item.max), r) ==
                      -2.5 &&
                  item.get(kActor,
                           static_cast<std::int32_t>(item.max + 1u), r) ==
                      static_cast<double>(item.max + 1u) &&
                  item.get(kActor, -1, r) == -1.0 &&
                  r.calls.size() == 2u,
              "float getter base, last index and signed fallback");
    }
}

void WordAdders() {
    struct Case { WordAdder add; std::uint32_t get, set; };
    const Case cases[] = {
        {pal4::ActorDamageAccessors::AddWord48_64, 48u, 64u},
        {pal4::ActorDamageAccessors::AddWord28_32, 28u, 32u},
        {pal4::ActorDamageAccessors::AddWord88_92, 88u, 92u},
        {pal4::ActorDamageAccessors::AddWord52_68, 52u, 68u}};
    for (const auto& item : cases) {
        Runtime r;
        r.int_current = std::numeric_limits<std::int32_t>::max();
        Check(item.add(kActor, 3u, 1, r) == 0x1234 &&
                  r.get_offset == item.get &&
                  r.set_offset == item.set &&
                  r.last_index == 3u &&
                  r.int_set_value ==
                      std::numeric_limits<std::int32_t>::min() &&
                  r.calls == std::vector<std::string>{
                      "read:4096", "get_int", "set_int"},
              "integer adder offsets, wrap and captured vtable");
    }
}

void FloatAdders() {
    struct Case { FloatAdder add; std::uint32_t get, set; };
    const Case cases[] = {
        {pal4::ActorDamageAccessors::AddFloat44_60, 44u, 60u},
        {pal4::ActorDamageAccessors::AddFloat100_104, 100u, 104u},
        {pal4::ActorDamageAccessors::AddFloat112_116, 112u, 116u},
        {pal4::ActorDamageAccessors::AddFloat40_56, 40u, 56u},
        {pal4::ActorDamageAccessors::AddFloat124_128, 124u, 128u}};
    for (const auto& item : cases) {
        Runtime r;
        r.float_current = 1.125;
        Check(item.add(kActor, 2u, 0.25f, r) == 0x2345 &&
                  r.get_offset == item.get &&
                  r.set_offset == item.set &&
                  r.last_index == 2u &&
                  r.float_set_bits ==
                      std::bit_cast<std::uint32_t>(1.375f) &&
                  r.calls == std::vector<std::string>{
                      "read:4096", "get_float", "set_float"},
              "float adder offsets, bits and captured vtable");
    }
}

void DirectSetters() {
    Runtime r;
    r.words[kActor + 1144u] = 1u;
    Check(pal4::ActorDamageAccessors::SetConditional672(
              kActor, 0x1234u, r) == kActor &&
              r.calls.size() == 1u,
          "positive gate prevents write");
    r = Runtime{};
    r.words[kActor + 1144u] = 0u;
    Check(pal4::ActorDamageAccessors::SetConditional672(
              kActor, 0x1234u, r) == kActor &&
              r.words.at(kActor + 672u) == 0x1234u,
          "zero gate writes value");
    r = Runtime{};
    r.words[kActor + 1144u] = 0xffffffffu;
    Check(pal4::ActorDamageAccessors::SetConditional672(
              kActor, 0x5678u, r) == kActor &&
              r.words.at(kActor + 672u) == 0x5678u,
          "negative signed gate writes value");
    r = Runtime{};
    Check(pal4::ActorDamageAccessors::SetMirrored332_668(
              kActor, 0xbeefu, r) == 0xbeefu &&
              r.words.at(kActor + 332u) == 0xbeefu &&
              r.words.at(kActor + 668u) == 0xbeefu &&
              r.calls == std::vector<std::string>{
                  "write:4428", "write:4764"},
          "mirrored field order and return");
}

}  // namespace

int main() {
    try {
        Constructor();
        CountAccessors();
        DirectFloatAdders();
        WordGetters();
        FloatGetters();
        WordAdders();
        FloatAdders();
        DirectSetters();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
