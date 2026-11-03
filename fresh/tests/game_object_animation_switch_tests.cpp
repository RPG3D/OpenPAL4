#include "pal4/game_object_animation_switch.h"

#include <algorithm>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::IGameObjectAnimationSwitchRuntime {
public:
    std::vector<std::string> calls;
    std::vector<std::string> names{"idle", "run"};
    std::uint32_t hierarchy = 0x700u;

    std::uint32_t FindHierarchy(std::uint32_t clump) override {
        Check(clump == 0x55u, "resolves source clump hierarchy");
        calls.emplace_back("hierarchy");
        return hierarchy;
    }
    void SetHierarchyFlags(std::uint32_t found,
                           std::uint32_t flags) override {
        Check(found == 0x700u && flags == 0x3000u,
              "sets animation hierarchy flags");
        calls.emplace_back("flags");
    }
    void AttachHierarchy(std::uint32_t found) override {
        Check(found == 0x700u, "attaches source hierarchy");
        calls.emplace_back("attach");
    }
    void AttachSkinAtomics(std::uint32_t clump,
                           std::uint32_t found) override {
        Check(clump == 0x66u && found == 0x700u,
              "binds target clump atomics to hierarchy");
        calls.emplace_back("atomics");
    }
    std::int32_t AnimationCount(
        pal4::GameObjectEmbeddedAnimationState&) override {
        calls.emplace_back("count");
        return static_cast<std::int32_t>(names.size());
    }
    std::string_view ActionName(
        pal4::GameObjectEmbeddedAnimationState&,
        std::int32_t index) override {
        calls.emplace_back("name:" + std::to_string(index));
        return names.at(static_cast<std::size_t>(index));
    }
    int CompareActionNames(std::string_view stored,
                           std::string_view requested) override {
        calls.emplace_back("compare");
        return stored == requested ? 0 : 1;
    }
    void CleanupCurrentRecord(std::uint32_t record) override {
        Check(record == 0x1000u, "cleans previous record before switch");
        calls.emplace_back("cleanup");
    }
    std::uint32_t RecordAnimation(std::uint32_t record) override {
        Check(record == 0x1000u || record == 0x1044u,
              "reads record at 68-byte stride");
        calls.emplace_back("record-animation");
        return record == 0x1000u ? 0x11u : 0x22u;
    }
    std::uint32_t HierarchyInterpolator(std::uint32_t found) override {
        Check(found == 0x700u, "reads hierarchy interpolator");
        calls.emplace_back("interpolator");
        return 0x900u;
    }
    void SetCurrentAnimation(std::uint32_t interpolator,
                             std::uint32_t animation) override {
        Check((interpolator == 0x900u && animation == 0x22u) ||
                  (interpolator == 0x2000u && animation == 0x11u) ||
                  (interpolator == 0x3000u && animation == 0x22u),
              "assigns direct or blend snapshot animation");
        calls.emplace_back("set-animation:" +
                           std::to_string(interpolator));
    }
    void SetCurrentTime(std::uint32_t interpolator,
                        float time) override {
        Check(interpolator == 0x900u && time == 0.0f,
              "direct switch rewinds interpolator");
        calls.emplace_back("set-time");
    }
    void UpdateMatrices(std::uint32_t found) override {
        Check(found == 0x700u, "direct switch updates matrices");
        calls.emplace_back("matrices");
    }
};

pal4::GameObjectEmbeddedAnimationState ReadyState() {
    pal4::GameObjectEmbeddedAnimationState state;
    state.words[1] = 0x55u;
    state.words[2] = 0x1000u;
    state.words[3] = 0x1000u;
    state.words[4] = 0x2000u;
    state.words[5] = 0x3000u;
    state.words[10] = 9;
    state.words[15] = 9;
    state.words[18] = 9;
    state.words[19] = 9;
    return state;
}

