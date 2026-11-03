#include "pal4/game_object_embedded_animation.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::IGameObjectInterpolatorRuntime {
public:
    std::vector<std::string> calls;
    std::uint32_t hierarchy = 0;
    std::uint32_t free_result = 0x77u;
    std::uint32_t current_animation = 0xaau;
    std::uint32_t next_action_name = 0xbbu;
    float duration = 1.0f;
    float time = 0.0f;
    float blend_weight = -1.0f;
    float record_duration = 1.25f;
    std::uint32_t record_animation = 0xbbu;

    void InitializeAnimationData(
        pal4::GameObjectEmbeddedAnimationState& state) override {
        Check(state.words[7] == 0 && state.words[8] == 0 &&
                  state.words[9] == 0 && state.words[1] == 0xaaaaaaaau,
              "manager init occurs after first three clears");
        calls.emplace_back("init");
        state.words[20] = 0x1234u;
    }
    void CleanupAnimationData(
        pal4::GameObjectEmbeddedAnimationState& state) override {
        Check(state.words[8] == state.words[7],
              "manager cleanup follows data cleanup");
        calls.emplace_back("manager-cleanup");
    }
    void CleanupAnimationDataRange(
        pal4::GameObjectEmbeddedAnimationState& state) override {
        Check(state.words[8] == state.words[7] &&
                  state.words[10] == 0 && state.words[11] == 0,
              "range cleanup follows field reset");
        calls.emplace_back("range-cleanup");
    }
    std::uint32_t AnimationArrayCount(std::uint32_t address) override {
        Check(address == 0x1fcu, "array count resides before elements");
        calls.emplace_back("array-count");
        return 3;
    }
    void DestructAnimationArray(std::uint32_t address,
                                std::uint32_t element_size,
                                std::uint32_t count) override {
        Check(address == 0x200u && element_size == 0x44u && count == 3,
              "array destructor receives element size and count");
        calls.emplace_back("array-destruct");
    }
    std::uint32_t FreeMemory(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
        return free_result;
    }
    void FreeWrappedMemory(std::uint32_t address) override {
        calls.emplace_back("wrapper:" + std::to_string(address));
    }
    std::uint32_t FindHierarchy(std::uint32_t clump) override {
        Check(clump == 0x55u, "hierarchy lookup receives clump");
        calls.emplace_back("hierarchy");
        return hierarchy;
    }
    void ProcessAnimationData(
        pal4::GameObjectEmbeddedAnimationState&,
        std::uint32_t found_hierarchy, std::int32_t index) override {
        Check(found_hierarchy == 0x99u, "process receives hierarchy");
        calls.emplace_back("process:" + std::to_string(index));
    }
    std::uint32_t HierarchyInterpolator(std::uint32_t found_hierarchy) override {
        Check(found_hierarchy == 0x99u, "reads hierarchy interpolator");
        calls.emplace_back("interpolator");
        return 0x88u;
    }
    std::uint32_t CurrentAnimation(std::uint32_t interpolator) override {
        Check(interpolator == 0x88u, "reads current animation");
        calls.emplace_back("current-animation");
        return current_animation;
    }
    float AnimationDuration(std::uint32_t animation) override {
        Check(animation == 0xaau || animation == 0xbbu,
              "reads current or record animation duration");
        calls.emplace_back("duration");
        return animation == 0xbbu ? record_duration : duration;
    }
    std::uint32_t RecordAnimationPointer(std::uint32_t record) override {
        Check(record == 0x77u, "reads animation pointer from current record");
        calls.emplace_back("record-animation");
        return record_animation;
    }
    float InterpolatorTime(std::uint32_t interpolator) override {
        Check(interpolator == 0x88u, "reads interpolator time");
        calls.emplace_back("time");
        return time;
    }
    void SetInterpolatorTime(std::uint32_t interpolator,
                             float value) override {
        Check(interpolator == 0x88u, "sets interpolator time");
        calls.emplace_back("set-time");
        time = value;
    }
    void AddInterpolatorTime(std::uint32_t interpolator,
                             float delta) override {
        Check(interpolator == 0x88u, "adds interpolator time");
        calls.emplace_back("add-time");
        time += delta;
    }
    void UpdateAnimationTime(
        pal4::GameObjectEmbeddedAnimationState&,
        float updated_time) override {
        Check(updated_time == time, "updates events from current interpolator time");
        calls.emplace_back("update-time");
    }
    void CleanupCurrentRecord(std::uint32_t record) override {
        Check(record == 0x77u, "cleans current animation record");
        calls.emplace_back("record-cleanup");
    }
    std::uint32_t NextActionName(std::uint32_t record) override {
        Check(record == 0x77u, "reads next action from current record");
        calls.emplace_back("next-name");
        return next_action_name;
    }
    std::uint32_t EmptyActionName() override {
        calls.emplace_back("empty-name");
        return 0xbeu;
    }
    void DispatchNextAction(
        pal4::GameObjectEmbeddedAnimationState&,
        std::uint32_t name, float blend_time) override {
        Check((name == 0xbbu || name == 0xbeu) &&
                  std::abs(blend_time - 0.2f) < 0.000001f,
              "dispatches next action with fixed blend time");
        calls.emplace_back("next-action");
    }
    void BlendAnimations(std::uint32_t interpolator,
                         std::uint32_t from, std::uint32_t to,
                         float weight) override {
        Check(interpolator == 0x88u && from == 0x11u && to == 0x22u,
              "blends the two stored interpolators");
        calls.emplace_back("blend");
        blend_weight = weight;
    }
    std::uint32_t AnimationFromInterpolator(
        std::uint32_t interpolator) override {
        Check(interpolator == 0x22u,
              "reads target animation from blend interpolator");
        calls.emplace_back("target-animation");
        return 0xccu;
    }
    void SetInterpolatorAnimation(std::uint32_t interpolator,
                                  std::uint32_t animation) override {
        Check(interpolator == 0x88u && animation == 0xccu,
              "switches to target animation");
        calls.emplace_back("set-animation");
    }
    void UpdateHierarchyMatrices(std::uint32_t found_hierarchy) override {
        Check(found_hierarchy == 0x99u, "updates active hierarchy matrices");
        calls.emplace_back("matrices");
    }
};

