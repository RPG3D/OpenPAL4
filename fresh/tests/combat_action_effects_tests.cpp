#include "pal4/combat_action_effects.h"

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

struct Runtime final : pal4::ICombatActionEffectsRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t random = 0u;
    bool further = false;
    std::int32_t stats[3]{10, 0, 5};
    std::int32_t slots[3]{7, 8, 9};
    std::uint32_t found_event = 6000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write " + std::to_string(address) + " " +
                        std::to_string(value));
        words[address] = value;
    }
    std::uint32_t RandomNumber() override {
        calls.push_back("random");
        return random;
    }
    std::int64_t FloatToInt(float value) override {
        calls.push_back("convert " + std::to_string(value));
        return static_cast<std::int64_t>(value);
    }
    bool CheckFurther(std::uint32_t context,
                      std::uint32_t value) override {
        calls.push_back("check " + std::to_string(context) + " " +
                        std::to_string(value));
        return further;
    }
    std::int32_t DispatchAction(std::uint32_t target,
                                std::uint32_t context,
                                std::uint32_t action,
                                std::uint32_t mode) override {
        calls.push_back("dispatch " + std::to_string(target) + " " +
                        std::to_string(context) + " " +
                        std::to_string(action) + " " +
                        std::to_string(mode));
        return 31;
    }
    std::int32_t GetStat(std::uint32_t actor,
                         std::uint32_t stat) override {
        calls.push_back("stat " + std::to_string(actor) + " " +
                        std::to_string(stat));
        return stats[stat];
    }
    std::int32_t AddStat(std::uint32_t actor, std::uint32_t stat,
                         std::int64_t delta) override {
        calls.push_back("add " + std::to_string(actor) + " " +
                        std::to_string(stat) + " " +
                        std::to_string(delta));
        return 41;
    }
    std::int32_t GetSlot(std::uint32_t target,
                         std::uint32_t slot) override {
        calls.push_back("get-slot " + std::to_string(target) + " " +
                        std::to_string(slot));
        return slots[slot];
    }
    void SetSlot(std::uint32_t target, std::uint32_t slot,
                 std::int32_t value) override {
        calls.push_back("set-slot " + std::to_string(target) + " " +
                        std::to_string(slot) + " " +
                        std::to_string(value));
        slots[slot] = value;
    }
    std::int32_t TransferSlots(std::uint32_t source,
                               std::int32_t first,
                               std::int32_t second) override {
        calls.push_back("transfer " + std::to_string(source) + " " +
                        std::to_string(first) + " " +
                        std::to_string(second));
        return 51;
    }
    std::int32_t TransferPositiveSlot(std::uint32_t source,
                                      std::int32_t value) override {
        calls.push_back("transfer-positive " + std::to_string(source) +
                        " " + std::to_string(value));
        return 61;
    }
    std::int32_t Fallback(std::uint32_t source) override {
        calls.push_back("fallback " + std::to_string(source));
        return 71;
    }
    std::int32_t SetProperty(std::uint32_t target,
                             std::uint32_t kind,
                             std::uint32_t value) override {
        calls.push_back("property " + std::to_string(target) + " " +
                        std::to_string(kind) + " " +
                        std::to_string(value));
        return 81;
    }
    std::uint32_t LookupEvent(std::int32_t index) override {
        calls.push_back("lookup " + std::to_string(index));
        return found_event;
    }
    std::int32_t DispatchEvent(std::uint32_t event,
                               std::uint32_t target,
                               std::uint32_t value) override {
        calls.push_back("dispatch-event " + std::to_string(event) + " " +
                        std::to_string(target) + " " +
                        std::to_string(value));
        return 91;
    }
};

constexpr std::uint32_t kAction = 1000u;
constexpr std::uint32_t kSource = 3000u;
constexpr std::uint32_t kTarget = 4000u;

void SetFloat(Runtime& runtime, std::uint32_t offset, float value) {
    runtime.words[kAction + offset] = std::bit_cast<std::uint32_t>(value);
}

