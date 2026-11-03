#include "pal4/actor_manager_combat_prepare.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kManager = 0x1000u;
constexpr std::uint32_t kCommand = 0x2000u;
constexpr std::uint32_t kCombat = 0x3000u;

struct Runtime final : pal4::IActorManagerCombatPrepareRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    std::vector<bool> inactive{true, true};
    std::size_t inactive_position = 0u;
    std::uint32_t random_index = 0u;
    std::uint32_t effect_count = 0u;
    std::uint32_t dispatch_result = 0x177u;
    bool effect_active = false;
    std::uint32_t null_text_literal = 0u;
    std::uint32_t fail_text_literal = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        trace.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    std::uint32_t CombatSystem() override {
        trace.push_back("combat-system");
        return kCombat;
    }
    bool CombatInactive(std::uint32_t combat) override {
        Check(combat == kCombat && inactive_position < inactive.size(),
              "inactive query owner/count");
        trace.push_back("inactive");
        return inactive[inactive_position++];
    }
    pal4::ActorManagerEffectHandle CreateEffect(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t position, std::uint32_t mode,
        std::uint32_t flags) override {
        Check(manager == kManager &&
                  position == pal4::ActorManagerCombatPrepare::kPosition &&
                  mode == 1u && flags == 0u,
              "effect creation constants");
        trace.push_back("effect:" + std::to_string(name));
        ++effect_count;
        return {0x5000u + effect_count, 0x6000u + effect_count};
    }
    void ResetTemporaryEffect(
        pal4::ActorManagerEffectHandle& handle) override {
        trace.push_back("effect-reset:" + std::to_string(handle.id));
        handle = {0xffffffffu, 0u};
    }
    std::uint32_t CreateText(std::uint32_t literal) override {
        trace.push_back("text:" + std::to_string(literal));
        if (literal == fail_text_literal)
            throw std::runtime_error("text construction failed");
        return literal == null_text_literal ? 0u : literal + 0x10000u;
    }
    void ReleaseText(std::uint32_t text) override {
        trace.push_back("text-release:" + std::to_string(text));
    }
    std::uint32_t RandomIndex(std::uint32_t count) override {
        Check(count == 3u && random_index < count,
              "random choice range");
        trace.push_back("random");
        return random_index;
    }
    bool EffectStillActive(std::uint32_t handle) override {
        Check(handle == kManager + 384u, "effect activity handle");
        trace.push_back("effect-active");
        return effect_active;
    }
    std::uint32_t EventRouter() override {
        trace.push_back("router");
        return 0x4000u;
    }
    std::uint32_t DispatchAddress(std::uint32_t router,
                                   std::uint32_t key,
                                   std::uint32_t payload,
                                   std::uint32_t mode) override {
        Check(router == 0x4000u && key == 0xa000u &&
                  payload == kCommand && mode == 1u,
              "live command payload dispatch");
        trace.push_back("dispatch-address");
        return dispatch_result;
    }
    std::uint32_t DispatchByte(std::uint32_t router,
                                std::uint32_t key,
                                std::uint8_t payload,
                                std::uint32_t mode) override {
        Check(router == 0x4000u && key == 0xa000u &&
                  payload == 0u && mode == 1u,
              "local byte payload dispatch");
        trace.push_back("dispatch-byte");
        return dispatch_result;
    }
};

void InactivePaths() {
    Runtime inactive;
    inactive.words[pal4::ActorManagerCombatPrepare::kCombatStartKey] =
        0xa000u;
    Check(pal4::ActorManagerCombatPrepare::Handle(
              kManager, kCommand, inactive) == 0x77u &&
              inactive.words[kCombat + 508u] == 2u &&
              inactive.effect_count == 0u &&
              inactive.trace.back() == "dispatch-address",
          "inactive combat dispatches live command without creating effect");

    Runtime became_inactive;
    became_inactive.inactive = {false, true};
    became_inactive.words[kCombat + 492u] = 1u;
    became_inactive.words[pal4::ActorManagerCombatPrepare::kCombatStartKey] =
        0xa000u;
    Check(pal4::ActorManagerCombatPrepare::Handle(
              kManager, kCommand, became_inactive) == 0x77u &&
              became_inactive.effect_count == 1u &&
              std::find(became_inactive.trace.begin(),
                        became_inactive.trace.end(),
                        "effect:8657252") <
                  std::find(became_inactive.trace.begin(),
                            became_inactive.trace.end(),
                            "dispatch-address") &&
              became_inactive.words[kManager + 384u] == 0u,
          "initial active preview uses fallback name before inactive transition");
}

