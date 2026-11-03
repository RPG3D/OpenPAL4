#include "pal4/actor_manager_player_animation_cache.h"

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

struct Runtime final : pal4::IActorManagerPlayerAnimationRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> strings;
    std::map<std::uint32_t, std::string> animation_names;
    std::vector<std::string> trace;
    std::uint32_t loaded = 0x7000u;
    std::uint32_t allocation = 0x8000u;
    std::uint32_t event_result = 0x9876u;
    std::uint32_t allocated_bytes = 0u;
    std::vector<std::uint32_t> freed;

    Runtime() { strings[name] = "run"; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        trace.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::string_view ReadCString(std::uint32_t address) override {
        trace.emplace_back("cstring:" + std::to_string(address));
        return strings.at(address);
    }
    std::string_view AnimationName(std::uint32_t animation) override {
        trace.emplace_back("name:" + std::to_string(animation));
        return animation_names.at(animation);
    }
    std::uint32_t SingletonManager() override {
        trace.emplace_back("singleton");
        return 0x9000u;
    }
    std::uint32_t LoadAnimation(std::uint32_t collection,
                                std::uint32_t requested) override {
        Check(collection == 0x9040u &&
                  (requested == name || requested == 0x841964u),
              "loader collection and requested name");
        trace.emplace_back("load");
        return loaded;
    }
    std::uint32_t AllocatePointerArray(std::uint32_t bytes) override {
        trace.emplace_back("allocate");
        allocated_bytes = bytes;
        return allocation;
    }
    void FreePointerArray(std::uint32_t address) override {
        trace.emplace_back("free:" + std::to_string(address));
        freed.push_back(address);
    }
    void GetFrame(std::uint32_t animation) override {
        Check(animation == loaded, "frame animation");
        trace.emplace_back("frame");
    }
    void RenderCamera(std::uint32_t animation,
                      std::uint32_t first,
                      std::uint32_t second) override {
        Check(animation == loaded && first == 0u && second == 0u,
              "camera render arguments");
        trace.emplace_back("render");
    }
    void CleanupAnimation(std::uint32_t animation,
                          std::uint32_t actual_actor) override {
        Check(animation == loaded && actual_actor == actor,
              "animation cleanup arguments");
        trace.emplace_back("cleanup");
    }
    std::uint32_t ProcessAnimationEvent(std::uint32_t actual_actor,
                                        std::uint32_t animation) override {
        Check(actual_actor == actor && animation == loaded,
              "animation event arguments");
        trace.emplace_back("event");
        return event_result;
    }
    void SetTransformFromActor(std::uint32_t animation,
                               std::uint32_t transform,
                               std::uint32_t flags) override {
        Check(animation == loaded && transform == 0x60acu && flags == 0u,
              "replacement receives old actor transform");
        trace.emplace_back("transform_actor");
    }
    void SetTransformFromPosition(std::uint32_t animation,
                                  std::uint32_t position,
                                  std::uint32_t flags) override {
        Check(animation == loaded && position == 0x60a0u && flags == 0u,
              "replacement receives old position");
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
    void DispatchRejoinScene(std::uint32_t actual_actor) override {
        Check(actual_actor == actor && words[actor + 808u] == loaded,
              "rejoin event observes updated animation");
        trace.emplace_back("re_join_scene");
    }
};

std::size_t Position(const std::vector<std::string>& trace,
                     const std::string& event) {
    const auto it = std::find(trace.begin(), trace.end(), event);
    Check(it != trace.end(), "animation operation missing");
    return static_cast<std::size_t>(it - trace.begin());
}

void CacheHitAndMiss() {
    using Cache = pal4::ActorManagerPlayerAnimationCache;
    Runtime runtime;
    runtime.words[actor + 1296u] = 0x5000u;
    runtime.words[actor + 1300u] = 0x5008u;
    runtime.words[actor + 1304u] = 0x5008u;
    runtime.words[0x5000u] = 0x6000u;
    runtime.words[0x5004u] = 0x6100u;
    runtime.animation_names[0x6000u] = "runner";
    runtime.animation_names[0x6100u] = "run";
    Check(Cache::FindOrLoad(actor, name, runtime) == 0x6100u &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "singleton") == runtime.trace.end(),
          "exact cached animation hits without singleton or load");

    Runtime failed;
    failed.loaded = 0u;
    Check(Cache::FindOrLoad(actor, name, failed) == 0u &&
              failed.trace.back() == "load" &&
              failed.words[actor + 1296u] == 0u &&
              failed.words[actor + 1300u] == 0u,
          "loader failure leaves an empty cache unchanged");
}