void PropertyAndDirectHandlers() {
    Runtime runtime;
    runtime.words[kAction + 20u] = 0x12345678u;
    SetFloat(runtime, 24u, 1.0f);
    Check(pal4::CombatActionEffects::HandlePropertyType(
              kAction, kTarget, runtime) == 81 &&
              runtime.calls.back() == "property 4000 2 305419896",
          "type one maps to property kind two with raw action word");
    runtime.calls.clear();
    SetFloat(runtime, 24u, 3.0f);
    Check(pal4::CombatActionEffects::HandlePropertyType(
              kAction, kTarget, runtime) == 81 &&
              runtime.calls.back() == "property 4000 3 305419896",
          "type three maps to property kind three");
    runtime.calls.clear();
    SetFloat(runtime, 24u, 2.0f);
    Check(pal4::CombatActionEffects::HandlePropertyType(
              kAction, kTarget, runtime) == 2 &&
              runtime.calls == std::vector<std::string>{
                  "read 1024", "convert 2.000000"},
          "other type returns converted value without property write");

    runtime.calls.clear();
    Check(pal4::CombatActionEffects::HandlePropertyTypeFour(
              kAction, kTarget, runtime) == 81 &&
              runtime.calls == std::vector<std::string>{
                  "read 1020", "property 4000 4 305419896"},
          "direct kind-four property uses raw action word");
    runtime.calls.clear();
    SetFloat(runtime, 20u, -4.9f);
    Check(pal4::CombatActionEffects::HandleStatDeltaThree(
              kAction, kTarget, runtime) == 41 &&
              runtime.calls == std::vector<std::string>{
                  "read 1020", "convert -4.900000", "add 4000 3 -4"},
          "direct stat-three handler converts signed float delta");
}