void AttachHierarchy() {
    Runtime runtime;
    pal4::GameObjectEmbeddedAnimationState state;
    Check(!pal4::GameObjectAnimationSwitch::AttachHierarchyToClump(
              state, 0x66u, runtime) && runtime.calls.empty(),
          "missing source clump short circuits attachment");
    state.words[1] = 0x55u;
    runtime.hierarchy = 0;
    Check(!pal4::GameObjectAnimationSwitch::AttachHierarchyToClump(
              state, 0x66u, runtime) &&
              runtime.calls == std::vector<std::string>{"hierarchy"},
          "missing hierarchy stops before mutation");
    runtime.calls.clear();
    runtime.hierarchy = 0x700u;
    Check(pal4::GameObjectAnimationSwitch::AttachHierarchyToClump(
              state, 0x66u, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "hierarchy", "flags", "attach", "atomics"},
          "attachment sets flags and binds target atomics in order");
}

void DirectSwitch() {
    Runtime runtime;
    auto state = ReadyState();
    state.words[1] = 0;
    Check(!pal4::GameObjectAnimationSwitch::SetCurrentAnimation(
              state, "run", runtime) && runtime.calls.empty(),
          "direct switch requires clump");
    state.words[1] = 0x55u;
    state.words[2] = 0;
    Check(!pal4::GameObjectAnimationSwitch::SetCurrentAnimation(
              state, "run", runtime) && runtime.calls.empty(),
          "direct switch requires record array");
    state.words[2] = 0x1000u;
    runtime.hierarchy = 0;
    Check(!pal4::GameObjectAnimationSwitch::SetCurrentAnimation(
              state, "run", runtime) &&
              runtime.calls == std::vector<std::string>{"hierarchy"},
          "direct switch requires hierarchy");
    runtime.calls.clear();
    runtime.hierarchy = 0x700u;
    Check(!pal4::GameObjectAnimationSwitch::SetCurrentAnimation(
              state, "missing", runtime) && state.words[3] == 0x1000u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "cleanup") == runtime.calls.end(),
          "unmatched action leaves current record untouched");
    runtime.calls.clear();
    Check(pal4::GameObjectAnimationSwitch::SetCurrentAnimation(
              state, "run", runtime) && state.words[3] == 0x1044u &&
              state.words[10] == 0 && state.words[15] == 0 &&
              state.words[18] == 0 && state.words[19] == 0 &&
              runtime.calls == std::vector<std::string>{
                  "hierarchy", "count", "name:0", "compare", "count",
                  "name:1", "compare", "cleanup", "interpolator",
                  "record-animation", "set-animation:2304", "set-time",
                  "matrices"},
          "direct switch selects second record and resets playback fields");
}

void BlendedSwitch() {
    Runtime runtime;
    auto state = ReadyState();
    state.words[4] = 0;
    Check(!pal4::GameObjectAnimationSwitch::SetCurrentAnimationBlended(
              state, "run", 0.2f, runtime) && runtime.calls.empty(),
          "blend requires both snapshots");
    state.words[4] = 0x2000u;
    runtime.names.clear();
    Check(!pal4::GameObjectAnimationSwitch::SetCurrentAnimationBlended(
              state, "run", 0.2f, runtime) && state.words[3] == 0x1000u,
          "blend with no data leaves current record unchanged");
    runtime.calls.clear();
    runtime.names = {"idle", "run"};
    Check(pal4::GameObjectAnimationSwitch::SetCurrentAnimationBlended(
              state, "run", 0.2f, runtime) &&
              state.words[3] == 0x1044u && state.words[10] == 0 &&
              state.words[15] == 1 && state.words[18] == 0 &&
              state.words[19] == 0 &&
              state.words[13] == std::bit_cast<std::uint32_t>(0.2f) &&
              state.words[14] == state.words[13] &&
              runtime.calls == std::vector<std::string>{
                  "count", "name:0", "compare", "count", "name:1",
                  "compare", "cleanup", "record-animation",
                  "set-animation:8192", "record-animation",
                  "set-animation:12288"},
          "blend loads old and new records before updating current pointer");
}

}  // namespace

int main() {
    AttachHierarchy();
    DirectSwitch();
    BlendedSwitch();
    std::cout << "game_object_animation_switch_tests passed\n";
}
