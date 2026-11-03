#include "pal4/combat_actor_property_callbacks.h"

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

constexpr std::uint32_t kCallback = 0x1000u;
constexpr std::uint32_t kOwner = 0x2000u;
constexpr std::uint32_t kActor = 0x3000u;

struct Runtime final : pal4::ICombatActorPropertyRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kCallback + 8u, kOwner}};
    std::vector<std::string> calls;
    std::vector<std::int32_t> slot_values;
    std::size_t slot_cursor = 0u;
    std::int32_t virtual48_result = 0;
    std::int32_t virtual68_result = 68;
    std::int32_t virtual84_result = 84;
    std::int32_t forwarded_result = 168;
    std::uint8_t owner_result = 0u;
    std::uint32_t virtual268_result = 0u;
    std::uint32_t last_index = 0u;
    std::int32_t last_value = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read_owner");
        return words.at(address);
    }
    std::int32_t ActorVirtual48(std::uint32_t actor,
                                std::uint32_t index) override {
        Check(actor == kActor && (index == 0u || index == 2u),
              "virtual +48 arguments");
        calls.emplace_back("virtual48");
        last_index = index;
        return virtual48_result;
    }
    std::int32_t ActorVirtual52(std::uint32_t actor,
                                std::uint32_t index) override {
        Check(actor == kActor && (index == 7u || index == 9u),
              "virtual +52 arguments");
        calls.emplace_back("virtual52");
        Check(slot_cursor < slot_values.size(), "slot read count");
        last_index = index;
        return slot_values[slot_cursor++];
    }
    std::int32_t ActorVirtual68(std::uint32_t actor,
                                std::uint32_t index,
                                std::int32_t value) override {
        Check(actor == kActor, "virtual +68 actor");
        calls.emplace_back("virtual68");
        last_index = index;
        last_value = value;
        return virtual68_result;
    }
    std::int32_t ActorVirtual84(std::uint32_t actor,
                                std::uint32_t index,
                                std::int32_t value) override {
        Check(actor == kActor, "virtual +84 actor");
        calls.emplace_back("virtual84");
        last_index = index;
        last_value = value;
        return virtual84_result;
    }
    std::int32_t OwnerVirtual168(std::uint32_t owner,
                                 std::uint32_t actor,
                                 std::uint32_t index,
                                 std::int32_t value) override {
        Check(owner == kOwner && actor == kActor,
              "owner virtual +168 arguments");
        calls.emplace_back("forward");
        last_index = index;
        last_value = value;
        return forwarded_result;
    }
    std::uint8_t SharesOwner(std::uint32_t actor,
                             std::uint32_t other) override {
        Check(actor == kActor && other == 0x4000u,
              "owner check arguments");
        calls.emplace_back("shares_owner");
        return owner_result;
    }
    std::uint32_t ActorVirtual268(std::uint32_t actor,
                                  std::uint32_t code,
                                  std::uint32_t source) override {
        Check(actor == kActor && source == kActor && code == 7312u,
              "virtual +268 arguments");
        calls.emplace_back("virtual268");
        return virtual268_result;
    }
};

void SlotNineDecrease() {
    Runtime r;
    Check(pal4::CombatActorPropertyCallbacks::AdjustSlotNineDecrease(
              kCallback, kActor, 7u, 80, r) == 168 &&
              r.last_value == 80 && r.last_index == 7u &&
              r.calls == std::vector<std::string>{
                  "read_owner", "forward"},
          "other slot forwards unchanged");
    r = Runtime{};
    r.slot_values = {80};
    (void)pal4::CombatActorPropertyCallbacks::AdjustSlotNineDecrease(
        kCallback, kActor, 9u, 80, r);
    Check(r.slot_cursor == 1u && r.last_value == 80,
          "slot nine comparison equality");
    r = Runtime{};
    r.slot_values = {100, 110};
    (void)pal4::CombatActorPropertyCallbacks::AdjustSlotNineDecrease(
        kCallback, kActor, 9u, 80, r);
    Check(r.last_value == 72 && r.slot_cursor == 2u &&
              r.calls == std::vector<std::string>{
                  "virtual52", "virtual52", "read_owner", "forward"},
          "slot nine uses second read and native float constant");
}

