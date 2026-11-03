#include "pal4/actor_manager_player_stage_lifecycle.h"

#include <algorithm>
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

struct Runtime final : pal4::IActorManagerPlayerStageRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    bool add_on_first_scene = false;
    bool replace_current_on_first_scene = false;
    std::uint32_t scene_events = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        trace.emplace_back("write:" + std::to_string(address) +
                           ":" + std::to_string(value));
        words[address] = value;
    }
    void PrepareStage(std::uint32_t actual_actor) override {
        Check(actual_actor == actor, "stage preparation actor");
        trace.emplace_back("prepare");
    }
    std::uint32_t ResolveAnimationTransform(std::uint32_t actual_actor,
                                            std::uint32_t mode,
                                            std::uint32_t context) override {
        Check(actual_actor == actor && mode == 0u && context == 0xabcdu,
              "stage transform inputs");
        trace.emplace_back("resolve");
        return 0x9000u;
    }
    void SetAnimationTransform(std::uint32_t animation,
                               std::uint32_t transform) override {
        Check(animation == 0x6000u && transform == 0x9000u,
              "stage animation transform");
        trace.emplace_back("set_transform");
    }
    std::uint32_t SendEvent(std::string_view name,
                            std::uint32_t actual_actor) override {
        Check(actual_actor == actor, "stage event actor payload");
        trace.emplace_back(std::string(name));
        if (name == "re_leave_scene") {
            ++scene_events;
            Check(words[actor + 808u] != 0u,
                  "leave scene observes current animation");
            if (add_on_first_scene && scene_events == 1u) {
                words[0x5004u] = 0x6100u;
                words[actor + 1300u] = 0x5008u;
            }
            if (replace_current_on_first_scene && scene_events == 1u)
                words[actor + 808u] = 0x6200u;
        }
        return name == "re_join_scene" ? 0x72u : 0x41u;
    }
    std::uint32_t SingletonManager() override {
        trace.emplace_back("singleton");
        return 0x9000u;
    }
    void ReturnAnimationToPool(std::uint32_t collection,
                               std::uint32_t limit,
                               std::uint32_t animation) override {
        Check(collection == 0x9040u && limit == 12u,
              "returned animation uses singleton pool and capacity 12");
        trace.emplace_back("return:" + std::to_string(animation));
    }
};

std::size_t Position(const std::vector<std::string>& trace,
                     const std::string& event) {
    const auto it = std::find(trace.begin(), trace.end(), event);
    Check(it != trace.end(), "stage operation missing");
    return static_cast<std::size_t>(it - trace.begin());
}

void EnterWithAnimation() {
    Runtime runtime;
    runtime.words[actor + 808u] = 0x6000u;
    Check(pal4::ActorManagerPlayerStageLifecycle::Enter(
              actor, 0xabcdu, runtime) == 0x72u &&
              Position(runtime.trace, "prepare") <
                  Position(runtime.trace, "resolve") &&
              Position(runtime.trace, "resolve") <
                  Position(runtime.trace, "set_transform") &&
              Position(runtime.trace, "set_transform") <
                  Position(runtime.trace, "re_join_stage") &&
              Position(runtime.trace, "re_join_stage") <
                  Position(runtime.trace, "re_join_scene"),
          "enter prepares, positions and dispatches both stage events");
}

void EnterWithoutAnimation() {
    Runtime runtime;
    Check(pal4::ActorManagerPlayerStageLifecycle::Enter(
              actor, 0xabcdu, runtime) == 0u &&
              Position(runtime.trace, "prepare") <
                  Position(runtime.trace, "re_join_stage") &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "resolve") == runtime.trace.end() &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "re_join_scene") == runtime.trace.end(),
          "enter without animation sends stage event only");
}

void LeaveEmptyAndPopulated() {
    Runtime empty;
    empty.words[actor + 808u] = 0x6000u;
    empty.words[actor + 1296u] = 0x5000u;
    empty.words[actor + 1300u] = 0x5000u;
    Check(pal4::ActorManagerPlayerStageLifecycle::Leave(actor, empty) ==
              0x5000u &&
              empty.words.at(actor + 808u) == 0u &&
              empty.scene_events == 0u &&
              empty.trace.front() == "re_leave_stage",
          "empty cache still leaves stage and clears current animation");

    Runtime populated;
    populated.words[actor + 1296u] = 0x5000u;
    populated.words[actor + 1300u] = 0x5008u;
    populated.words[0x5000u] = 0x6000u;
    populated.words[0x5004u] = 0x6100u;
    Check(pal4::ActorManagerPlayerStageLifecycle::Leave(actor, populated) ==
              0x5008u &&
              populated.scene_events == 2u &&
              populated.words.at(actor + 808u) == 0u &&
              Position(populated.trace, "write:4904:24576") <
                  Position(populated.trace, "re_leave_scene") &&
              populated.trace.back() == "write:4904:0",
          "leave visits every cached animation and clears current afterward");
}

void LeaveRechecksEnd() {
    Runtime runtime;
    runtime.add_on_first_scene = true;
    runtime.words[actor + 1296u] = 0x5000u;
    runtime.words[actor + 1300u] = 0x5004u;
    runtime.words[0x5000u] = 0x6000u;
    Check(pal4::ActorManagerPlayerStageLifecycle::Leave(actor, runtime) ==
              0x5008u &&
              runtime.scene_events == 2u &&
              runtime.words.at(actor + 808u) == 0u,
          "leave rereads cache end after each event callback");
}

void LeaveAndReturnEmpty() {
    Runtime runtime;
    runtime.words[actor + 808u] = 0x6000u;
    runtime.words[actor + 1296u] = 0x5000u;
    runtime.words[actor + 1300u] = 0x5000u;
    Check(pal4::ActorManagerPlayerStageLifecycle::LeaveAndReturnAnimations(
              actor, runtime) == 0x5000u &&
              runtime.words.at(actor + 808u) == 0u &&
              runtime.scene_events == 0u &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "singleton") == runtime.trace.end() &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "re_leave_stage") == runtime.trace.end(),
          "empty cache clears current without stage event or pool call");
}

void LeaveAndReturnCallbackChanges() {
    Runtime runtime;
    runtime.words[actor + 1296u] = 0x5000u;
    runtime.words[actor + 1300u] = 0x5004u;
    runtime.words[0x5000u] = 0x6000u;
    runtime.add_on_first_scene = true;
    runtime.replace_current_on_first_scene = true;
    Check(pal4::ActorManagerPlayerStageLifecycle::LeaveAndReturnAnimations(
              actor, runtime) == 0x5008u &&
              runtime.scene_events == 2u &&
              runtime.words.at(actor + 808u) == 0u &&
              Position(runtime.trace, "re_leave_scene") <
                  Position(runtime.trace, "singleton") &&
              Position(runtime.trace, "singleton") <
                  Position(runtime.trace, "return:25088") &&
              Position(runtime.trace, "return:25088") <
                  Position(runtime.trace, "return:24832") &&
              runtime.trace.back() == "write:4904:0",
          "scene callback changes current pointer before pool return and extends loop");
}

}  // namespace

int main() {
    try {
        EnterWithAnimation();
        EnterWithoutAnimation();
        LeaveEmptyAndPopulated();
        LeaveRechecksEnd();
        LeaveAndReturnEmpty();
        LeaveAndReturnCallbackChanges();
        std::cout << "actor_manager_player_stage_lifecycle_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
