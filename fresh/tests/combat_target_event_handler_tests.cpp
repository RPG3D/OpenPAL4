#include "pal4/combat_target_event_handler.h"

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

struct Runtime final : pal4::ICombatTargetEventRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t type = 1u;
    std::uint8_t state_three = 0u;
    std::uint32_t random = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    std::uint32_t RandomNumber() override {
        calls.push_back("random");
        return random;
    }
    std::int32_t FloatToInt(float value) override {
        calls.push_back("convert " + std::to_string(value));
        return static_cast<std::int32_t>(value);
    }
    std::uint32_t TargetType(std::uint32_t target) override {
        calls.push_back("type " + std::to_string(target));
        return type;
    }
    std::uint8_t IsStateThree(std::uint32_t target) override {
        calls.push_back("state " + std::to_string(target));
        return state_three;
    }
    void SetEventData(std::uint32_t slot, std::uint32_t target,
                      std::int32_t first, float scale,
                      std::int32_t second) override {
        calls.push_back("set " + std::to_string(slot) + " " +
                        std::to_string(target) + " " +
                        std::to_string(first) + " " +
                        std::to_string(scale) + " " +
                        std::to_string(second));
    }
    double PropertyValue(std::uint32_t target,
                         std::uint32_t property) override {
        calls.push_back("property " + std::to_string(target) + " " +
                        std::to_string(property));
        return property == 2u ? 2.75 : 3.5;
    }
    void ApplyProperty(std::uint32_t slot, float value) override {
        calls.push_back("apply " + std::to_string(slot) + " " +
                        std::to_string(value));
    }
    std::uint32_t ProcessObject(std::uint32_t slot,
                                std::uint32_t target) override {
        calls.push_back("process " + std::to_string(slot) + " " +
                        std::to_string(target));
        return 0x123u;
    }
};

void SetFloat(Runtime& runtime, std::uint32_t offset, float value) {
    runtime.words[1000u + offset] = std::bit_cast<std::uint32_t>(value);
}

void Prepare(Runtime& runtime, float first = 2.0f,
             float second = 6.0f) {
    SetFloat(runtime, 20u, 0.0f);
    SetFloat(runtime, 32u, first);
    SetFloat(runtime, 36u, 0.5f);
    SetFloat(runtime, 40u, 3.9f);
    SetFloat(runtime, 44u, second);
    SetFloat(runtime, 48u, 1.5f);
    SetFloat(runtime, 52u, -4.9f);
}

void EarlyGates() {
    Runtime type_four;
    Prepare(type_four);
    type_four.type = 4u;
    Check(pal4::CombatTargetEventHandler::Process(
              1000u, 4000u, type_four) == 4u &&
              type_four.calls == std::vector<std::string>{"type 4000"},
          "type four exits before conversion and random roll");

    Runtime failed_roll;
    Prepare(failed_roll);
    failed_roll.random = 1u;
    Check(pal4::CombatTargetEventHandler::Process(
              1000u, 4000u, failed_roll) == 0u &&
              failed_roll.calls == std::vector<std::string>{
                  "type 4000", "read 1032", "convert 2.000000",
                  "read 1044", "convert 6.000000", "random",
                  "read 1020"},
          "event ids are converted before failed probability gate");

    Runtime state_three;
    Prepare(state_three);
    state_three.state_three = 7u;
    Check(pal4::CombatTargetEventHandler::Process(
              1000u, 4000u, state_three) == 7u &&
              state_three.calls.back() == "state 4000",
          "state-three gate forwards original nonzero byte");
}

void TwoMatchingEvents() {
    Runtime runtime;
    Prepare(runtime);
    Check(pal4::CombatTargetEventHandler::Process(
              1000u, 4000u, runtime) == 0x23u &&
              runtime.calls == std::vector<std::string>{
                  "type 4000", "read 1032", "convert 2.000000",
                  "read 1044", "convert 6.000000", "random",
                  "read 1020", "state 4000", "read 1040",
                  "convert 3.900000", "read 1036",
                  "set 0 4000 2 0.500000 3", "read 1052",
                  "convert -4.900000", "read 1048",
                  "set 1 4000 6 1.500000 -4",
                  "property 4000 2", "apply 0 2.750000",
                  "process 0 4000", "property 4000 3",
                  "apply 1 3.500000", "process 1 4000"},
          "two event records use id-specific property and processing order");

    Runtime reversed;
    Prepare(reversed, 6.0f, 2.0f);
    Check(pal4::CombatTargetEventHandler::Process(
              1000u, 4000u, reversed) == 0x23u &&
              reversed.calls[16] == "property 4000 3" &&
              reversed.calls[19] == "property 4000 2",
          "reversed ids select properties three then two");
}

void UnmatchedEventIds() {
    Runtime runtime;
    Prepare(runtime, 1.0f, 8.0f);
    Check(pal4::CombatTargetEventHandler::Process(
              1000u, 4000u, runtime) == 2u &&
              runtime.calls.back() == "set 1 4000 8 1.500000 -4",
          "unmatched second id returns its difference from six");
    runtime.calls.clear();
    SetFloat(runtime, 44u, 1.0f);
    Check(pal4::CombatTargetEventHandler::Process(
              1000u, 4000u, runtime) == 251u,
          "negative id-minus-six result is narrowed to a byte");
}

}  // namespace

int main() {
    try {
        EarlyGates();
        TwoMatchingEvents();
        UnmatchedEventIds();
        std::cout << "combat target event handler tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
