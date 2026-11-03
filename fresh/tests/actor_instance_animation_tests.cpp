#include "pal4/actor_instance_animation.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr auto actor = 0x1000u;

struct Runtime final : pal4::IActorInstanceAnimationRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    bool clump_load_succeeds = true;
    bool shadow_load_succeeds = true;
    std::uint32_t get_frame_result = 0x89u;
    std::uint32_t update_result = 0xabu;
    std::string directory_seen;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    void SetAnimationFlag(const std::uint32_t state,
                          const std::uint32_t value) override {
        Check(state == actor + 368u && value == 0u,
              "get-frame flag target");
        trace.push_back("flag");
    }
    std::string_view GlobalAnimationName() override {
        trace.push_back("global-name");
        return {};
    }
    void SetAnimationName(const std::uint32_t state,
                          const std::string_view name) override {
        Check(state == actor + 368u && name.empty(),
              "global animation name fallback");
        trace.push_back("set-name");
    }
    void InvokeActorSlot24(const std::uint32_t address,
                           const std::uint32_t value) override {
        Check(address == actor && value == 12u, "actor slot 24 argument");
        trace.push_back("slot24");
    }
    std::uint32_t InvokeActorSlot20(const std::uint32_t address) override {
        Check(address == actor, "actor slot 20 target");
        trace.push_back("slot20");
        return get_frame_result;
    }
    void BeginResourceContext() override { trace.push_back("begin-context"); }
    void SetResourceName(const std::uint32_t field,
                         const std::string_view name) override {
        Check(field == actor + 496u && name == "hero",
              "resource name field and value");
        trace.push_back("resource-name");
    }
    std::string_view PathPrefixOne() override {
        trace.push_back("prefix-one");
        return "root/";
    }
    std::string_view PathPrefixTwo() override {
        trace.push_back("prefix-two");
        return "models/";
    }
    std::uint32_t GameInstance() override {
        trace.push_back("game-instance");
        return 0x8000u;
    }
    bool LoadClump(const std::uint32_t address,
                   const std::string_view resource_name,
                   const std::string_view directory,
                   const std::string_view action_name,
                   const std::uint32_t game_context) override {
        Check(address == actor && resource_name == "hero" &&
              directory == "root/models/run\\" &&
              action_name == "run" &&
              game_context == 0x8000u + 428u,
              "clump load resource, directory and game context");
        directory_seen = directory;
        trace.push_back("load-clump");
        return clump_load_succeeds;
    }
    void LoadAnimationData(const std::uint32_t state,
                           const std::uint32_t clump,
                           const std::string_view directory,
                           const std::string_view action_name) override {
        Check(state == actor + 368u && clump == 0x5000u &&
              directory == "root/models/run\\" && action_name == "run",
              "animation data inputs");
        trace.push_back("load-animation");
    }
    bool LoadShadowClump(const std::uint32_t address,
                         const std::uint32_t game_context) override {
        Check(address == actor && game_context == 0x8000u + 428u,
              "shadow game context");
        trace.push_back("load-shadow");
        return shadow_load_succeeds;
    }
    void AttachHierarchy(const std::uint32_t state,
                         const std::uint32_t hierarchy) override {
        Check(state == actor + 368u && hierarchy == 0x6000u,
              "shadow hierarchy attachment");
        trace.push_back("attach-hierarchy");
    }
    void InitializeBoundingBox(const std::uint32_t box,
                               const std::uint32_t clump) override {
        Check(box == actor + 524u && clump == 0x5000u,
              "bounding box and clump");
        trace.push_back("bounding-box");
    }
    void HideHookAtomics(const std::uint32_t clump) override {
        Check(clump == 0x5000u, "loaded clump hook atomics");
        trace.push_back("hide-atomics");
    }
    void EndResourceContext() override { trace.push_back("end-context"); }
    void RenderClump(const std::uint32_t address,
                     const std::uint32_t argument) override {
        Check(address == actor && argument == 3u, "render clump arguments");
        trace.push_back("render-clump");
    }
    void ProcessAnimation(const std::uint32_t state,
                          const std::uint32_t argument) override {
        Check(state == actor + 368u && argument == 3u,
              "animation processing arguments");
        trace.push_back("process-animation");
    }
    void RenderChild(const std::uint32_t child,
                     const std::uint32_t argument) override {
        Check(argument == 3u, "child render argument");
        trace.push_back("render-child:" + std::to_string(child));
    }
    void UpdateAndRenderClump(const std::uint32_t address,
                              const std::uint32_t elapsed_bits) override {
        Check(address == actor && elapsed_bits == 0x3f800000u,
              "clump update argument bits");
        trace.push_back("update-clump");
    }
    void UpdateInterpolator(const std::uint32_t state,
                            const std::uint32_t elapsed_bits) override {
        Check(state == actor + 368u && elapsed_bits == 0x3f800000u,
              "interpolator state and elapsed bits");
        trace.push_back("update-interpolator");
    }
    std::uint32_t UpdateBoundingBox(const std::uint32_t box,
                                    const std::uint32_t clump) override {
        Check(box == actor + 524u && clump == 0x5000u,
              "bounding box update arguments");
        trace.push_back("update-box");
        return update_result;
    }
};

