#include "pal4/actor_manager_player_action_playback.h"

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
constexpr std::uint32_t name = 0x4000u;

struct Runtime final : pal4::IActorManagerPlayerActionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> strings;
    std::map<std::uint32_t, std::string> animation_names;
    std::vector<std::string> trace;
    std::uint32_t loaded = 0x7000u;
    bool has_action = false;

    Runtime() {
        words[actor + 808u] = 0x6000u;
        words[actor + 1148u] = name;
        words[actor + 1152u] = 4u;
        strings[name] = "hero";
        animation_names[0x6000u] = "hero";
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == actor + 808u && value == loaded,
              "current animation replacement");
        trace.emplace_back("write_current");
        words[address] = value;
    }
    std::string_view ReadCString(std::uint32_t address) override {
        trace.emplace_back("requested");
        return strings.at(address);
    }
    std::string_view AnimationName(std::uint32_t animation) override {
        trace.emplace_back("active_name");
        return animation_names.at(animation);
    }
    bool HasActionAnimation(std::uint32_t animation,
                            std::string_view filename) override {
        Check(animation == 0x6000u && filename == "run.anm",
              "action file name and current model");
        trace.emplace_back("has_action");
        return has_action;
    }
    void PlayAction(std::uint32_t actual_actor, std::string_view action,
                    std::uint8_t flags) override {
        Check(actual_actor == actor && action == "run" && flags == 0xa5u,
              "action name and low-byte flags forwarded");
        trace.emplace_back("play");
    }
    void TouchSkillSystem() override { trace.emplace_back("skill"); }
    void TouchGlobalSystem() override { trace.emplace_back("global"); }
    std::uint32_t FindOrLoad(std::uint32_t actual_actor,
                             std::string_view animation_name) override {
        Check(actual_actor == actor && animation_name == "herorun",
              "base model and action name are concatenated");
        trace.emplace_back("load");
        return loaded;
    }
    void GetFrame(std::uint32_t animation) override {
        Check(animation == loaded, "frame receiver");
        trace.emplace_back("frame");
    }
    void CleanupAnimation(std::uint32_t animation,
                          std::uint32_t actual_actor) override {
        Check(animation == loaded && actual_actor == actor,
              "animation cleanup arguments");
        trace.emplace_back("cleanup");
    }
    void SetTransformFromActor(std::uint32_t animation,
                               std::uint32_t transform,
                               std::uint32_t flags) override {
        Check(animation == loaded && transform == 0x60acu && flags == 0u,
              "actor transform transfer");
        trace.emplace_back("transform_actor");
    }
    void SetTransformFromPosition(std::uint32_t animation,
                                  std::uint32_t position,
                                  std::uint32_t flags) override {
        Check(animation == loaded && position == 0x60a0u && flags == 0u,
              "position transfer");
        trace.emplace_back("transform_position");
    }
    void SetLeftHandActive(std::uint32_t animation, bool active) override {
        trace.emplace_back("left:" + std::to_string(animation) +
                           ":" + std::to_string(active));
    }
    void SetRightHandActive(std::uint32_t animation, bool active) override {
        trace.emplace_back("right:" + std::to_string(animation) +
                           ":" + std::to_string(active));
    }
    void UpdateAttachments(std::uint32_t actual_actor,
                           std::uint32_t animation) override {
        Check(actual_actor == actor && animation == loaded &&
                  words[actor + 808u] == loaded,
              "attachments observe new current animation");
        trace.emplace_back("attachments");
    }
    void DispatchRejoinScene(std::uint32_t actual_actor) override {
        Check(actual_actor == actor, "scene event receiver");
        trace.emplace_back("re_join_scene");
    }
};

std::size_t Position(const std::vector<std::string>& trace,
                     const std::string& event) {
    const auto it = std::find(trace.begin(), trace.end(), event);
    Check(it != trace.end(), "action playback operation missing");
    return static_cast<std::size_t>(it - trace.begin());
}

