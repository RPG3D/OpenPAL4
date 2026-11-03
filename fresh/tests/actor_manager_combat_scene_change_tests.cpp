#include "pal4/actor_manager_combat_scene_change.h"

#include <algorithm>
#include <array>
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
constexpr std::uint32_t child = 0x2000u;
constexpr std::uint32_t record = 0x3000u;

struct Runtime final : pal4::IActorManagerCombatSceneChangeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    std::uint32_t database_result = record;
    std::uint32_t child_after_copy = child;
    std::uint32_t model = 2592u;
    std::uint32_t next_effect = 1u;
    std::map<std::uint32_t, pal4::ActorEffectHandle> effects;

    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == actor + 1164u && value == 0u,
              "pending record clears after join event");
        trace.emplace_back("clear-key");
        words[address] = value;
    }
    std::uint32_t ActorSlot20(std::uint32_t address) override {
        Check(address == actor, "slot 20 actor");
        trace.emplace_back("slot20");
        return 20u;
    }
    std::uint32_t ActorSlot52(std::uint32_t address,
                              std::uint32_t index) override {
        Check(address == actor &&
                  (index == 7u || index == 9u || index == 8u),
              "slot 52 index");
        trace.emplace_back("slot52:" + std::to_string(index));
        return index * 10u;
    }
    std::uint32_t FindDatabaseRecord(std::uint32_t key) override {
        Check(key == 0x7788u, "pending database key");
        trace.emplace_back("database");
        return database_result;
    }
    void DispatchEvent(std::string_view event,
                       std::uint32_t address,
                       std::uint32_t mode) override {
        Check(address == actor && mode == 1u &&
                  (event == "me_leave_scene" || event == "me_join_scene"),
              "scene event payload");
        trace.emplace_back(std::string(event));
    }
    void ActorSlotNoArgument(std::uint32_t address,
                             std::uint32_t slot) override {
        Check(address == actor &&
                  (slot == 588u || slot == 584u || slot == 276u ||
                   slot == 580u), "actor no-argument slot");
        trace.emplace_back("slot:" + std::to_string(slot));
    }
    void CopyFrom(std::uint32_t address, std::uint32_t source) override {
        Check(address == actor && source == record,
              "copy actor and database source");
        trace.emplace_back("copy");
        words[actor + 808u] = child_after_copy;
    }
    void ActorSlot24(std::uint32_t address,
                     std::uint32_t value) override {
        Check(address == actor && value == 20u, "restore actor state");
        trace.emplace_back("slot24");
    }
    void ActorSlot68(std::uint32_t address, std::uint32_t index,
                     std::uint32_t value) override {
        Check(address == actor && value == index * 10u,
              "restore captured property");
        trace.emplace_back("slot68:" + std::to_string(index));
    }
    void ResetEffect(std::uint32_t address,
                     std::uint32_t handle) override {
        Check(address == actor &&
                  (handle == actor + 868u || handle == actor + 876u ||
                   handle == actor + 884u), "effect reset slot");
        trace.emplace_back("reset:" + std::to_string(handle - actor));
    }
    pal4::ActorEffectHandle LoadBottomEffect(
        std::uint32_t address, std::uint32_t frame,
        std::string_view hook) override {
        Check(address == actor && frame == child + 496u &&
                  hook == "hook_ef_bottom", "bottom effect arguments");
        trace.emplace_back("bottom");
        return {next_effect++, 0x9000u};
    }
    pal4::ActorEffectHandle LoadNamedEffect(
        std::uint32_t address, std::string_view name,
        std::string_view hook) override {
        Check(address == actor, "named effect actor");
        trace.emplace_back("load:" + std::string(name) + ":" +
                           std::string(hook));
        return {next_effect++, 0x9000u};
    }
    void StoreEffect(std::uint32_t handle,
                     pal4::ActorEffectHandle effect) override {
        Check(effect.owner == 0x9000u && effect.id != 0u,
              "stored effect handle");
        effects[handle - actor] = effect;
        trace.emplace_back("store:" + std::to_string(handle - actor));
    }
    void ResetTemporary(pal4::ActorEffectHandle effect) override {
        Check(effect.owner == 0x9000u && effect.id != 0u,
              "temporary effect handle");
        trace.emplace_back("release-temp");
    }
    std::uint32_t ModelId(std::uint32_t address) override {
        Check(address == actor, "model actor");
        trace.emplace_back("model");
        return model;
    }
};

const std::vector<std::string> prefix{
    "slot20", "slot52:7", "slot52:9", "slot52:8", "read:5260",
    "database"};

const std::vector<std::string> scene_change{
    "me_leave_scene", "slot:588", "copy", "slot:584", "slot:276",
    "slot24", "slot68:7", "slot68:9", "slot68:8", "slot:580",
    "me_join_scene", "clear-key", "reset:868", "reset:876", "reset:884",
    "read:4904"};

std::vector<std::string> ExpectedScene() {
    auto expected = prefix;
    expected.insert(expected.end(), scene_change.begin(), scene_change.end());
    return expected;
}

void GatesAndOrder() {
    Runtime absent;
    absent.words[actor + 1164u] = 0x7788u;
    absent.database_result = 0u;
    pal4::ActorManagerCombatSceneChange::ApplyPendingRecord(actor, absent);
    Check(absent.trace == prefix &&
              absent.words[actor + 1164u] == 0x7788u,
          "database miss preserves pending key and skips scene change");

    Runtime no_child;
    no_child.words[actor + 1164u] = 0x7788u;
    no_child.child_after_copy = 0u;
    pal4::ActorManagerCombatSceneChange::ApplyPendingRecord(actor, no_child);
    Check(no_child.trace == ExpectedScene() && no_child.effects.empty() &&
              no_child.words[actor + 1164u] == 0u,
          "scene changes and resets effects before late child gate");
}

void EffectModels() {
    for (const auto model : std::array{2592u, 2598u, 2599u,
                                      2603u, 2604u, 2605u, 2606u,
                                      2593u}) {
        Runtime runtime;
        runtime.words[actor + 1164u] = 0x7788u;
        runtime.model = model;
        pal4::ActorManagerCombatSceneChange::ApplyPendingRecord(actor, runtime);
        auto expected = ExpectedScene();
        expected.insert(expected.end(), {
            "bottom", "store:868", "release-temp", "model"});
        std::string resource;
        switch (model) {
            case 2592u: resource = "EFF20"; break;
            case 2603u: resource = "EFF21"; break;
            case 2604u: resource = "EFF22"; break;
            case 2605u: resource = "EFF23"; break;
            case 2606u: resource = "EFF24"; break;
            case 2598u: resource = "H_B20"; break;
            case 2599u: resource = "H_B21"; break;
            default: break;
        }
        if (!resource.empty()) {
            const auto hook = model == 2598u || model == 2599u
                                  ? "hook_ef_bottom" : "hook_ef_lhand";
            expected.insert(expected.end(), {
                "load:" + resource + ":" + hook,
                "store:876", "release-temp"});
            if (model != 2598u && model != 2599u)
                expected.insert(expected.end(), {
                    "load:" + resource + ":hook_ef_rhand",
                    "store:884", "release-temp"});
        }
        Check(runtime.trace == expected &&
                  runtime.effects.size() ==
                      (resource.empty() ? 1u :
                       (model == 2598u || model == 2599u ? 2u : 3u)),
              "model-specific effect names, slots and order");
    }
}

}  // namespace

int main() {
    try {
        GatesAndOrder();
        EffectModels();
        std::cout << "actor_manager_combat_scene_change_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