void GetFrameDispatch() {
    Runtime runtime;
    runtime.words[actor + 520u] = 12u;
    Check(pal4::ActorInstanceAnimation::GetFrame(actor, runtime) == 0x89u &&
          runtime.trace == std::vector<std::string>{
              "flag", "global-name", "set-name", "slot24", "slot20"},
          "get-frame uses animation state then actor virtual slots");
}

void PlayFailureAndSuccess() {
    Runtime failure;
    failure.clump_load_succeeds = false;
    Check(!pal4::ActorInstanceAnimation::Play(
              actor, "hero", "run", failure) &&
          failure.trace == std::vector<std::string>{
              "begin-context", "resource-name", "prefix-one",
              "prefix-two", "game-instance", "load-clump"},
          "clump-load failure skips animation setup and native context cleanup");
    Runtime success;
    success.words[actor + 20u] = 0x5000u;
    success.words[actor + 48u] = 0x6000u;
    success.words[actor + 492u] = 7u;
    for (auto index = 0u; index < 4u; ++index)
        success.words[actor + 208u + 4u * index] = 100u + index;
    Check(pal4::ActorInstanceAnimation::Play(
              actor, "hero", "run", success) &&
          success.trace == std::vector<std::string>{
              "begin-context", "resource-name", "prefix-one",
              "prefix-two", "game-instance", "load-clump",
              "load-animation", "game-instance", "load-shadow",
              "attach-hierarchy", "bounding-box", "hide-atomics",
              "end-context"} &&
          success.directory_seen == "root/models/run\\" &&
          success.words[actor + 516u] == 7u &&
          success.words[actor + 412u] == actor + 512u &&
          success.words[actor + 520u] == 0xffffffffu,
          "success loads shadow, updates actor links and closes context");
    for (auto index = 0u; index < 4u; ++index)
        Check(success.words[actor + 608u + 4u * index] == 100u + index,
              "four actor state words copy to +608");
    Runtime no_shadow;
    no_shadow.shadow_load_succeeds = false;
    no_shadow.words[actor + 20u] = 0x5000u;
    Check(pal4::ActorInstanceAnimation::Play(
              actor, "hero", "run", no_shadow) &&
          no_shadow.trace[9] == "bounding-box",
          "missing shadow skips hierarchy attachment but completes load");
}

void RenderAndTick() {
    Runtime runtime;
    runtime.words[actor + 20u] = 0x5000u;
    pal4::ActorInstanceAnimation::Render(actor, 3u, runtime);
    Check(runtime.trace == std::vector<std::string>{
              "render-clump", "process-animation", "render-child:4720",
              "render-child:5184"},
          "render path processes two embedded actor children in order");
    runtime.trace.clear();
    Check(pal4::ActorInstanceAnimation::Update(
              actor, 0x3f800000u, runtime) == runtime.update_result &&
          runtime.trace == std::vector<std::string>{
              "update-clump", "update-interpolator", "update-box"},
          "tick updates clump, interpolator and bounds in order");
}

}  // namespace

int main() {
    try {
        GetFrameDispatch();
        PlayFailureAndSuccess();
        RenderAndTick();
        std::cout << "actor_instance_animation_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
