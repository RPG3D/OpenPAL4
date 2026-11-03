#include "pal4/actor_damage_event_setters.h"

#include <bit>
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

constexpr std::uint32_t kActor = 0x1000u;
constexpr std::uint32_t kRecord = 0x5000u;
constexpr std::uint32_t kRouter = 0x6000u;

struct Runtime final : pal4::IActorDamageEventRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::int32_t> int_reads;
    std::vector<double> float_reads;
    std::vector<std::int32_t> state_reads;
    std::size_t int_cursor = 0u;
    std::size_t float_cursor = 0u;
    std::size_t state_cursor = 0u;
    std::uint32_t first_get_offset = 0u;
    std::uint32_t last_get_offset = 0u;
    std::uint32_t event_key = 0u;
    std::uint32_t event_count = 0u;
    std::uint32_t state_get_offset = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void ResetTemporaryHandle(std::uint32_t&,
                              std::uint32_t&) override {
        throw std::runtime_error("unexpected temporary handle");
    }
    std::int32_t ActorIntVirtual(std::uint32_t actor,
                                 std::uint32_t offset,
                                 std::uint32_t index) override {
        Check(actor == kActor && index <= 9u,
              "integer getter arguments");
        calls.emplace_back("get_int");
        if (int_cursor == 0u) first_get_offset = offset;
        last_get_offset = offset;
        Check(int_cursor < int_reads.size(), "integer getter count");
        return int_reads[int_cursor++];
    }
    double ActorFloatVirtual(std::uint32_t actor,
                             std::uint32_t offset,
                             std::uint32_t index) override {
        Check(actor == kActor && index <= 9u,
              "float getter arguments");
        calls.emplace_back("get_float");
        if (float_cursor == 0u && int_cursor == 0u)
            first_get_offset = offset;
        last_get_offset = offset;
        Check(float_cursor < float_reads.size(), "float getter count");
        return float_reads[float_cursor++];
    }
    std::int32_t ActorStateVirtual(std::uint32_t actor,
                                   std::uint32_t offset) override {
        Check(actor == kActor && offset >= 136u && offset <= 200u &&
                  (offset - 136u) % 8u == 0u,
              "state getter arguments");
        state_get_offset = offset;
        calls.emplace_back("get_state");
        Check(state_cursor < state_reads.size(), "state getter count");
        return state_reads[state_cursor++];
    }
    void ActorVoidVirtual(std::uint32_t actor,
                          std::uint32_t offset) override {
        Check(actor == kActor && offset == 548u,
              "state follow-up virtual arguments");
        calls.emplace_back("state_follow_up");
    }
    void SetValidatedProperty(std::uint32_t property,
                              std::uint32_t index,
                              std::int32_t value) override {
        Check(property == kActor + 340u && index < 10u,
              "validated property arguments");
        calls.emplace_back("validate:" + std::to_string(index) + ":" +
                           std::to_string(value));
    }
    void UpdateHealthStatus(std::uint32_t actor) override {
        Check(actor == kActor, "health update actor");
        calls.emplace_back("health");
    }
    std::uint32_t TemporaryRecord() override {
        calls.emplace_back("temporary_record");
        return kRecord;
    }
    std::uint32_t EventRouter() override {
        calls.emplace_back("router");
        return kRouter;
    }
    void DispatchEvent(std::uint32_t router,
                       std::uint32_t key,
                       std::uint32_t record,
                       std::uint32_t mode) override {
        Check(router == kRouter && record == kRecord && mode == 1u,
              "event dispatch arguments");
        event_key = key;
        ++event_count;
        calls.emplace_back("dispatch");
    }
};

using IntSetter = std::int32_t (*)(
    std::uint32_t, std::uint32_t, std::int32_t,
    pal4::IActorDamageEventRuntime&);
using FloatSetter = double (*)(
    std::uint32_t, std::int32_t, float,
    pal4::IActorDamageEventRuntime&);

