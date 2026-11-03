#include "pal4/actor_manager_combat_animation_lookup.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t actor = 0x1000u;
constexpr std::uint32_t source = 0x2000u;

struct Runtime final : pal4::IActorManagerCombatAnimationLookupRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> names;
    std::vector<std::string> trace;
    bool found = false;
    std::uint32_t last_child = 0xffffffffu;
    std::string last_name;

    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("word:" + std::to_string(address));
        return words[address];
    }
    std::string_view ReadText(std::uint32_t record) override {
        trace.emplace_back("text:" + std::to_string(record));
        return names[record];
    }
    bool FindAnimation(std::uint32_t child,
                       std::string_view name) override {
        trace.emplace_back("find");
        last_child = child;
        last_name = name;
        return found;
    }
};

void Lookup() {
    using pal4::ActorManagerCombatAnimationLookup;
    Runtime primary;
    primary.words[actor + 808u] = 0x3000u;
    primary.names[source + 88u] = "Attack";
    primary.found = true;
    Check(ActorManagerCombatAnimationLookup::FromPrimaryName(
              actor, source, primary) == source + 88u &&
              primary.last_child == 0x3000u &&
              primary.last_name == "Attack.anm" &&
              primary.trace == std::vector<std::string>{
                  "text:8280", "word:4904", "find"},
          "primary name copies, appends and returns source text record");

    Runtime secondary;
    secondary.names[source + 144u] = "Guard";
    Check(ActorManagerCombatAnimationLookup::FromSecondaryName(
              actor, source, secondary) ==
              ActorManagerCombatAnimationLookup::kMissingAnimation &&
              secondary.last_child == 0u &&
              secondary.last_name == "Guard.anm" &&
              secondary.trace == std::vector<std::string>{
                  "text:8336", "word:4904", "find"},
          "secondary lookup forwards child even when null and falls back");

    Runtime empty;
    empty.found = true;
    Check(ActorManagerCombatAnimationLookup::FromSecondaryName(
              actor, source, empty) == source + 144u &&
              empty.last_name == ".anm",
          "empty source still appends animation extension");
}

}  // namespace

int main() {
    try {
        Lookup();
        std::cout << "actor_manager_combat_animation_lookup_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
