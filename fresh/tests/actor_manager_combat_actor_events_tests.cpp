#include "pal4/actor_manager_combat_actor_events.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t actor = 0x1000u;

struct Runtime final : pal4::IActorManagerCombatEventRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    bool set_child_during_name = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void InvokeActorSlot592(std::uint32_t address,
                            std::uint32_t text) override {
        Check(address == actor, "name virtual receives actor");
        trace.emplace_back("slot592:" + std::to_string(text));
        if (set_child_during_name) words[actor + 808u] = 0x6000u;
    }
    std::uint32_t InvokeActorSlot444(std::uint32_t address,
                                     std::uint32_t argument) override {
        Check(address == actor && argument == 0u,
              "actor slot 444 receives zero");
        trace.emplace_back("slot444");
        return 0x777u;
    }
    std::uint32_t InvokeChildSlot4(std::uint32_t child,
                                   std::uint32_t argument) override {
        Check(child == 0x6000u && argument == 0x777u,
              "child slot 4 receives actor slot 444 result");
        trace.emplace_back("child4");
        return 0x1234u;
    }
    void DispatchPair(std::string_view event, std::uint32_t first,
                      std::uint32_t second, std::uint32_t mode) override {
        Check(event == "summon" && first == 0x2000u &&
                  second == 0x3000u && mode == 1u,
              "summon payload and mode");
        trace.emplace_back("summon");
    }
    std::uint32_t InvokeActorSlot48(std::uint32_t address,
                                    std::uint32_t index) override {
        Check(address == actor && (index == 0u || index == 2u),
              "actor slot 48 index");
        trace.emplace_back("slot48:" + std::to_string(index));
        return index == 0u ? 70u : 90u;
    }
    void InvokeActorSlot68(std::uint32_t address,
                           std::uint32_t index,
                           std::uint32_t value) override {
        Check(address == actor &&
                  ((index == 7u && value == 70u) ||
                   (index == 9u && value == 90u)),
              "actor slot 68 receives matching data value");
        trace.emplace_back("slot68:" + std::to_string(index));
    }
    void InvokeActorSlot580(std::uint32_t address) override {
        Check(address == actor, "actor slot 580 address");
        trace.emplace_back("slot580");
    }
    std::uint32_t DispatchActor(std::string_view event,
                                std::uint32_t address,
                                std::uint32_t mode) override {
        Check(address == actor && mode == 1u,
              "stage and scene event payload");
        trace.emplace_back(std::string(event));
        if (event == "me_join_stage" || event == "me_leave_stage")
            return 11u;
        return 22u;
    }
};

void ResourceRefresh() {
    Runtime missing;
    Check(pal4::ActorManagerCombatActorEvents::RefreshResource(
              actor, missing) == 0u &&
              missing.trace == std::vector<std::string>{
                  "read:5100", "slot592:8657252", "read:4904"},
          "absent name and child use shared empty text then stop");

    Runtime present;
    present.words[actor + 1004u] = 0x7000u;
    present.words[actor + 808u] = 0x6000u;
    Check(pal4::ActorManagerCombatActorEvents::RefreshResource(
              actor, present) == 0x1234u &&
              present.trace == std::vector<std::string>{
                  "read:5100", "slot592:28672", "read:4904",
                  "slot444", "child4"},
          "present child receives slot 444 value and returns slot 4 result");

    Runtime mutated;
    mutated.set_child_during_name = true;
    Check(pal4::ActorManagerCombatActorEvents::RefreshResource(
              actor, mutated) == 0x1234u,
          "child pointer is read after name virtual call");
}

void Summon() {
    for (const auto pair : {
             std::pair<std::uint32_t, std::uint32_t>{0u, 0x3000u},
             {0x2000u, 0u}, {0u, 0u}}) {
        Runtime runtime;
        pal4::ActorManagerCombatActorEvents::Summon(
            pair.first, pair.second, runtime);
        Check(runtime.trace.empty(), "summon needs both arguments");
    }
    Runtime runtime;
    pal4::ActorManagerCombatActorEvents::Summon(
        0x2000u, 0x3000u, runtime);
    Check(runtime.trace == std::vector<std::string>{"summon"},
          "summon dispatches pair once");
}

void StageAndScene() {
    Runtime joined;
    Check(pal4::ActorManagerCombatActorEvents::JoinStageAndScene(
              actor, 0xdeadbeefu, joined) == 22u &&
              joined.trace == std::vector<std::string>{
                  "slot48:0", "slot68:7", "slot48:2", "slot68:9",
                  "slot580", "me_join_stage", "me_join_scene"},
          "join refreshes two properties before ordered events");

    Runtime left;
    Check(pal4::ActorManagerCombatActorEvents::LeaveStageAndScene(
              actor, left) == 22u &&
              left.trace == std::vector<std::string>{
                  "me_leave_stage", "me_leave_scene"},
          "leave dispatches stage then scene and returns second result");
}

}  // namespace

int main() {
    try {
        ResourceRefresh();
        Summon();
        StageAndScene();
        std::cout << "actor_manager_combat_actor_events_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