void IntSetters() {
    struct Case {
        IntSetter setter;
        std::uint32_t base, max, getter, key_address;
    };
    const Case cases[] = {
        {pal4::ActorDamageEventSetters::SetWord344,
         344u, 7u, 48u, 0x8b5fa4u},
        {pal4::ActorDamageEventSetters::SetWord464,
         464u, 4u, 28u, 0x8b5fb8u},
        {pal4::ActorDamageEventSetters::SetWord392,
         392u, 5u, 88u, 0x8b5facu},
        {pal4::ActorDamageEventSetters::SetCountWord344,
         344u, 7u, 48u, 0x8b62f8u},
        {pal4::ActorDamageEventSetters::SetCountWord464,
         464u, 4u, 28u, 0x8b630cu},
        {pal4::ActorDamageEventSetters::SetCountWord392,
         392u, 5u, 88u, 0x8b6300u}};
    for (const auto& item : cases) {
        Runtime r;
        Check(item.setter(kActor, item.max + 1u, 12, r) ==
                  static_cast<std::int32_t>(item.max + 1u) &&
                  r.calls.empty(), "integer setter bound");
        r = Runtime{};
        r.int_reads = {12};
        Check(item.setter(kActor, item.max, 12, r) == 12 &&
                  r.event_count == 0u && r.int_cursor == 1u &&
                  r.calls == std::vector<std::string>{"get_int"},
              "integer setter equal short circuit");
        r = Runtime{};
        r.int_reads = {10, 99};
        r.words[item.key_address] = 0x7000u;
        Check(item.setter(kActor, item.max, 12, r) == 99 &&
                  r.words.at(kActor + item.base + 4u * item.max) == 12u &&
                  r.words.at(kRecord) == kActor + 716u &&
                  r.words.at(kRecord + 4u) == item.max &&
                  r.words.at(kRecord + 8u) == 10u &&
                  r.first_get_offset == item.getter &&
                  r.last_get_offset == item.getter &&
                  r.event_key == 0x7000u &&
                  r.event_count == 1u && r.int_cursor == 2u,
              "integer setter layout, event and reread");
        Check(r.calls == std::vector<std::string>{
                  "get_int",
                  "write:" + std::to_string(
                      kActor + item.base + 4u * item.max),
                  "temporary_record", "write:20480", "write:20484",
                  "write:20488",
                  "read:" + std::to_string(item.key_address),
                  "router", "dispatch", "get_int"},
              "integer setter event order");
    }
}

void FloatSetters() {
    struct Case {
        FloatSetter setter;
        std::uint32_t base, max, old_getter, result_getter, key_address;
        bool integer_previous;
    };
    const Case cases[] = {
        {pal4::ActorDamageEventSetters::SetFloat376,
         376u, 3u, 44u, 44u, 0x8b5fa8u, false},
        {pal4::ActorDamageEventSetters::SetFloat416FromInt,
         416u, 5u, 88u, 100u, 0x8b5fb0u, true},
        {pal4::ActorDamageEventSetters::SetFloat440,
         440u, 5u, 112u, 112u, 0x8b5fb4u, false},
        {pal4::ActorDamageEventSetters::SetFloat584,
         584u, 4u, 40u, 40u, 0x8b5fc0u, false},
        {pal4::ActorDamageEventSetters::SetFloat604,
         604u, 6u, 124u, 124u, 0x8b5fc4u, false},
        {pal4::ActorDamageEventSetters::SetFloat484,
         484u, 5u, 216u, 216u, 0x8b5fccu, false},
        {pal4::ActorDamageEventSetters::SetFloat508,
         508u, 6u, 212u, 212u, 0x8b5fd0u, false},
        {pal4::ActorDamageEventSetters::SetFloat536,
         536u, 1u, 208u, 208u, 0x8b5fd4u, false},
        {pal4::ActorDamageEventSetters::SetCountFloat376,
         376u, 3u, 44u, 44u, 0x8b62fcu, false},
        {pal4::ActorDamageEventSetters::SetCountFloat416FromInt,
         416u, 5u, 88u, 100u, 0x8b6304u, true},
        {pal4::ActorDamageEventSetters::SetCountFloat440,
         440u, 5u, 112u, 112u, 0x8b6308u, false},
        {pal4::ActorDamageEventSetters::SetCountFloat584,
         584u, 4u, 40u, 40u, 0x8b6314u, false},
        {pal4::ActorDamageEventSetters::SetCountFloat604,
         604u, 6u, 124u, 124u, 0x8b6318u, false},
        {pal4::ActorDamageEventSetters::SetCountFloat484,
         484u, 5u, 216u, 216u, 0x8b6320u, false},
        {pal4::ActorDamageEventSetters::SetCountFloat508,
         508u, 6u, 212u, 212u, 0x8b6324u, false},
        {pal4::ActorDamageEventSetters::SetCountFloat536,
         536u, 1u, 208u, 208u, 0x8b6328u, false}};
    for (const auto& item : cases) {
        Runtime r;
        Check(item.setter(kActor, -1, 2.5f, r) == -1.0 &&
                  r.calls.empty(), "float setter negative index");
        Check(item.setter(kActor,
                          static_cast<std::int32_t>(item.max + 1u),
                          2.5f, r) ==
                  static_cast<double>(item.max + 1u) &&
                  r.calls.empty(), "float setter upper bound");
        r = Runtime{};
        if (item.integer_previous) r.int_reads = {2};
        else r.float_reads = {2.5};
        const auto equal_value = item.integer_previous ? 2.0f : 2.5f;
        Check(item.setter(kActor, 0, equal_value, r) ==
                  static_cast<double>(equal_value) &&
                  r.event_count == 0u && r.calls.size() == 1u,
              "float setter equal short circuit");
        r = Runtime{};
        if (item.integer_previous) {
            r.int_reads = {2};
            r.float_reads = {3.75};
        } else {
            r.float_reads = {1.25, 3.75};
        }
        r.words[item.key_address] = 0x7100u;
        Check(item.setter(kActor,
                          static_cast<std::int32_t>(item.max),
                          2.5f, r) == 3.75 &&
                  r.words.at(kActor + item.base + 4u * item.max) ==
                      std::bit_cast<std::uint32_t>(2.5f) &&
                  r.words.at(kRecord) == kActor + 716u &&
                  r.words.at(kRecord + 4u) == item.max &&
                  r.words.at(kRecord + 8u) ==
                      std::bit_cast<std::uint32_t>(
                          item.integer_previous ? 2.0f : 1.25f) &&
                  r.first_get_offset == item.old_getter &&
                  r.last_get_offset == item.result_getter &&
                  r.event_key == 0x7100u && r.event_count == 1u,
              "float setter bits, event and result getter");
    }
}

