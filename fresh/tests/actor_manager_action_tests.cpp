#include "pal4/actor_manager_action.h"

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
constexpr std::uint32_t child = 0x5000u;

struct Runtime final : pal4::IActorManagerActionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    std::vector<bool> search_results{false, true};
    std::uint32_t search_count = 0;
    bool replace_child = false;
    std::uint32_t expected_child = child;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    bool FindAnimation(std::uint32_t target,
                       std::string_view name) override {
        Check(target == expected_child && name == "run.anm",
              "animation lookup uses child and .anm suffix");
        trace.push_back("find");
        return search_results[search_count++];
    }
    void LoadMissingAnimation(std::uint32_t target) override {
        Check(target == actor, "load missing animation actor");
        trace.push_back("load");
        if(replace_child)words[actor+808u]=expected_child=0x7000u;
    }
    void SetAttachmentActive(std::uint32_t target, bool right_hand,
                             std::uint8_t active,
                             std::uint32_t effect_context) override {
        Check(target == expected_child && active == 1u &&
              effect_context == actor, "attachment activation context");
        trace.push_back(right_hand ? "right-active" : "left-active");
    }
    void SetAnimationFlag(std::uint32_t state,
                          std::uint8_t flag) override {
        Check(state == expected_child + 368u && flag == 0x80u,
              "animation state flag byte");
        trace.push_back("flag");
    }
    void SetAnimationName(std::uint32_t state,
                          std::string_view name) override {
        Check(state == expected_child + 368u && name == "run",
              "animation state receives original name");
        trace.push_back("name");
    }
    void TouchSkillSystem() override { trace.push_back("skills"); }
    void InvokeChildSlot4(std::uint32_t target, std::uint32_t first,
                          std::uint32_t second) override {
        Check(target == child && first == 7u && second == 8u,
              "rotation child slot arguments");
        trace.push_back("slot4");
    }
    std::uint32_t InvokeChildSlot8(std::uint32_t target,
                                   std::uint32_t first,
                                   std::uint32_t second) override {
        Check(target == child && first == 7u && second == 8u,
              "scale child slot arguments");
        trace.push_back("slot8");
        return 0x77u;
    }
    std::uint32_t GetAnimationFlag(std::uint32_t state) override {
        Check(state == child + 368u, "animation flag state");
        trace.push_back("get-flag");
        return 0x55u;
    }
    std::string_view GetAnimationName(std::uint32_t state) override {
        Check(state == child + 368u, "animation name state");
        trace.push_back("get-name");
        return "run";
    }
};

void PlayPaths() {
    Runtime runtime;
    pal4::ActorManagerAction::Play(actor, "run", 0x80u, runtime);
    Check(runtime.trace.empty(), "missing child skips all animation work");
    runtime.words[actor + 808u] = child;
    pal4::ActorManagerAction::Play(actor, "run", 0x80u, runtime);
    Check(runtime.trace == std::vector<std::string>{
              "find", "load", "find", "left-active", "right-active",
              "flag", "name", "skills"},
          "missing animation loads then rechecks and activates both hands");
    runtime.trace.clear();
    runtime.search_count = 0;
    runtime.search_results = {true, true};
    pal4::ActorManagerAction::Play(actor, "run", 0x80u, runtime);
    Check(runtime.trace == std::vector<std::string>{
              "find", "find", "left-active", "right-active",
              "flag", "name", "skills"},
          "existing animation still receives second search without reload");
    runtime.trace.clear();
    runtime.search_count = 0;
    runtime.search_results = {false, false};
    pal4::ActorManagerAction::Play(actor, "run", 0x80u, runtime);
    Check(runtime.trace == std::vector<std::string>{
              "find", "load", "find", "flag", "name", "skills"},
          "failed second search skips attachment activation");
    runtime.trace.clear();runtime.search_count=0;
    runtime.search_results={false,true};runtime.replace_child=true;
    pal4::ActorManagerAction::Play(actor,"run",0x80u,runtime);
    Check(runtime.words[actor+808u]==0x7000u && runtime.trace==std::vector<std::string>{
        "find","load","find","left-active","right-active","flag","name","skills"},
        "Player slot568 replacement receives second lookup, hands and animation writes");
}

void ChildAccessors() {
    Runtime runtime;
    Check(pal4::ActorManagerAction::Position(actor, runtime) == 0x8b5c58u &&
          pal4::ActorManagerAction::Matrix(actor, runtime) == 0x8b5c58u &&
          pal4::ActorManagerAction::BoundingBox(actor, runtime) ==
              0x8f60b0u &&
          pal4::ActorManagerAction::Clump(actor, runtime) == 0u &&
          pal4::ActorManagerAction::AnimationFlag(actor, runtime) == 1u &&
          pal4::ActorManagerAction::AnimationName(actor, runtime).empty() &&
          !pal4::ActorManagerAction::InvokeRotation(
              actor, 7u, 8u, runtime) &&
          pal4::ActorManagerAction::InvokeScale(
              actor, 7u, 8u, runtime) == 0u,
          "missing child uses IDA static fallbacks and no virtual calls");
    runtime.words[actor + 808u] = child;
    runtime.words[child + 20u] = 0x6000u;
    Check(pal4::ActorManagerAction::Position(actor, runtime) ==
              child + 172u &&
          pal4::ActorManagerAction::Matrix(actor, runtime) ==
              child + 160u &&
          pal4::ActorManagerAction::BoundingBox(actor, runtime) ==
              child + 524u &&
          pal4::ActorManagerAction::Clump(actor, runtime) == 0x6000u &&
          pal4::ActorManagerAction::AnimationFlag(actor, runtime) == 0x55u &&
          pal4::ActorManagerAction::AnimationName(actor, runtime) == "run" &&
          pal4::ActorManagerAction::InvokeRotation(
              actor, 7u, 8u, runtime) &&
          pal4::ActorManagerAction::InvokeScale(
              actor, 7u, 8u, runtime) == 0x77u &&
          runtime.trace == std::vector<std::string>{
              "get-flag", "get-name", "slot4", "slot8"},
          "present child supplies offsets, animation data and virtual results");
}

}  // namespace

int main() {
    try {
        PlayPaths();
        ChildAccessors();
        std::cout << "actor_manager_action_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