void InitializeState() {
    Runtime runtime;
    pal4::GameObjectEmbeddedAnimationState state;
    state.words.fill(0xaaaaaaaau);
    Check(&pal4::GameObjectEmbeddedAnimation::InitializeState(
              state, 0x42u, runtime) == &state &&
              runtime.calls == std::vector<std::string>{"init"},
          "initializer returns state and initializes embedded manager");
    for (const auto index : {1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 13, 14, 15,
                             16, 17, 18, 19, 24, 25, 26, 27, 28, 29, 30})
        Check(state.words[index] == 0, "initializer clears recorded field");
    Check(state.words[0] == 0x84b948u &&
              state.words[6] == 0xaaaaaa42u &&
              state.words[12] == 0x3f800000u &&
              state.words[20] == 0x1234u &&
              state.words[21] == 0xaaaaaaaau,
          "initializer preserves unspecified words and manager state");
}

void CleanupDataAndState() {
    Runtime runtime;
    pal4::GameObjectEmbeddedAnimationState state;
    state.words[1] = 0x100u;
    state.words[2] = 0x200u;
    state.words[3] = 0x222u;
    state.words[4] = 0x300u;
    state.words[5] = 0x400u;
    state.words[7] = 0x500u;
    state.words[8] = 0x600u;
    state.words[10] = 0x700u;
    state.words[11] = 0x800u;
    state.words[28] = 0x900u;
    state.words[29] = 0xa00u;
    state.words[30] = 0xb00u;
    Check(pal4::GameObjectEmbeddedAnimation::CleanupData(state, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "array-count", "array-destruct", "free:508",
                  "wrapper:768", "wrapper:1024", "range-cleanup",
                  "free:2304", "free:2560", "free:2816"},
          "data cleanup destroys and releases owned allocations in order");
    for (const auto index : {1, 2, 3, 4, 5, 10, 11, 28, 29, 30})
        Check(state.words[index] == 0, "cleanup clears released fields");
    Check(state.words[7] == 0x500u && state.words[8] == 0x500u,
          "data cleanup keeps capacity and moves end to begin");

    runtime.calls.clear();
    state.words[0] = 0;
    Check(pal4::GameObjectEmbeddedAnimation::CleanupState(state, runtime) ==
                  0x77u &&
              state.words[0] == 0x84b948u &&
              state.words[7] == 0 && state.words[8] == 0 &&
              state.words[9] == 0 &&
              runtime.calls == std::vector<std::string>{
                  "range-cleanup", "manager-cleanup", "free:1280"},
          "state cleanup restores vtable and frees capacity last");
    runtime.calls.clear();
    Check(pal4::GameObjectEmbeddedAnimation::CleanupState(state, runtime) ==
                  0x77u && runtime.calls.back() == "free:0",
          "state cleanup forwards null capacity to original free call");
}

