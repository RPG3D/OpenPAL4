#include "pal4/effect_rotate_task.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}
void Put(pal4::EffectRotateTaskState& state, std::size_t word, float value) {
    state.words[word] = std::bit_cast<std::uint32_t>(value);
}
float Get(const pal4::EffectRotateTaskState& state, std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}
bool Near(float actual, float expected) {
    return std::fabs(actual - expected) < 0.0001f;
}
void CheckVector(const std::array<float, 3u>& actual,
                 const std::array<float, 3u>& expected) {
    for (std::size_t i = 0; i < 3u; ++i)
        Check(Near(actual[i], expected[i]), "rotation vector component");
}

struct Apply final {
    std::uint32_t facet;
    std::array<float, 3u> value;
    bool relative;
};

struct Runtime final : pal4::IEffectRotateTaskRuntime {
    std::vector<std::string> calls;
    std::vector<Apply> applied;
    std::array<float, 3u> initial_rotation{2.0f, 4.0f, 6.0f};
    std::uint32_t camera = 0x4000u;
    bool complete = true;
    bool locked = false;
    std::int32_t random_value = 0;
    unsigned random_calls = 0u;
    unsigned base_checks = 0u;
    unsigned free_calls = 0u;

    void ConstructBase(pal4::EffectRotateTaskState& state) override {
        calls.emplace_back("construct");
        state.words[0u] = 0xdeadbeefu;
    }
    void DestroyBase(pal4::EffectRotateTaskState& state) override {
        Check(state.words[0u] == pal4::EffectRotateTask::kVtable,
              "base destructor sees derived vtable");
        calls.emplace_back("destroy");
    }
    void Free(pal4::EffectRotateTaskState&) override {
        calls.emplace_back("free"); ++free_calls;
    }
    std::uint32_t ResolveFacet(std::uint32_t owner,
                                std::string_view name) override {
        Check(owner == 0x2000u, "facet owner");
        calls.emplace_back("resolve:" + std::string(name));
        return name == "RotateTask" ? 0x3000u : camera;
    }
    void ReadRotation(std::uint32_t facet,
                      std::array<float, 3u>& rotation) override {
        Check(facet == 0x3000u, "rotation facet read");
        calls.emplace_back("read"); rotation = initial_rotation;
    }
    void ApplyRotation(std::uint32_t facet,
                       const std::array<float, 3u>& value,
                       bool relative) override {
        Check(facet == 0x3000u, "rotation facet write");
        calls.emplace_back(relative ? "relative" : "absolute");
        applied.push_back({facet, value, relative});
    }
    bool BaseDurationComplete(const pal4::EffectRotateTaskState&) override {
        ++base_checks; return complete;
    }
    std::int32_t RandomInt() override {
        ++random_calls; calls.emplace_back("random"); return random_value;
    }
    void SetCameraLockedLookAt(std::uint32_t facet, bool value) override {
        Check(facet == camera, "camera facet");
        locked = value; calls.emplace_back("camera-lock");
    }
};

pal4::EffectRotateTaskState BaseState(bool result, bool rollback,
                                       bool add_velocity = false) {
    pal4::EffectRotateTaskState state{};
    state.words[16u] = 0x2000u;
    Put(state, 5u, 2.0f);
    Put(state, 20u, 10.0f);
    Put(state, 21u, 20.0f);
    Put(state, 22u, 30.0f);
    Put(state, 23u, 11.0f);
    Put(state, 24u, 22.0f);
    Put(state, 25u, 33.0f);
    state.words[18u] = (result ? 1u : 0u) |
                       (add_velocity ? 0x100u : 0u) | 0x10000u;
    state.words[33u] = rollback ? 1u : 0u;
    Put(state, 36u, 0.0f);
    Put(state, 38u, 1.0f);
    Put(state, 41u, 2.0f);
    return state;
}

}  // namespace