void AppendAndGrow() {
    using Cache = pal4::ActorManagerPlayerAnimationCache;
    Runtime spare;
    spare.words[actor + 1296u] = 0x5000u;
    spare.words[actor + 1300u] = 0x5004u;
    spare.words[actor + 1304u] = 0x5008u;
    spare.words[0x5000u] = 0x6000u;
    spare.animation_names[0x6000u] = "other";
    Check(Cache::FindOrLoad(actor, name, spare) == spare.loaded &&
              spare.words.at(0x5004u) == spare.loaded &&
              spare.words.at(actor + 1300u) == 0x5008u &&
              spare.allocated_bytes == 0u,
          "spare slot appends without reallocating");

    Runtime grow;
    grow.words[actor + 1296u] = 0x5000u;
    grow.words[actor + 1300u] = 0x5008u;
    grow.words[actor + 1304u] = 0x5008u;
    grow.words[0x5000u] = 0x6000u;
    grow.words[0x5004u] = 0x6100u;
    grow.animation_names[0x6000u] = "one";
    grow.animation_names[0x6100u] = "two";
    Check(Cache::FindOrLoad(actor, name, grow) == grow.loaded &&
              grow.allocated_bytes == 16u &&
              grow.words.at(0x8000u) == 0x6000u &&
              grow.words.at(0x8004u) == 0x6100u &&
              grow.words.at(0x8008u) == grow.loaded &&
              grow.words.at(actor + 1296u) == 0x8000u &&
              grow.words.at(actor + 1300u) == 0x800cu &&
              grow.words.at(actor + 1304u) == 0x8010u &&
              grow.freed == std::vector<std::uint32_t>{0x5000u} &&
              Position(grow.trace, "free:20480") <
                  Position(grow.trace, "write:5392"),
          "full two-slot cache grows to four and preserves both entries");

    Runtime first;
    Check(Cache::FindOrLoad(actor, name, first) == first.loaded &&
              first.allocated_bytes == 4u &&
              first.words.at(actor + 1296u) == 0x8000u &&
              first.words.at(actor + 1300u) == 0x8004u &&
              first.words.at(actor + 1304u) == 0x8004u,
          "empty cache allocates one pointer slot");

    Runtime allocation_failed;
    allocation_failed.allocation = 0u;
    Check(Cache::FindOrLoad(actor, name, allocation_failed) == 0u &&
              allocation_failed.words[actor + 1296u] == 0u &&
              allocation_failed.words[actor + 1300u] == 0u,
          "failed pointer-array allocation has defined no-insert result");
}

void ActivateTail() {
    using Cache = pal4::ActorManagerPlayerAnimationCache;
    Runtime runtime;
    runtime.strings[Cache::kEmptyName] = "";
    Check(Cache::Activate(actor, runtime) == runtime.event_result &&
              runtime.words.at(actor + 808u) == runtime.loaded &&
              Position(runtime.trace, "cstring:8657252") <
                  Position(runtime.trace, "load") &&
              Position(runtime.trace, "frame") <
                  Position(runtime.trace, "render") &&
              Position(runtime.trace, "render") <
                  Position(runtime.trace, "cleanup") &&
              runtime.trace.back() == "event",
          "empty name fallback loads and runs noncontiguous animation tail");

    Runtime absent;
    absent.loaded = 0u;
    absent.words[actor + 1148u] = name;
    Check(Cache::Activate(actor, absent) == 0u &&
              absent.words.at(actor + 808u) == 0u &&
              std::find(absent.trace.begin(), absent.trace.end(),
                        "frame") == absent.trace.end(),
          "failed load stores null and skips frame/render/cleanup tail");
}

void SwitchAlreadyNamed() {
    Runtime runtime;
    runtime.words[actor + 808u] = 0x6000u;
    runtime.words[actor + 1148u] = name;
    runtime.words[actor + 1152u] = 3u;
    runtime.animation_names[0x6000u] = "run";
    pal4::ActorManagerPlayerAnimationCache::SwitchToRequestedAnimation(
        actor, runtime);
    Check(runtime.words.at(actor + 808u) == 0x6000u &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "singleton") == runtime.trace.end() &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "transform_actor") == runtime.trace.end(),
          "matching current animation skips cache and switching");
}

void SwitchCached() {
    Runtime runtime;
    runtime.words[actor + 808u] = 0x6000u;
    runtime.words[actor + 1148u] = name;
    runtime.words[actor + 1152u] = 3u;
    runtime.words[actor + 1296u] = 0x5000u;
    runtime.words[actor + 1300u] = 0x5008u;
    runtime.words[0x5000u] = 0x6200u;
    runtime.words[0x5004u] = runtime.loaded;
    runtime.animation_names[0x6000u] = "idle";
    runtime.animation_names[0x6200u] = "runner";
    runtime.animation_names[runtime.loaded] = "run";
    pal4::ActorManagerPlayerAnimationCache::SwitchToRequestedAnimation(
        actor, runtime);
    Check(runtime.words.at(actor + 808u) == runtime.loaded &&
              Position(runtime.trace, "name:25088") <
                  Position(runtime.trace, "name:28672") &&
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
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "singleton") == runtime.trace.end() &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "re_join_scene") == runtime.trace.end(),
          "cached animation switches both transforms and hands without event");
}