void ProcessAnimation() {
    Runtime runtime;
    pal4::GameObjectEmbeddedAnimationState state;
    Check(!pal4::GameObjectEmbeddedAnimation::ProcessAnimation(
              state, 123, runtime) && runtime.calls.empty(),
          "missing clump stops before hierarchy lookup");
    state.words[1] = 0x55u;
    Check(!pal4::GameObjectEmbeddedAnimation::ProcessAnimation(
              state, 123, runtime) &&
              runtime.calls == std::vector<std::string>{"hierarchy"},
          "missing hierarchy returns false");
    runtime.calls.clear();
    runtime.hierarchy = 0x99u;
    Check(pal4::GameObjectEmbeddedAnimation::ProcessAnimation(
              state, 123, runtime) &&
              runtime.calls == std::vector<std::string>{"hierarchy"},
          "disabled animation returns true without processing");
    runtime.calls.clear();
    state.words[24] = 1;
    state.words[25] = 2;
    Check(pal4::GameObjectEmbeddedAnimation::ProcessAnimation(
              state, 123, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "hierarchy", "process:0", "process:1"},
          "enabled animation processes each signed index");
    runtime.calls.clear();
    state.words[25] = 0xffffffffu;
    Check(pal4::GameObjectEmbeddedAnimation::ProcessAnimation(
              state, 123, runtime) &&
              runtime.calls == std::vector<std::string>{"hierarchy"},
          "negative signed count performs no processing");
}