int main() {
    Runtime runtime;
    pal4::EffectRotateTaskState constructed{};
    constructed.words.fill(0xabcdef01u);
    Check(&pal4::EffectRotateTask::Construct(constructed, runtime) == &constructed &&
          constructed.words[0u] == pal4::EffectRotateTask::kVtable &&
          constructed.words[19u] == 0u && constructed.words[32u] == 0u &&
          constructed.words[18u] == 0xabcdef00u &&
          constructed.words[20u] == 0xabcdef01u &&
          runtime.calls == std::vector<std::string>{"construct"},
          "constructor order, cleared fields and preserved bytes");
    Check(pal4::EffectRotateTask::ClassName() == "RotateTask",
          "native class name distinguishes C++ types");
    runtime.calls.clear();
    constructed.words[16u] = 0x2000u;
    Check(&pal4::EffectRotateTask::Delete(constructed, 0u, runtime) ==
              &constructed &&
          runtime.calls == std::vector<std::string>{"destroy"} &&
          runtime.free_calls == 0u, "deleting destructor without free flag");
    runtime.calls.clear();
    Check(&pal4::EffectRotateTask::Delete(constructed, 3u, runtime) ==
              &constructed &&
          runtime.calls == std::vector<std::string>{"destroy", "free"} &&
          runtime.free_calls == 1u, "deleting destructor honors low flag bit");

    auto state = BaseState(true, false);
    runtime.calls.clear();
    Check(!pal4::EffectRotateTask::IsDurationComplete(
              BaseState(false, true), runtime) && runtime.base_checks == 0u,
          "rollback suppresses base completion query");
    Check(pal4::EffectRotateTask::IsDurationComplete(state, runtime) &&
          runtime.base_checks == 1u, "nonrollback delegates completion");
    runtime.complete = false;
    Check(!pal4::EffectRotateTask::IsDurationComplete(state, runtime),
          "base incomplete result propagates");

    runtime.calls.clear();
    Check(pal4::EffectRotateTask::Start(state, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{"resolve:RotateTask", "read"} &&
          state.words[32u] == 0x3000u && Near(Get(state, 35u), 2.0f) &&
          Near(Get(state, 26u), 4.0f) && Near(Get(state, 27u), 8.0f) &&
          Near(Get(state, 28u), 12.0f) && state.words[34u] == 0u &&
          Near(Get(state, 43u), 2.0f),
          "result start derives velocity from facet rotation");
    runtime.applied.clear();
    Check(pal4::EffectRotateTask::Update(state, 0.5f, runtime) == 1u &&
          Near(Get(state, 34u), 0.5f) && runtime.applied.size() == 1u &&
          runtime.applied.back().relative, "result mode advances elapsed time");
    CheckVector(runtime.applied.back().value, {2, 4, 6});
    Check(pal4::EffectRotateTask::Update(state, 2.0f, runtime) == 1u &&
          Near(Get(state, 34u), 2.0f), "result mode clamps to duration");
    CheckVector(runtime.applied.back().value, {6, 12, 18});
    runtime.calls.clear();
    runtime.applied.clear();
    Check(pal4::EffectRotateTask::Stop(state, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{
              "absolute", "resolve:StartCameraTask", "camera-lock"} &&
          runtime.locked && state.words[32u] == 0u,
          "stop applies configured absolute rotation and camera flag");
    CheckVector(runtime.applied.back().value, {11, 22, 33});
    runtime.camera = 0u;
    state.words[32u] = 0x3000u;
    runtime.calls.clear();
    (void)pal4::EffectRotateTask::Stop(state, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "absolute", "resolve:StartCameraTask"},
          "missing camera skips slot 20");
    runtime.camera = 0x4000u;

    auto periodic = BaseState(true, true);
    runtime.applied.clear();
    runtime.random_calls = 0u;
    Check(pal4::EffectRotateTask::Start(periodic, runtime) == 1u &&
          runtime.random_calls == 1u && Near(Get(periodic, 35u), 2.0f),
          "rollback start randomizes duration even for zero bias");
    runtime.applied.clear();
    (void)pal4::EffectRotateTask::Update(periodic, 1.0f, runtime);
    Check(runtime.applied.size() == 1u, "first forward step");
    CheckVector(runtime.applied.back().value, {4, 8, 12});
    (void)pal4::EffectRotateTask::Update(periodic, 1.0f, runtime);
    Check(runtime.applied.size() == 3u && !runtime.applied.back().relative &&
          (periodic.words[33u] & 0xff00u) == 0x100u &&
          (periodic.words[37u] & 0xffu) == 1u &&
          Near(Get(periodic, 26u), -4.0f) && periodic.words[34u] == 0u,
          "forward completion snaps target and starts interval one");
    CheckVector(runtime.applied.back().value, {10, 20, 30});
    const auto applications = runtime.applied.size();
    (void)pal4::EffectRotateTask::Update(periodic, 0.5f, runtime);
    (void)pal4::EffectRotateTask::Update(periodic, 0.5f, runtime);
    Check(runtime.applied.size() == applications &&
          Near(Get(periodic, 39u), 1.0f),
          "first interval waits using pre-increment comparison");
    (void)pal4::EffectRotateTask::Update(periodic, 0.25f, runtime);
    Check(runtime.applied.size() == applications + 1u &&
          (periodic.words[37u] & 0xffu) == 0u &&
          Near(Get(periodic, 34u), 0.25f),
          "expired first interval resumes reverse motion");
    CheckVector(runtime.applied.back().value, {-1, -2, -3});
    (void)pal4::EffectRotateTask::Update(periodic, 1.75f, runtime);
    Check(runtime.random_calls == 2u &&
          (periodic.words[33u] & 0xff00u) == 0u &&
          (periodic.words[40u] & 0xffu) == 1u &&
          Near(Get(periodic, 26u), 4.0f) &&
          !runtime.applied.back().relative,
          "reverse completion restores origin and starts interval two");
    CheckVector(runtime.applied.back().value, {2, 4, 6});
    const auto reverse_applications = runtime.applied.size();
    (void)pal4::EffectRotateTask::Update(periodic, 1.0f, runtime);
    (void)pal4::EffectRotateTask::Update(periodic, 1.0f, runtime);
    Check(runtime.applied.size() == reverse_applications &&
          Near(Get(periodic, 42u), 2.0f), "second interval waits");
    (void)pal4::EffectRotateTask::Update(periodic, 0.5f, runtime);
    Check(runtime.applied.size() == reverse_applications + 1u &&
          (periodic.words[40u] & 0xffu) == 0u,
          "second interval expires and motion resumes");
    CheckVector(runtime.applied.back().value, {2, 4, 6});

    auto biased = BaseState(true, true);
    Put(biased, 36u, 1.0f);
    runtime.random_value = 32767;
    (void)pal4::EffectRotateTask::Start(biased, runtime);
    Check(Near(Get(biased, 35u), 3.0f) &&
          Near(Get(biased, 26u), 8.0f / 3.0f),
          "maximum CRT random value applies positive time bias");
    runtime.random_value = 0;

    auto periodic_velocity = BaseState(false, true, true);
    Put(periodic_velocity, 20u, 1.0f);
    Put(periodic_velocity, 21u, 2.0f);
    Put(periodic_velocity, 22u, 3.0f);
    Put(periodic_velocity, 23u, 3.0f);
    Put(periodic_velocity, 24u, 4.0f);
    Put(periodic_velocity, 25u, 5.0f);
    (void)pal4::EffectRotateTask::Start(periodic_velocity, runtime);
    runtime.applied.clear();
    (void)pal4::EffectRotateTask::Update(periodic_velocity, 2.0f, runtime);
    Check(runtime.applied.size() == 1u &&
          runtime.applied.back().relative &&
          Near(Get(periodic_velocity, 26u), -3.0f) &&
          (periodic_velocity.words[37u] & 0xffu) == 1u,
          "rollback velocity mode integrates and reverses without target snap");
    CheckVector(runtime.applied.back().value, {6, 8, 10});

    auto velocity = BaseState(false, false, true);
    Put(velocity, 20u, 1.0f); Put(velocity, 21u, 2.0f);
    Put(velocity, 22u, 3.0f); Put(velocity, 23u, 3.0f);
    Put(velocity, 24u, 4.0f); Put(velocity, 25u, 5.0f);
    (void)pal4::EffectRotateTask::Start(velocity, runtime);
    Check(Near(Get(velocity, 26u), 1.0f) &&
          Near(Get(velocity, 29u), 1.0f),
          "velocity start derives acceleration");
    runtime.applied.clear();
    (void)pal4::EffectRotateTask::Update(velocity, 0.5f, runtime);
    CheckVector(runtime.applied.back().value, {0.75f, 1.25f, 1.75f});
    Check(velocity.words[34u] == 0u, "unbounded velocity leaves elapsed unchanged");
    velocity.words[32u] = 0x3000u;
    velocity.words[18u] &= 0xffff0000u;
    runtime.calls.clear();
    runtime.applied.clear();
    (void)pal4::EffectRotateTask::Stop(velocity, runtime);
    Check(runtime.applied.empty() && velocity.words[32u] == 0u,
          "stop without result flag does not snap rotation");
}