void SwitchLoadedAndFailure() {
    Runtime runtime;
    runtime.words[actor + 808u] = 0x6000u;
    runtime.words[actor + 1148u] = name;
    runtime.words[actor + 1152u] = 3u;
    runtime.animation_names[0x6000u] = "idle";
    pal4::ActorManagerPlayerAnimationCache::SwitchToRequestedAnimation(
        actor, runtime);
    Check(runtime.words.at(actor + 808u) == runtime.loaded &&
              runtime.words.at(runtime.allocation) == runtime.loaded &&
              Position(runtime.trace, "load") <
                  Position(runtime.trace, "allocate") &&
              Position(runtime.trace, "right:24576:0") <
                  Position(runtime.trace, "re_join_scene") &&
              runtime.trace.back() == "re_join_scene",
          "cache miss loads, inserts, switches and dispatches rejoin event");

    Runtime load_failed;
    load_failed.loaded = 0u;
    load_failed.words[actor + 808u] = 0x6000u;
    load_failed.words[actor + 1148u] = name;
    load_failed.words[actor + 1152u] = 3u;
    load_failed.animation_names[0x6000u] = "idle";
    pal4::ActorManagerPlayerAnimationCache::SwitchToRequestedAnimation(
        actor, load_failed);
    Check(load_failed.words.at(actor + 808u) == 0x6000u &&
              load_failed.words.at(load_failed.allocation) == 0u &&
              load_failed.words.at(actor + 1300u) ==
                  load_failed.allocation + 4u &&
              std::find(load_failed.trace.begin(), load_failed.trace.end(),
                        "transform_actor") == load_failed.trace.end() &&
              std::find(load_failed.trace.begin(), load_failed.trace.end(),
                        "re_join_scene") == load_failed.trace.end(),
          "failed load inserts null but leaves current animation active");

    Runtime alloc_failed;
    alloc_failed.allocation = 0u;
    alloc_failed.words[actor + 808u] = 0x6000u;
    alloc_failed.words[actor + 1148u] = name;
    alloc_failed.words[actor + 1152u] = 3u;
    alloc_failed.animation_names[0x6000u] = "idle";
    pal4::ActorManagerPlayerAnimationCache::SwitchToRequestedAnimation(
        actor, alloc_failed);
    Check(alloc_failed.words.at(actor + 808u) == 0x6000u &&
              alloc_failed.words[actor + 1296u] == 0u &&
              std::find(alloc_failed.trace.begin(), alloc_failed.trace.end(),
                        "re_join_scene") == alloc_failed.trace.end(),
          "failed allocation leaves cache and current animation unchanged");
}

void SwitchSamePointerAndEmptyName() {
    Runtime same;
    same.loaded = 0x6000u;
    same.words[actor + 808u] = 0x6000u;
    same.words[actor + 1148u] = name;
    same.words[actor + 1152u] = 3u;
    same.animation_names[0x6000u] = "idle";
    pal4::ActorManagerPlayerAnimationCache::SwitchToRequestedAnimation(
        actor, same);
    Check(same.words.at(same.allocation) == 0x6000u &&
              std::find(same.trace.begin(), same.trace.end(),
                        "transform_actor") == same.trace.end() &&
              std::find(same.trace.begin(), same.trace.end(),
                        "re_join_scene") == same.trace.end(),
          "same pointer after load is cached without switching or event");

    Runtime empty;
    empty.words[actor + 808u] = 0x6000u;
    empty.words[actor + 1152u] = 0u;
    empty.animation_names[0x6000u] = "idle";
    empty.strings[pal4::ActorManagerPlayerAnimationCache::kEmptyName] = "";
    pal4::ActorManagerPlayerAnimationCache::SwitchToRequestedAnimation(
        actor, empty);
    Check(Position(empty.trace, "cstring:8657252") <
              Position(empty.trace, "load") &&
              empty.trace.back() == "re_join_scene",
          "null requested name uses shared empty text");
}

}  // namespace

int main() {
    try {
        CacheHitAndMiss();
        AppendAndGrow();
        ActivateTail();
        SwitchAlreadyNamed();
        SwitchCached();
        SwitchLoadedAndFailure();
        SwitchSamePointerAndEmptyName();
        std::cout << "actor_manager_player_animation_cache_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