void StateSetter() {
    Runtime r;
    r.state_reads = {2, 2};
    Check(pal4::ActorDamageEventSetters::SetState632(
              kActor, 2u, r) == 2 &&
              r.event_count == 0u && r.state_cursor == 2u,
          "equal state still reread");
    r = Runtime{};
    r.state_reads = {1, 3};
    r.words[0x8b5fc8u] = 0x7200u;
    Check(pal4::ActorDamageEventSetters::SetState632(
              kActor, 2u, r) == 3 &&
              r.words.at(kActor + 632u) == 2u &&
              r.words.at(kRecord) == kActor + 716u &&
              r.words.at(kRecord + 4u) == 0u &&
              r.words.at(kRecord + 8u) == 1u &&
              r.event_key == 0x7200u,
          "state write, event and reread");
    r = Runtime{};
    r.state_reads = {1, 4};
    r.words[0x8b5fc8u] = 0x7200u;
    Check(pal4::ActorDamageEventSetters::SetState632(
              kActor, 8u, r) == 4 &&
              r.words.find(kActor + 632u) == r.words.end() &&
              r.event_count == 1u,
          "invalid changed state still emits event");
}

void StateSetters() {
    using StateSetter = std::int32_t (*)(
        std::uint32_t, std::uint32_t,
        pal4::IActorDamageEventRuntime&);
    struct Case {
        StateSetter setter;
        std::uint32_t index, maximum, key_address;
    };
    const Case cases[] = {
        {pal4::ActorDamageEventSetters::SetState632, 0u, 5u, 0x8b5fc8u},
        {pal4::ActorDamageEventSetters::SetState636, 1u, 6u, 0x8b5fc8u},
        {pal4::ActorDamageEventSetters::SetState640, 2u, 6u, 0x8b5fc8u},
        {pal4::ActorDamageEventSetters::SetState644, 3u, 1u, 0x8b5fc8u},
        {pal4::ActorDamageEventSetters::SetState648, 4u, 3u, 0x8b5fc8u},
        {pal4::ActorDamageEventSetters::SetState652, 5u, 1u, 0x8b5fc8u},
        {pal4::ActorDamageEventSetters::SetState656, 6u, 4u, 0x8b5fc8u},
        {pal4::ActorDamageEventSetters::SetState660, 7u, 2u, 0x8b5fc8u},
        {pal4::ActorDamageEventSetters::SetState664, 8u, 5u, 0x8b5fc8u},
        {pal4::ActorDamageEventSetters::SetCountState632, 0u, 5u, 0x8b631cu},
        {pal4::ActorDamageEventSetters::SetCountState636, 1u, 6u, 0x8b631cu},
        {pal4::ActorDamageEventSetters::SetCountState640, 2u, 6u, 0x8b631cu},
        {pal4::ActorDamageEventSetters::SetCountState644, 3u, 1u, 0x8b631cu},
        {pal4::ActorDamageEventSetters::SetCountState648, 4u, 3u, 0x8b631cu},
        {pal4::ActorDamageEventSetters::SetCountState652, 5u, 1u, 0x8b631cu},
        {pal4::ActorDamageEventSetters::SetCountState656, 6u, 4u, 0x8b631cu},
        {pal4::ActorDamageEventSetters::SetCountState660, 7u, 2u, 0x8b631cu},
        {pal4::ActorDamageEventSetters::SetCountState664, 8u, 5u, 0x8b631cu}};
    for (const auto& item : cases) {
        const auto field = kActor + 632u + 4u * item.index;
        Runtime r;
        r.state_reads = {0, 7};
        r.words[item.key_address] = 0x7200u;
        Check(item.setter(kActor, item.maximum, r) == 7 &&
                  r.words.at(field) == item.maximum &&
                  r.words.at(kRecord) == kActor + 716u &&
                  r.words.at(kRecord + 4u) == item.index &&
                  r.words.at(kRecord + 8u) == 0u &&
                  r.state_get_offset == 136u + 8u * item.index &&
                  r.event_count == 1u && r.event_key == 0x7200u,
              "state setter maximum and event record");
        r = Runtime{};
        r.state_reads = {0, 9};
        r.words[item.key_address] = 0x7200u;
        Check(item.setter(kActor, item.maximum + 1u, r) == 9 &&
                  !r.words.contains(field) && r.event_count == 1u &&
                  r.state_cursor == 2u,
              "state setter invalid value still emits and rereads");
        r = Runtime{};
        r.state_reads = {1, 9};
        Check(item.setter(kActor, 1u, r) == 9 &&
                  r.calls == std::vector<std::string>{
                      "get_state", "get_state"},
              "state setter unchanged value reread");
    }
    Runtime r;
    r.state_reads = {3, 1};
    r.words[0x8b5fc8u] = 0x7200u;
    Check(pal4::ActorDamageEventSetters::SetState648(
              kActor, 2u, r) == 1 &&
              r.calls.back() == "get_state" &&
              r.calls[r.calls.size() - 2u] == "state_follow_up",
          "state four follow-up after event");
    r = Runtime{};
    r.state_reads = {2, 1};
    r.words[0x8b5fc8u] = 0x7200u;
    (void)pal4::ActorDamageEventSetters::SetState648(
        kActor, 3u, r);
    Check(r.calls.back() == "get_state" &&
              r.calls[r.calls.size() - 2u] == "dispatch",
          "state four follow-up depends on old state");
    r = Runtime{};
    r.state_reads = {3, 1};
    r.words[0x8b631cu] = 0x7201u;
    Check(pal4::ActorDamageEventSetters::SetCountState648(
              kActor, 2u, r) == 1 &&
              r.event_key == 0x7201u &&
              r.calls[r.calls.size() - 2u] == "state_follow_up",
          "count state four follows up after its own event");
}