void UpdateInterpolator() {
    Runtime runtime;
    pal4::GameObjectEmbeddedAnimationState state;
    state.words[12] = std::bit_cast<std::uint32_t>(2.0f);
    Check(!pal4::GameObjectEmbeddedAnimation::UpdateInterpolator(
              state, 0.1f, runtime) && runtime.calls.empty(),
          "update requires clump and current record");
    state.words[1] = 0x55u;
    Check(!pal4::GameObjectEmbeddedAnimation::UpdateInterpolator(
              state, 0.1f, runtime) && runtime.calls.empty(),
          "update requires current record after clump");
    state.words[3] = 0x77u;
    state.words[19] = 1;
    Check(pal4::GameObjectEmbeddedAnimation::UpdateInterpolator(
              state, 0.1f, runtime) && runtime.calls.empty(),
          "pause returns true before hierarchy access");
    state.words[19] = 0;
    Check(!pal4::GameObjectEmbeddedAnimation::UpdateInterpolator(
              state, 0.1f, runtime) &&
              runtime.calls == std::vector<std::string>{"hierarchy"},
          "missing hierarchy returns false");
    runtime.calls.clear();
    runtime.hierarchy = 0x99u;
    runtime.current_animation = 0;
    Check(!pal4::GameObjectEmbeddedAnimation::UpdateInterpolator(
              state, 0.1f, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "hierarchy", "interpolator", "current-animation"},
          "missing current animation returns false");
    runtime.calls.clear();
    runtime.current_animation = 0xaau;
    runtime.time = 0.2f;
    state.words[18] = 1;
    Check(pal4::GameObjectEmbeddedAnimation::UpdateInterpolator(
              state, 0.1f, runtime) &&
              std::abs(runtime.time - 0.4f) < 0.000001f &&
              state.words[18] == 0 &&
              runtime.calls == std::vector<std::string>{
                  "hierarchy", "interpolator", "current-animation",
                  "duration", "time", "add-time", "time", "update-time",
                  "matrices"},
          "scaled delta advances time and clears completion flag");
    runtime.calls.clear();
    runtime.time = 0.9f;
    Check(pal4::GameObjectEmbeddedAnimation::UpdateInterpolator(
              state, 0.1f, runtime) && runtime.time == 1.0f &&
              state.words[18] == 1 &&
              runtime.calls == std::vector<std::string>{
                  "hierarchy", "interpolator", "current-animation",
                  "duration", "time", "set-time", "time", "update-time",
                  "matrices"},
          "end of animation clamps time and marks completion");

    runtime.calls.clear();
    state.words[17] = 1;
    Check(pal4::GameObjectEmbeddedAnimation::UpdateInterpolator(
              state, 0.1f, runtime) && runtime.time == 0.0f &&
              state.words[18] == 1 &&
              runtime.calls == std::vector<std::string>{
                  "hierarchy", "interpolator", "current-animation",
                  "duration", "time", "set-time", "time", "update-time",
                  "record-cleanup", "set-time", "matrices"},
          "restart cleans record and rewinds interpolator");
    runtime.calls.clear();
    runtime.time = 0.9f;
    state.words[16] = 1;
    runtime.next_action_name = 0;
    Check(pal4::GameObjectEmbeddedAnimation::UpdateInterpolator(
              state, 0.1f, runtime) && runtime.time == 1.0f &&
              state.words[18] == 1 &&
              runtime.calls == std::vector<std::string>{
                  "hierarchy", "interpolator", "current-animation",
                  "duration", "time", "set-time", "time", "update-time",
                  "record-cleanup", "next-name", "empty-name",
                  "next-action", "matrices"},
          "next-action branch dispatches fallback name without rewind");

    runtime.calls.clear();
    state.words[15] = 1;
    state.words[13] = std::bit_cast<std::uint32_t>(1.0f);
    state.words[14] = std::bit_cast<std::uint32_t>(0.5f);
    state.words[4] = 0x11u;
    state.words[5] = 0x22u;
    Check(pal4::GameObjectEmbeddedAnimation::UpdateInterpolator(
              state, 0.1f, runtime) &&
              std::abs(runtime.blend_weight - 0.5f) < 0.000001f &&
              std::abs(std::bit_cast<float>(state.words[14]) - 0.3f) <
                  0.000001f &&
              runtime.calls == std::vector<std::string>{
                  "hierarchy", "interpolator", "current-animation",
                  "duration", "blend", "matrices"},
          "active blend bypasses time advance and reduces remaining time");
    runtime.calls.clear();
    Check(pal4::GameObjectEmbeddedAnimation::UpdateInterpolator(
              state, 0.2f, runtime) &&
              state.words[13] == 0 && state.words[14] == 0 &&
              state.words[15] == 0 && runtime.time == 0.0f &&
              runtime.calls == std::vector<std::string>{
                  "hierarchy", "interpolator", "current-animation",
                  "duration", "blend", "target-animation",
                  "set-animation", "set-time", "matrices"},
          "finished blend switches animation and clears blend fields");
    runtime.calls.clear();
    state.words[15] = 1;
    state.words[13] = 0;
    Check(pal4::GameObjectEmbeddedAnimation::UpdateInterpolator(
              state, 0.1f, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "hierarchy", "interpolator", "current-animation",
                  "duration", "matrices"},
          "nonpositive blend duration skips blending");
}