void NoCurrentAndDirectAction() {
    Runtime absent;
    absent.words[actor + 808u] = 0u;
    Check(pal4::ActorManagerPlayerActionPlayback::Play(
              actor, "run", 0xa5u, absent) == 0u &&
              absent.trace == std::vector<std::string>{"read:4904"},
          "null current model skips action and all services");

    Runtime direct;
    direct.has_action = true;
    Check(pal4::ActorManagerPlayerActionPlayback::Play(
              actor, "run", 0xa5u, direct) == 0u &&
              direct.trace == std::vector<std::string>{
                  "read:4904", "has_action", "play"},
          "existing action animation plays without model lookup");
}

void MismatchAndMissingReplacement() {
    Runtime mismatch;
    mismatch.animation_names[0x6000u] = "other";
    Check(pal4::ActorManagerPlayerActionPlayback::Play(
              actor, "run", 0xa5u, mismatch) == 0u &&
              std::count(mismatch.trace.begin(), mismatch.trace.end(),
                         "skill") == 1 &&
              std::find(mismatch.trace.begin(), mismatch.trace.end(),
                        "load") == mismatch.trace.end() &&
              mismatch.trace.back() == "play",
          "model mismatch does not load but still plays action");

    Runtime missing;
    missing.loaded = 0u;
    Check(pal4::ActorManagerPlayerActionPlayback::Play(
              actor, "run", 0xa5u, missing) == 0u &&
              std::count(missing.trace.begin(), missing.trace.end(),
                         "skill") == 2 &&
              std::count(missing.trace.begin(), missing.trace.end(),
                         "global") == 2 &&
              Position(missing.trace, "load") <
                  Position(missing.trace, "play") &&
              std::find(missing.trace.begin(), missing.trace.end(),
                        "frame") == missing.trace.end() &&
              missing.words.at(actor + 808u) == 0x6000u,
          "missing replacement leaves current model and still plays action");
}

void SwitchBeforePlayback() {
    Runtime runtime;
    Check(pal4::ActorManagerPlayerActionPlayback::Play(
              actor, "run", 0xa5u, runtime) == 0u &&
              runtime.words.at(actor + 808u) == runtime.loaded &&
              Position(runtime.trace, "load") <
                  Position(runtime.trace, "frame") &&
              Position(runtime.trace, "frame") <
                  Position(runtime.trace, "cleanup") &&
              Position(runtime.trace, "cleanup") <
                  Position(runtime.trace, "transform_actor") &&
              Position(runtime.trace, "transform_actor") <
                  Position(runtime.trace, "transform_position") &&
              Position(runtime.trace, "transform_position") <
                  Position(runtime.trace, "left:28672:1") &&
              Position(runtime.trace, "left:28672:1") <
                  Position(runtime.trace, "right:28672:1") &&
              Position(runtime.trace, "right:28672:1") <
                  Position(runtime.trace, "left:24576:0") &&
              Position(runtime.trace, "left:24576:0") <
                  Position(runtime.trace, "right:24576:0") &&
              Position(runtime.trace, "right:24576:0") <
                  Position(runtime.trace, "write_current") &&
              Position(runtime.trace, "write_current") <
                  Position(runtime.trace, "attachments") &&
              Position(runtime.trace, "attachments") <
                  Position(runtime.trace, "re_join_scene") &&
              runtime.trace.back() == "play",
          "new model is framed, transferred, attached and announced before action");

    Runtime same;
    same.loaded = 0x6000u;
    const auto result = pal4::ActorManagerPlayerActionPlayback::Play(
        actor, "run", 0xa5u, same);
    Check(result == 0u && Position(same.trace, "frame") <
              Position(same.trace, "transform_actor") &&
              Position(same.trace, "re_join_scene") <
                  Position(same.trace, "play"),
          "native action path does not skip a replacement equal to current");
}

}  // namespace

int main() {
    try {
        NoCurrentAndDirectAction();
        MismatchAndMissingReplacement();
        SwitchBeforePlayback();
        std::cout << "actor_manager_player_action_playback_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