void CommandEffects() {
    for (const auto command : {1u, 2u}) {
        Runtime runtime;
        runtime.inactive = {false, false};
        runtime.words[kCombat + 492u] = 1u;
        runtime.words[kCombat + 488u] = 0x7000u;
        runtime.words[kCommand] = command;
        runtime.effect_active = true;
        Check(pal4::ActorManagerCombatPrepare::Handle(
                  kManager, kCommand, runtime) == 1u &&
                  runtime.effect_count == 2u &&
                  runtime.words[kManager + 384u] == 0x5002u &&
                  runtime.words[kManager + 388u] == 0x6002u &&
                  std::find(runtime.trace.begin(), runtime.trace.end(),
                            "effect:" + std::to_string(
                                command == 1u ? 0x8b6770u : 0x8b676cu)) !=
                      runtime.trace.end() &&
                  std::find(runtime.trace.begin(), runtime.trace.end(),
                            "dispatch-byte") == runtime.trace.end(),
              "commands one and two store fixed effect handles");
    }

    Runtime named;
    named.inactive = {false, false};
    named.words[kCombat + 492u] = 1u;
    named.words[kCombat + 488u] = 0x7000u;
    named.words[kCombat + 476u] = 1u;
    named.words[kCombat + 472u] = 0x7100u;
    named.words[kCommand] = 3u;
    named.words[pal4::ActorManagerCombatPrepare::kCombatStartKey] =
        0xa000u;
    Check(pal4::ActorManagerCombatPrepare::Handle(
              kManager, kCommand, named) == 0x77u &&
              named.words[kManager + 384u] == 0x5002u &&
              named.words[kCombat + 508u] == 2u &&
              std::find(named.trace.begin(), named.trace.end(),
                        "effect:28928") != named.trace.end() &&
              named.trace.back() == "dispatch-byte",
          "other command uses configured effect and starts combat if inactive");
}

void RandomFallbacks() {
    Runtime runtime;
    runtime.inactive = {false, false};
    runtime.words[kCommand] = 3u;
    runtime.random_index = 2u;
    runtime.null_text_literal = 0x8b6760u;
    runtime.words[pal4::ActorManagerCombatPrepare::kCombatStartKey] =
        0xa000u;
    Check(pal4::ActorManagerCombatPrepare::Handle(
              kManager, kCommand, runtime) == 0x77u &&
              runtime.effect_count == 2u &&
              runtime.words[kManager + 384u] == 0x5002u &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "effect:8657252") != runtime.trace.end() &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "text-release:" + std::to_string(
                            0x8b6768u + 0x10000u)) >
                  std::find(runtime.trace.begin(), runtime.trace.end(),
                            "text-release:" + std::to_string(
                                0x8b6764u + 0x10000u)),
          "both random branches use three choices and release text in reverse");

    Runtime throwing;
    throwing.inactive = {false, false};
    throwing.fail_text_literal = 0x8b677cu;
    bool caught = false;
    try {
        static_cast<void>(pal4::ActorManagerCombatPrepare::Handle(
            kManager, kCommand, throwing));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught &&
              std::find(throwing.trace.begin(), throwing.trace.end(),
                        "text-release:" + std::to_string(
                            0x8b6784u + 0x10000u)) !=
                  throwing.trace.end(),
          "already-created random text is released when later creation throws");
}

void TypeOneEffect() {
    Runtime inactive;
    Check(pal4::ActorManagerCombatPrepare::HandleTypeOne(
              kManager, kCommand, inactive) == 1u &&
              inactive.trace == std::vector<std::string>{
                  "combat-system", "inactive"},
          "type-one event stops before reading input when combat inactive");

    Runtime other;
    other.inactive = {false};
    other.words[kCommand] = 2u;
    Check(pal4::ActorManagerCombatPrepare::HandleTypeOne(
              kManager, kCommand, other) == kCommand &&
              other.effect_count == 0u,
          "type-one event returns input address for other commands");

    Runtime selected;
    selected.inactive = {false};
    selected.words[kCommand] = 1u;
    Check(pal4::ActorManagerCombatPrepare::HandleTypeOne(
              kManager, kCommand, selected) == 0u &&
              selected.effect_count == 1u &&
              selected.words[kManager + 384u] == 0u &&
              selected.trace[selected.trace.size() - 2u] ==
                  "effect:9136008" &&
              selected.trace.back() == "effect-reset:20481",
          "type-one event creates CA47 effect without storing handle");
}

}  // namespace

int main() {
    try {
        InactivePaths();
        CommandEffects();
        RandomFallbacks();
        TypeOneEffect();
        std::cout << "actor_manager_combat_prepare_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