void CountExtendedWordSetter() {
    Runtime r;
    Check(pal4::ActorDamageEventSetters::SetCountExtendedWord(
              kActor, -1, 9, r) == -1 &&
              pal4::ActorDamageEventSetters::SetCountExtendedWord(
                  kActor, 10, 9, r) == 10 && r.calls.empty(),
          "count extended word index bounds");
    r.int_reads = {3, 8};
    r.words[0x8b6310u] = 0x7301u;
    Check(pal4::ActorDamageEventSetters::SetCountExtendedWord(
              kActor, 4, 9, r) == 8 &&
              r.words.at(kActor + 224u) == 9u &&
              r.words.at(kRecord + 4u) == 4u &&
              r.words.at(kRecord + 8u) == 3u &&
              r.words.at(kRecord + 12u) == 9u &&
              r.event_key == 0x7301u && r.event_count == 1u &&
              r.calls[r.calls.size() - 2u] == "dispatch",
          "count extended word mirror, record and event key");
    r = Runtime{};
    r.int_reads = {3, 8};
    r.words[0x8b6310u] = 0x7301u;
    Check(pal4::ActorDamageEventSetters::SetCountExtendedWord(
              kActor, 7, 9, r) == 8 &&
              r.calls[2] == "health" &&
              r.calls[r.calls.size() - 2u] == "health",
          "count extended word health update order");
}