void StatTransfer() {
    Runtime runtime;
    SetFloat(runtime, 20u, 0.5f);
    runtime.random = 501u;
    Check(pal4::CombatActionEffects::ProcessStatTransfer(
              kAction, kSource, 9000u, kTarget, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{
                  "random", "read 1020"},
          "failed probability gate skips all action calls");

    runtime.calls.clear();
    runtime.random = 0u;
    runtime.further = true;
    Check(pal4::CombatActionEffects::ProcessStatTransfer(
              kAction, kSource, 9000u, kTarget, runtime) == 31 &&
              runtime.calls == std::vector<std::string>{
                  "random", "read 1020", "check 4340 0",
                  "dispatch 4000 9000 1000 0"},
          "extended condition dispatches target virtual and returns result");

    runtime.calls.clear();
    runtime.further = false;
    SetFloat(runtime, 24u, 1.5f);
    SetFloat(runtime, 28u, 0.8f);
    Check(pal4::CombatActionEffects::ProcessStatTransfer(
              kAction, kSource, 9000u, kTarget, runtime) == 41 &&
              runtime.calls == std::vector<std::string>{
                  "random", "read 1020", "check 4340 0",
                  "read 1024", "stat 4000 0", "convert 15.000000",
                  "add 3000 7 15", "convert -15.000000",
                  "add 4000 7 -15", "read 1028", "stat 4000 2",
                  "convert 4.000000", "add 3000 9 4",
                  "convert -4.000000", "add 4000 9 -4"},
          "stat transfer writes both signed deltas for ids seven and nine");
}

void ConsumeZeroOne() {
    Runtime runtime;
    SetFloat(runtime, 20u, 0.0f);
    runtime.random = 1u;
    runtime.words[kSource + 712u] = 9u;
    Check(pal4::CombatActionEffects::ConsumeSlotsZeroOne(
              kAction, kSource, kTarget, runtime) == 9 &&
              runtime.calls == std::vector<std::string>{
                  "random", "read 1020", "read 3712"},
          "failed gate returns nonzero source guard");
    runtime.calls.clear();
    runtime.words[kSource + 712u] = 0u;
    Check(pal4::CombatActionEffects::ConsumeSlotsZeroOne(
              kAction, kSource, kTarget, runtime) == 71 &&
              runtime.calls.back() == "fallback 3000",
          "failed gate calls source fallback when guard is zero");

    runtime.calls.clear();
    runtime.random = 0u;
    Check(pal4::CombatActionEffects::ConsumeSlotsZeroOne(
              kAction, kSource, kTarget, runtime) == 51 &&
              runtime.slots[0] == 0 && runtime.slots[1] == 0 &&
              runtime.words[kTarget + 208u] == 0u &&
              runtime.words[kTarget + 212u] == 0u &&
              runtime.words[kSource + 708u] == 7u,
          "successful slot transfer clears both target slots and caches");
    Check(runtime.calls == std::vector<std::string>{
              "random", "read 1020", "get-slot 4000 0",
              "get-slot 4000 1", "set-slot 4000 0 0",
              "set-slot 4000 1 0", "write 4208 0", "write 4212 0",
              "transfer 3000 7 8", "write 3708 7"},
          "slot-zero and slot-one call order");

    runtime.calls.clear();
    runtime.slots[1] = 5;
    Check(pal4::CombatActionEffects::ConsumeSlotsZeroOne(
              kAction, kSource, kTarget, runtime) == 71 &&
              runtime.slots[1] == 0 &&
              runtime.calls.back() == "fallback 3000",
          "zero primary slot still clears both slots before fallback");
}

void ConsumeTwo() {
    Runtime runtime;
    SetFloat(runtime, 20u, 0.0f);
    runtime.random = 1u;
    runtime.words[kSource + 708u] = 6u;
    Check(pal4::CombatActionEffects::ConsumeSlotTwo(
              kAction, kSource, kTarget, runtime) == 6 &&
              runtime.calls == std::vector<std::string>{
                  "random", "read 1020", "read 3708"},
          "failed slot-two gate returns source guard");
    runtime.calls.clear();
    runtime.words[kSource + 708u] = 0u;
    Check(pal4::CombatActionEffects::ConsumeSlotTwo(
              kAction, kSource, kTarget, runtime) == 71 &&
              runtime.calls.back() == "fallback 3000",
          "failed slot-two gate falls back with zero guard");

    runtime.calls.clear();
    runtime.random = 0u;
    Check(pal4::CombatActionEffects::ConsumeSlotTwo(
              kAction, kSource, kTarget, runtime) == 61 &&
              runtime.slots[2] == 0 &&
              runtime.words[kTarget + 216u] == 0u &&
              runtime.words[kSource + 712u] == 9u &&
              runtime.calls == std::vector<std::string>{
                  "random", "read 1020", "get-slot 4000 2",
                  "set-slot 4000 2 0", "write 4216 0",
                  "transfer-positive 3000 9", "write 3712 9"},
          "positive slot-two transfer and cache update order");

    runtime.calls.clear();
    runtime.slots[2] = -2;
    runtime.words[kSource + 708u] = 4u;
    Check(pal4::CombatActionEffects::ConsumeSlotTwo(
              kAction, kSource, kTarget, runtime) == 4 &&
              runtime.slots[2] == 0 &&
              runtime.calls.back() == "read 3708",
          "negative slot-two value clears target and returns existing guard");
}

void DirectPropertiesAndIndexedDispatch() {
    Runtime runtime;
    runtime.words[kAction + 20u] = 0x12345678u;
    Check(pal4::CombatActionEffects::HandlePropertyTypeZero(
              kAction, kTarget, runtime) == 81 &&
              runtime.calls == std::vector<std::string>{
                  "read 1020", "property 4000 0 305419896"},
          "direct property zero uses raw action word");
    runtime.calls.clear();
    Check(pal4::CombatActionEffects::HandlePropertyTypeOne(
              kAction, kTarget, runtime) == 81 &&
              runtime.calls == std::vector<std::string>{
                  "read 1020", "property 4000 1 305419896"},
          "direct property one uses raw action word");

    runtime.calls.clear();
    SetFloat(runtime, 20u, 49.0f);
    Check(pal4::CombatActionEffects::DispatchIndexed(
              kAction, kTarget, runtime) == 49 &&
              runtime.calls == std::vector<std::string>{
                  "read 1020", "convert 49.000000"},
          "index below 50 returns converted value without lookup");
    runtime.calls.clear();
    SetFloat(runtime, 20u, 57.0f);
    Check(pal4::CombatActionEffects::DispatchIndexed(
              kAction, kTarget, runtime) == 57 &&
              runtime.calls.size() == 2u,
          "index above 56 skips lookup");
    runtime.calls.clear();
    SetFloat(runtime, 20u, 50.0f);
    runtime.words[kAction + 24u] = 0xfeedu;
    Check(pal4::CombatActionEffects::DispatchIndexed(
              kAction, kTarget, runtime) == 91 &&
              runtime.calls == std::vector<std::string>{
                  "read 1020", "convert 50.000000", "lookup 50",
                  "read 1024", "dispatch-event 6000 4000 65261"},
          "lower index boundary dispatches found event with raw parameter");
    runtime.calls.clear();
    SetFloat(runtime, 20u, 56.0f);
    runtime.found_event = 0u;
    Check(pal4::CombatActionEffects::DispatchIndexed(
              kAction, kTarget, runtime) == 0 &&
              runtime.calls.back() == "lookup 56" &&
              runtime.calls.size() == 3u,
          "upper index boundary with missing event returns zero");
}

}  // namespace

int main() {
    try {
        PropertyAndDirectHandlers();
        StatTransfer();
        ConsumeZeroOne();
        ConsumeTwo();
        DirectPropertiesAndIndexedDispatch();
        std::cout << "combat action effects tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