void FlagsAndTimeAccess() {
    Runtime runtime;
    pal4::GameObjectEmbeddedAnimationState state;
    state.words[4] = 0x55u;
    Check(pal4::GameObjectEmbeddedAnimation::SetRestartFlag(
              state, 7u) == 7u && state.words[17] == 7u &&
              pal4::GameObjectEmbeddedAnimation::SetTimeScaleBits(
                  state, 0xbf800000u) == 0xbf800000u &&
              state.words[12] == 0xbf800000u &&
              pal4::GameObjectEmbeddedAnimation::SetPauseFlag(
                  state, 3u) == 3u && state.words[19] == 3u &&
              state.words[4] == 0x55u &&
              pal4::GameObjectEmbeddedAnimation::CompletedFlag(state) == 0,
          "short setters write and return only their original word");
    state.words[18] = 0x1234u;
    Check(pal4::GameObjectEmbeddedAnimation::CompletedFlag(state) == 0x1234u,
          "completion getter returns the raw word");
    Check(pal4::GameObjectEmbeddedAnimation::AnimationLength(
              state, runtime) == 0.0 && runtime.calls.empty(),
          "length without current record is zero");
    state.words[3] = 0x77u;
    runtime.record_animation = 0;
    Check(pal4::GameObjectEmbeddedAnimation::AnimationLength(
              state, runtime) == 0.0 &&
              runtime.calls == std::vector<std::string>{"record-animation"},
          "length without record animation returns zero");
    runtime.calls.clear();
    runtime.record_animation = 0xbbu;
    Check(pal4::GameObjectEmbeddedAnimation::AnimationLength(
              state, runtime) == 1.25 &&
              runtime.calls == std::vector<std::string>{
                  "record-animation", "duration"},
          "length converts current record float to double");
    runtime.calls.clear();
    Check(pal4::GameObjectEmbeddedAnimation::AnimationTime(
              state, runtime) == 0.0 && runtime.calls.empty(),
          "time without clump is zero");
    Check(!pal4::GameObjectEmbeddedAnimation::SetTime(
              state, 0.5f, runtime) && runtime.calls.empty(),
          "set time without clump stops immediately");
    state.words[1] = 0x55u;
    runtime.hierarchy = 0;
    Check(!pal4::GameObjectEmbeddedAnimation::SetTime(
              state, 0.5f, runtime) &&
              runtime.calls == std::vector<std::string>{"hierarchy"},
          "set time requires hierarchy");
    runtime.calls.clear();
    runtime.hierarchy = 0x99u;
    runtime.current_animation = 0;
    Check(!pal4::GameObjectEmbeddedAnimation::SetTime(
              state, 0.5f, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "hierarchy", "interpolator", "current-animation"},
          "set time requires current animation");
    runtime.calls.clear();
    runtime.current_animation = 0xaau;
    Check(!pal4::GameObjectEmbeddedAnimation::SetTime(
              state, -0.1f, runtime) &&
              runtime.calls.back() == "current-animation",
          "negative time is rejected before duration lookup");
    runtime.calls.clear();
    Check(!pal4::GameObjectEmbeddedAnimation::SetTime(
              state, 1.1f, runtime) &&
              runtime.calls.back() == "duration",
          "time above animation duration is rejected");
    runtime.calls.clear();
    Check(pal4::GameObjectEmbeddedAnimation::SetTime(
              state, 1.0f, runtime) && runtime.time == 1.0f &&
              runtime.calls == std::vector<std::string>{
                  "hierarchy", "interpolator", "current-animation",
                  "duration", "set-time", "matrices"},
          "duration endpoint sets time and updates matrices");
    runtime.calls.clear();
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    Check(pal4::GameObjectEmbeddedAnimation::SetTime(
              state, nan, runtime) && std::isnan(runtime.time) &&
              runtime.calls.back() == "matrices",
          "unordered x87 comparisons accept NaN time");
    runtime.calls.clear();
    runtime.time = 0.75f;
    Check(pal4::GameObjectEmbeddedAnimation::AnimationTime(
              state, runtime) == 0.75 &&
              runtime.calls == std::vector<std::string>{
                  "hierarchy", "interpolator", "time"},
          "time getter converts interpolator float to double");
}

}  // namespace

int main() {
    InitializeState();
    CleanupDataAndState();
    ProcessAnimation();
    UpdateInterpolator();
    FlagsAndTimeAccess();
    std::cout << "game_object_embedded_animation_tests passed\n";
}