void ExtendedWordSetter() {
    Runtime r;
    Check(pal4::ActorDamageEventSetters::SetExtendedWord(
              kActor, -1, 9, r) == -1 &&
              pal4::ActorDamageEventSetters::SetExtendedWord(
                  kActor, 10, 9, r) == 10 && r.calls.empty(),
          "extended word index bounds");
    r = Runtime{};
    r.int_reads = {3, 8};
    r.words[0x8b5fbcu] = 0x7300u;
    Check(pal4::ActorDamageEventSetters::SetExtendedWord(
              kActor, 4, 9, r) == 8 &&
              r.words.at(kActor + 224u) == 9u &&
              r.words.at(kRecord) == kActor + 716u &&
              r.words.at(kRecord + 4u) == 4u &&
              r.words.at(kRecord + 8u) == 3u &&
              r.words.at(kRecord + 12u) == 9u &&
              r.first_get_offset == 52u &&
              r.event_key == 0x7300u,
          "extended word special field and four-word event");
    Check(r.calls[1] == "write:" + std::to_string(kActor + 224u) &&
              r.calls[2] == "validate:4:9" &&
              r.calls[r.calls.size() - 2u] == "dispatch",
          "extended word validation and event order");
    r = Runtime{};
    r.int_reads = {3, 8};
    r.words[0x8b5fbcu] = 0x7300u;
    Check(pal4::ActorDamageEventSetters::SetExtendedWord(
              kActor, 7, 9, r) == 8 &&
              r.calls[1] == "validate:7:9" &&
              r.calls[2] == "health" &&
              r.calls[r.calls.size() - 2u] == "health" &&
              r.calls.back() == "get_int",
          "extended word health updates bracket event");
    r = Runtime{};
    r.int_reads = {3, 8};
    Check(pal4::ActorDamageEventSetters::SetExtendedWord(
              kActor, 7, 3, r) == 8 &&
              r.calls == std::vector<std::string>{
                  "get_int", "health", "get_int"},
          "equal health property still refreshes status");
}

void DirectFloatGetters() {
    using Getter = double (*)(std::uint32_t, std::int32_t,
                              pal4::IActorDamageEventRuntime&);
    struct Case { Getter getter; std::uint32_t base, maximum; };
    const Case cases[] = {
        {pal4::ActorDamageEventSetters::GetFloat484, 484u, 5u},
        {pal4::ActorDamageEventSetters::GetFloat508, 508u, 6u},
        {pal4::ActorDamageEventSetters::GetFloat536, 536u, 1u}};
    for (const auto& item : cases) {
        Runtime r;
        Check(item.getter(kActor, -1, r) == -1.0 &&
                  item.getter(kActor,
                              static_cast<std::int32_t>(item.maximum + 1u),
                              r) == static_cast<double>(item.maximum + 1u) &&
                  r.calls.empty(), "direct float getter bounds");
        r.words[kActor + item.base] = std::bit_cast<std::uint32_t>(1.25f);
        r.words[kActor + item.base + 4u * item.maximum] =
            std::bit_cast<std::uint32_t>(-2.5f);
        Check(item.getter(kActor, 0, r) == 1.25 &&
                  item.getter(kActor,
                              static_cast<std::int32_t>(item.maximum),
                              r) == -2.5,
              "direct float getter first and last slots");
    }
}

}  // namespace

int main() {
    try {
        IntSetters();
        FloatSetters();
        StateSetter();
        StateSetters();
        ExtendedWordSetter();
        CountExtendedWordSetter();
        DirectFloatGetters();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
