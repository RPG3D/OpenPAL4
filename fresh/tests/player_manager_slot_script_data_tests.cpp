#include "pal4/player_manager_slot_script_data.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::size_t IndexOf(const std::vector<std::string>& events,
                    const std::string& wanted) {
    for (std::size_t index = 0u; index < events.size(); ++index)
        if (events[index] == wanted) return index;
    Check(false, "expected script-data event missing");
    return events.size();
}

struct Runtime final : pal4::PlayerManagerSlotScriptRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint32_t> values;
    std::vector<std::uint32_t> max0{10u};
    std::vector<std::uint32_t> max2{5u};
    std::size_t max0_calls{};
    std::size_t max2_calls{};
    std::vector<std::string> events;
    std::uint32_t expected_object{0x5000u};

    Runtime() {
        for (std::uint32_t index = 0u; index < 0x54u; ++index)
            words[expected_object + 4u + index * 4u] = 0xa000u + index;
    }
    std::uint32_t GetValue(std::uint32_t object,
                           std::uint32_t index) override {
        Check(object == expected_object, "value object");
        events.push_back("get:" + std::to_string(index));
        return values[index];
    }
    std::uint32_t GetMaximum(std::uint32_t object,
                             std::uint32_t index) override {
        Check(object == expected_object && (index == 0u || index == 2u),
              "maximum object and index");
        events.push_back("max:" + std::to_string(index));
        if (index == 0u) {
            const auto position = max0_calls++;
            return max0[position < max0.size() ? position
                                                : max0.size() - 1u];
        }
        const auto position = max2_calls++;
        return max2[position < max2.size() ? position
                                            : max2.size() - 1u];
    }
    std::uint32_t SetValue(std::uint32_t object,
                           std::uint32_t index,
                           std::uint32_t value) override {
        Check(object == expected_object, "setter object");
        events.push_back("set:" + std::to_string(index));
        values[index] = value;
        return 0xfeedu;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words[address];
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        words[address] = value;
        if (address == expected_object + 0x154u + 0x53u * 4u)
            events.emplace_back("copy-last");
    }
    void ProcessInventory(std::uint32_t slots, std::uint32_t copy,
                          std::uint32_t original) override {
        Check(slots == expected_object + 0x2ccu &&
                  copy == expected_object + 0x154u &&
                  original == expected_object + 4u,
              "inventory process operands");
        for (std::uint32_t index = 0u; index < 0x54u; ++index)
            Check(words[copy + index * 4u] == 0xa000u + index,
                  "all 84 DWORDs are copied before inventory processing");
        events.emplace_back("inventory");
    }
    void ProcessSkills(std::uint32_t object,
                       std::uint32_t mode,
                       std::uint32_t marker) override {
        Check(object == expected_object && mode == 1u &&
                  marker == (object == 0x654u ? 0u : object),
              "skill batch object, mode and special marker");
        events.emplace_back("skills");
    }
};

void ClampAndCallOrder() {
    Runtime runtime;
    runtime.values = {{7u, 12u}, {9u, 9u}, {8u, 4u}};
    runtime.max0 = {10u, 8u, 7u};
    runtime.max2 = {5u};
    Check(pal4::PlayerManagerSlotScriptData::Refresh(
              0x5000u, runtime) == 0xfeedu &&
              runtime.values[7u] == 8u && runtime.values[9u] == 7u &&
              runtime.values[8u] == 4u &&
              runtime.max0_calls == 3u && runtime.max2_calls == 1u,
          "both signed limits clamp with repeated index-zero lookup");
    const auto& events = runtime.events;
    Check(events[0] == "get:7" && events[1] == "get:9" &&
              events[2] == "get:8" &&
              IndexOf(events, "copy-last") < IndexOf(events, "inventory") &&
              IndexOf(events, "inventory") < IndexOf(events, "skills") &&
              IndexOf(events, "skills") < IndexOf(events, "set:7") &&
              events.back() == "set:9",
          "read, copy, inventory, skills, restore and clamp order");
}

void InRangeAndSpecialMarker() {
    Runtime runtime;
    runtime.values = {{7u, 5u}, {9u, 3u}, {8u, 2u}};
    Check(pal4::PlayerManagerSlotScriptData::Refresh(
              0x5000u, runtime) == 5u &&
              runtime.max0_calls == 1u && runtime.max2_calls == 1u &&
              runtime.values[7u] == 5u && runtime.values[9u] == 3u &&
              runtime.events.back() == "max:2",
          "in-range path returns index-two maximum without another setter");

    Runtime special;
    special.expected_object = 0x654u;
    special.words.clear();
    for (std::uint32_t index = 0u; index < 0x54u; ++index)
        special.words[0x658u + index * 4u] = 0xa000u + index;
    special.values = {{7u, 1u}, {9u, 1u}, {8u, 1u}};
    Check(pal4::PlayerManagerSlotScriptData::Refresh(
              0x654u, special) == 5u,
          "script object at address 0x654 passes a zero skill marker");
}

void SignedComparison() {
    Runtime runtime;
    runtime.values = {{7u, static_cast<std::uint32_t>(-1)},
                      {9u, static_cast<std::uint32_t>(-3)},
                      {8u, 0u}};
    runtime.max0 = {static_cast<std::uint32_t>(-2),
                    static_cast<std::uint32_t>(-4),
                    static_cast<std::uint32_t>(-5)};
    runtime.max2 = {static_cast<std::uint32_t>(-6)};
    Check(pal4::PlayerManagerSlotScriptData::Refresh(
              0x5000u, runtime) == 0xfeedu &&
              static_cast<std::int32_t>(runtime.values[7u]) == -4 &&
              static_cast<std::int32_t>(runtime.values[9u]) == -5,
          "negative values use signed comparisons against negative maxima");
}

}  // namespace

int main() {
    ClampAndCallOrder();
    InRangeAndSpecialMarker();
    SignedComparison();
    std::cout << "3 player-manager script-data groups passed\n";
}