void SlotNinePercent() {
    Runtime r;
    r.virtual48_result = 101;
    r.slot_values = {5};
    Check(pal4::CombatActorPropertyCallbacks::ApplySlotNinePercent(
              kActor, r) == 68 && r.last_index == 9u &&
              r.last_value == 1 &&
              r.calls == std::vector<std::string>{
                  "virtual48", "virtual52", "virtual68"},
          "slot nine below five percent");
    r = Runtime{};
    r.virtual48_result = 100;
    r.slot_values = {5};
    Check(pal4::CombatActorPropertyCallbacks::ApplySlotNinePercent(
              kActor, r) == 84 && r.last_value == -5 &&
              r.calls.back() == "virtual84",
          "slot nine equality takes decrement path");
}

void DifferentOwner() {
    Runtime r;
    r.owner_result = 0x81u;
    Check(pal4::CombatActorPropertyCallbacks::NotifyDifferentOwner(
              kActor, 0x4000u, r) == 0x81u &&
              r.calls == std::vector<std::string>{"shares_owner"},
          "nonzero owner byte forwarded");
    r = Runtime{};
    r.virtual268_result = 0x1234u;
    Check(pal4::CombatActorPropertyCallbacks::NotifyDifferentOwner(
              kActor, 0x4000u, r) == 0x34u &&
              r.calls == std::vector<std::string>{
                  "shares_owner", "virtual268"},
          "notification result narrowed to byte");
}

void SlotSevenPercent() {
    Runtime r;
    r.virtual48_result = 101;
    r.slot_values = {4};
    Check(pal4::CombatActorPropertyCallbacks::ApplySlotSevenPercent(
              kActor, r) == 68 && r.last_index == 7u &&
              r.last_value == 1,
          "slot seven below five percent");
    r = Runtime{};
    r.virtual48_result = 100;
    r.slot_values = {5};
    Check(pal4::CombatActorPropertyCallbacks::ApplySlotSevenPercent(
              kActor, r) == 84 && r.last_index == 7u &&
              r.last_value == -5,
          "slot seven equality decrements");
}

void ReflectSlotSeven() {
    Runtime r;
    Check(pal4::CombatActorPropertyCallbacks::ReflectSlotSevenExcess(
              kCallback, kActor, 9u, 120, r) == 168 &&
              r.last_value == 120 && r.slot_cursor == 0u,
          "other slot unchanged");
    r = Runtime{};
    r.slot_values = {100, 90};
    (void)pal4::CombatActorPropertyCallbacks::ReflectSlotSevenExcess(
        kCallback, kActor, 7u, 120, r);
    Check(r.slot_cursor == 2u && r.last_value == 60 &&
              r.calls == std::vector<std::string>{
                  "virtual52", "virtual52", "read_owner", "forward"},
          "reflect uses second current value");
}

void ScaleSlotSevenNine() {
    for (const auto index : {7u, 9u}) {
        Runtime r;
        r.slot_values = {100, 90};
        (void)pal4::CombatActorPropertyCallbacks::ScaleSlotSevenNineExcess(
            kCallback, kActor, index, 120, r);
        Check(r.slot_cursor == 2u && r.last_index == index &&
                  r.last_value == 110,
              "scaled excess uses second read and native -0.7");
    }
    Runtime r;
    (void)pal4::CombatActorPropertyCallbacks::ScaleSlotSevenNineExcess(
        kCallback, kActor, 2u, 120, r);
    Check(r.slot_cursor == 0u && r.last_value == 120,
          "unrelated slot unchanged");
    r = Runtime{};
    r.slot_values = {120};
    (void)pal4::CombatActorPropertyCallbacks::ScaleSlotSevenNineExcess(
        kCallback, kActor, 7u, 120, r);
    Check(r.slot_cursor == 1u && r.last_value == 120,
          "equal slot avoids second read");
}

}  // namespace

int main() {
    try {
        SlotNineDecrease();
        SlotNinePercent();
        DifferentOwner();
        SlotSevenPercent();
        ReflectSlotSeven();
        ScaleSlotSevenNine();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
