#include "pal4/combat_event_data_registry.h"
#include "pal4/combat_event_object_builder.h"

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

struct Runtime final : pal4::ICombatEventObjectBuildRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    double event_value = -2.5;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write " + std::to_string(address) + " " +
                        std::to_string(value));
        words[address] = value;
    }
    double ReadEventValue(std::uint32_t event,
                          std::uint32_t source_address) override {
        calls.push_back("value " + std::to_string(event) + " " +
                        std::to_string(source_address));
        return event_value;
    }
};

constexpr auto kBase = pal4::CombatEventDataRegistry::kBase;

void LookupBounds() {
    Runtime runtime;
    runtime.words[kBase] = 11u;
    runtime.words[kBase + 78u * 4u] = 88u;
    Check(pal4::CombatEventDataRegistry::Lookup(0u, runtime) == 11u &&
              pal4::CombatEventDataRegistry::Lookup(78u, runtime) == 88u &&
              pal4::CombatEventDataRegistry::Lookup(79u, runtime) == 0u &&
              pal4::CombatEventDataRegistry::Lookup(0xffffffffu, runtime) ==
                  0u &&
              runtime.calls == std::vector<std::string>{
                  "read 9414864", "read 9415176"},
          "registry reads only indices 0 through 78");
}

void DirectAndInvalidModes() {
    Runtime mode_zero;
    mode_zero.words[kBase + 4u * 4u] = 700u;
    const auto negative_zero = -0.0f;
    Check(pal4::CombatEventObjectBuilder::Build(
              2000u, 1000u, 4u, negative_zero, 0u, mode_zero) == 2000u &&
              mode_zero.calls == std::vector<std::string>{
                  "read 9414880", "write 2008 0", "write 2000 700",
                  "write 2004 2147483648"},
          "mode zero preserves raw scale bits and native write order");

    Runtime mode_two;
    mode_two.words[kBase + 4u * 4u] = 700u;
    Check(pal4::CombatEventObjectBuilder::Build(
              2000u, 1000u, 4u, 1.25f, 2u, mode_two) == 2000u &&
              mode_two.calls == std::vector<std::string>{
                  "read 9414880", "write 2000 700", "write 2004 1067450368",
                  "write 2008 1"},
          "mode two stores mode one after event and raw scale");

    Runtime missing;
    Check(pal4::CombatEventObjectBuilder::Build(
              2000u, 1000u, 4u, 1.0f, 0u, missing) == 2000u &&
              missing.calls == std::vector<std::string>{
                  "read 9414880", "write 2000 0", "write 2004 0",
                  "write 2008 2"},
          "missing lookup produces invalid mode-two record");

    Runtime invalid;
    invalid.words[kBase + 4u * 4u] = 700u;
    Check(pal4::CombatEventObjectBuilder::Build(
              2000u, 1000u, 4u, 1.0f, 5u, invalid) == 2000u &&
              invalid.calls.back() == "write 2008 2",
          "unknown mode produces invalid record");
}

void EventValuesAndActorFields() {
    Runtime mode_one;
    mode_one.words[kBase + 4u * 4u] = 700u;
    Check(pal4::CombatEventObjectBuilder::Build(
              2000u, 1000u, 4u, 2.0f, 1u, mode_one) == 2000u &&
              mode_one.calls == std::vector<std::string>{
                  "read 9414880", "value 700 1340", "write 2008 0",
                  "write 2000 700", "write 2004 3231711232"},
          "mode one reads event value using actor plus 0x154");

    Runtime mode_three;
    mode_three.words[kBase + 4u * 4u] = 700u;
    mode_three.event_value = 3.5;
    Check(pal4::CombatEventObjectBuilder::Build(
              2000u, 1000u, 4u, 0.5f, 3u, mode_three) == 2000u &&
              mode_three.calls[1] == "value 700 1004" &&
              mode_three.words[2004u] == std::bit_cast<std::uint32_t>(1.75f),
          "mode three reads event value using actor plus four");

    Runtime index_six;
    index_six.words[kBase + 6u * 4u] = 700u;
    index_six.words[1352u] = std::bit_cast<std::uint32_t>(-3);
    Check(pal4::CombatEventObjectBuilder::Build(
              2000u, 1000u, 6u, 0.5f, 4u, index_six) == 2000u &&
              index_six.calls == std::vector<std::string>{
                  "read 9414888", "read 1352", "write 2008 0",
                  "write 2000 700", "write 2004 3217031168"},
          "mode four index six multiplies signed actor field plus 0x160");

    Runtime index_two;
    index_two.words[kBase + 2u * 4u] = 700u;
    index_two.words[1344u] = 9u;
    Check(pal4::CombatEventObjectBuilder::Build(
              2000u, 1000u, 2u, 2.0f, 4u, index_two) == 2000u &&
              index_two.calls[1] == "read 1344" &&
              index_two.words[2004u] == std::bit_cast<std::uint32_t>(18.0f),
          "mode four index two uses actor field plus 0x158");

    Runtime other_index;
    other_index.words[kBase + 4u * 4u] = 700u;
    Check(pal4::CombatEventObjectBuilder::Build(
              2000u, 1000u, 4u, 1.0f, 4u, other_index) == 2000u &&
              other_index.calls == std::vector<std::string>{
                  "read 9414880", "write 2000 0", "write 2004 0",
                  "write 2008 2"},
          "mode four rejects indices other than two and six");
}

}  // namespace

int main() {
    try {
        LookupBounds();
        DirectAndInvalidModes();
        EventValuesAndActorFields();
        std::cout << "combat event object builder tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
