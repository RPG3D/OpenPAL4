#include "pal4/game_data_equipment_text_formatter.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataEquipmentTextRuntime {
    bool allocation_succeeds = true;
    std::vector<std::string> calls;
    std::uint32_t expected_destination = 0x7000;
    std::string assigned;

    bool PrepareTemporary(std::string_view text) override {
        calls.emplace_back("prepare " + std::string(text));
        return allocation_succeeds;
    }
    void InitializeDestination(std::uint32_t destination,
                               std::uint8_t seed) override {
        Check(destination == expected_destination && seed == 0x5a,
              "destination and explicit stack seed");
        calls.emplace_back("initialize");
    }
    void Assign(std::uint32_t destination,
                std::string_view text) override {
        Check(destination == expected_destination,
              "assignment destination");
        assigned = text;
        calls.emplace_back("assign");
    }
    void ReleaseTemporary() override { calls.emplace_back("release"); }
};

void CheckIntegers() {
    Runtime runtime;
    Check(pal4::GameDataEquipmentTextFormatter::FromInteger(
              0x7000, -2147483647 - 1, 0x5a, runtime) == 0x7000 &&
              runtime.assigned == "-2147483648" &&
              runtime.calls == std::vector<std::string>{
                                   "prepare -2147483648", "initialize",
                                   "assign", "release"},
          "minimum signed value formats in decimal and releases temporary");
    runtime.calls.clear();
    Check(pal4::GameDataEquipmentTextFormatter::FromInteger(
              0x7000, 0, 0x5a, runtime) == 0x7000 &&
              runtime.assigned == "0",
          "zero formats as one digit");
    runtime.calls.clear();
    runtime.allocation_succeeds = false;
    Check(pal4::GameDataEquipmentTextFormatter::FromInteger(
              0x7000, 42, 0x5a, runtime) == 0x7000 &&
              runtime.assigned.empty() &&
              runtime.calls == std::vector<std::string>{
                                   "prepare 42", "initialize", "assign"},
          "failed temporary allocation assigns empty text without release");
}

void CheckFormattedText() {
    Runtime runtime;
    Check(pal4::GameDataEquipmentTextFormatter::Format(
              0x7000, 0x5a, runtime, "%s:%d%%", "Rate", -25) == 0x7000 &&
              runtime.assigned == "Rate:-25%" &&
              runtime.calls == std::vector<std::string>{
                                   "prepare Rate:-25%", "initialize",
                                   "assign", "release"},
          "printf formatting preserves argument order and punctuation");
    runtime.calls.clear();
    runtime.allocation_succeeds = false;
    Check(pal4::GameDataEquipmentTextFormatter::Format(
              0x7000, 0x5a, runtime, "%d", 17) == 0x7000 &&
              runtime.assigned.empty() &&
              runtime.calls == std::vector<std::string>{
                                   "prepare 17", "initialize", "assign"},
          "formatted temporary allocation failure keeps destination flow");
    runtime.calls.clear();
    bool rejected = false;
    try {
        (void)pal4::GameDataEquipmentTextFormatter::Format(
            0x7000, 0x5a, runtime, "%01024d", 1);
    } catch (const std::length_error&) {
        rejected = true;
    }
    Check(rejected && runtime.calls.empty(),
          "oversized formatting rejects before touching runtime state");
}

}  // namespace

int main() {
    try {
        CheckIntegers();
        CheckFormattedText();
        std::cout << "game_data_equipment_text_formatter_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
