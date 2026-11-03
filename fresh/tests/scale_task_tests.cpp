#include "pal4/scale_task.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
void Put(pal4::ScaleTaskState& state, std::size_t word, float value) {
    state.words[word] = std::bit_cast<std::uint32_t>(value);
}
float Get(const pal4::ScaleTaskState& state, std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}
bool Near(float actual, float expected) {
    return std::fabs(actual - expected) < 0.0001f;
}
void CheckVector(const std::array<float, 3u>& actual,
                 const std::array<float, 3u>& expected) {
    for (std::size_t i = 0; i < 3u; ++i)
        Check(Near(actual[i], expected[i]), "scale component");
}

struct Runtime final : pal4::IScaleTaskRuntime {
    std::vector<std::string> calls;
    std::vector<std::array<float, 3u>> applied;
    std::array<float, 3u> current{2.0f, 4.0f, 6.0f};
    std::int32_t random = 0;
    unsigned random_calls = 0u;
    unsigned base_checks = 0u;
    bool complete = true;
    void ConstructBase(pal4::ScaleTaskState& state) override {
        calls.emplace_back("construct"); state.words[0u] = 0xdeadbeefu;
    }
    void DestroyBase(pal4::ScaleTaskState& state) override {
        Check(state.words[0u] == pal4::ScaleTask::kVtable,
              "base destructor sees derived vtable");
        calls.emplace_back("destroy");
    }
    std::uint32_t ResolveFacet(std::uint32_t owner,
                                std::string_view name) override {
        Check(owner == 0x1000u && name == "ScaleTask", "facet resolution");
        calls.emplace_back("resolve"); return 0x2000u;
    }
    void ReadScale(std::uint32_t facet,
                   std::array<float, 3u>& scale) override {
        Check(facet == 0x2000u, "facet read");
        calls.emplace_back("read"); scale = current;
    }
    void SetScale(std::uint32_t facet,
                  const std::array<float, 3u>& scale,
                  std::uint32_t mode) override {
        Check(facet == 0x2000u && mode == 0u, "absolute scale slot");
        calls.emplace_back("set"); applied.push_back(scale); current = scale;
    }
    bool BaseDurationComplete(const pal4::ScaleTaskState&) override {
        ++base_checks; return complete;
    }
    std::int32_t RandomInt() override { ++random_calls; return random; }
};

pal4::ScaleTaskState State(bool result, bool rollback) {
    pal4::ScaleTaskState state{};
    state.words[16u] = 0x1000u;
    state.words[18u] = result ? 1u : 0u;
    state.words[26u] = rollback ? 1u : 0u;
    Put(state, 5u, 2.0f);
    Put(state, 22u, 10.0f); Put(state, 23u, 20.0f);
    Put(state, 24u, 30.0f);
    Put(state, 31u, 1.0f); Put(state, 34u, 2.0f);
    return state;
}

}  // namespace

int main() {
    Runtime runtime;
    auto constructed = State(false, false);
    constructed.words[25u] = 0xffffffffu;
    (void)pal4::ScaleTask::Construct(constructed, runtime);
    Check(sizeof(constructed) == 156u &&
          constructed.words[0u] == pal4::ScaleTask::kVtable &&
          constructed.words[25u] == 0u && constructed.words[22u] != 0u &&
          pal4::ScaleTask::ClassName() == "ScaleTask",
          "constructor layout and native class");
    (void)pal4::ScaleTask::Destroy(constructed, runtime);
    Check(runtime.calls == std::vector<std::string>{"construct", "destroy"},
          "base lifecycle order");

    auto result = State(true, false);
    runtime.calls.clear();
    Check(pal4::ScaleTask::IsDurationComplete(result, runtime) &&
          runtime.base_checks == 1u, "base duration completion");
    runtime.complete = false;
    Check(!pal4::ScaleTask::IsDurationComplete(result, runtime),
          "base incomplete result");
    runtime.complete = true;
    result.words[26u] = 1u;
    Check(!pal4::ScaleTask::IsDurationComplete(result, runtime) &&
          runtime.base_checks == 2u, "rollback short circuits base");
    result.words[26u] = 0u;
    Check(pal4::ScaleTask::Start(result, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{"resolve", "read"} &&
          result.words[25u] == 0x2000u && Near(Get(result, 28u), 2.0f),
          "result start resolves and reads facet");
    Check(Near(Get(result, 19u), 4.0f) &&
          Near(Get(result, 20u), 8.0f) && Near(Get(result, 21u), 12.0f),
          "result velocity derives from origin");
    (void)pal4::ScaleTask::Update(result, 0.5f, runtime);
    Check(Near(Get(result, 27u), 0.5f), "result elapsed advances");
    CheckVector(runtime.applied.back(), {4.0f, 8.0f, 12.0f});
    (void)pal4::ScaleTask::Update(result, 3.0f, runtime);
    Check(Near(Get(result, 27u), 2.0f), "result step clips to duration");
    CheckVector(runtime.applied.back(), {10.0f, 20.0f, 30.0f});
    (void)pal4::ScaleTask::Stop(result, runtime);
    Check(result.words[25u] == 0u, "stop clears facet");
    CheckVector(runtime.applied.back(), {10.0f, 20.0f, 30.0f});

    runtime = Runtime{};
    auto periodic = State(true, true);
    (void)pal4::ScaleTask::Start(periodic, runtime);
    Check(runtime.random_calls == 1u, "rollback start samples CRT rand");
    (void)pal4::ScaleTask::Update(periodic, 2.0f, runtime);
    Check((periodic.words[26u] & 0xff00u) == 0x100u &&
          (periodic.words[30u] & 0xffu) == 1u &&
          Near(Get(periodic, 19u), -4.0f),
          "forward boundary reverses velocity and enables wait one");
    Check(runtime.applied.size() == 2u, "result boundary snaps target");
    const auto before_wait = runtime.applied.size();
    (void)pal4::ScaleTask::Update(periodic, 0.5f, runtime);
    (void)pal4::ScaleTask::Update(periodic, 0.5f, runtime);
    Check(runtime.applied.size() == before_wait &&
          Near(Get(periodic, 32u), 1.0f), "first wait preincrement comparison");
    (void)pal4::ScaleTask::Update(periodic, 0.25f, runtime);
    Check(runtime.applied.size() == before_wait + 1u &&
          Near(Get(periodic, 27u), 0.25f), "reverse starts after wait");
    CheckVector(runtime.applied.back(), {9.0f, 18.0f, 27.0f});
    (void)pal4::ScaleTask::Update(periodic, 1.75f, runtime);
    Check(runtime.random_calls == 2u &&
          (periodic.words[26u] & 0xff00u) == 0u &&
          (periodic.words[33u] & 0xffu) == 1u &&
          Near(Get(periodic, 19u), 4.0f),
          "reverse boundary restores origin and rerolls duration");
    CheckVector(runtime.applied.back(), {2.0f, 4.0f, 6.0f});
    const auto before_second_wait = runtime.applied.size();
    (void)pal4::ScaleTask::Update(periodic, 1.0f, runtime);
    (void)pal4::ScaleTask::Update(periodic, 1.0f, runtime);
    Check(runtime.applied.size() == before_second_wait &&
          Near(Get(periodic, 35u), 2.0f), "second interval waits");
    (void)pal4::ScaleTask::Update(periodic, 0.5f, runtime);
    Check(runtime.applied.size() == before_second_wait + 1u &&
          (periodic.words[33u] & 0xffu) == 0u,
          "second wait expires and forward motion resumes");

    runtime = Runtime{};
    auto velocity = State(false, false);
    (void)pal4::ScaleTask::Start(velocity, runtime);
    Check(Near(Get(velocity, 19u), 10.0f), "velocity mode copies target");
    (void)pal4::ScaleTask::Update(velocity, 0.5f, runtime);
    CheckVector(runtime.applied.back(), {7.0f, 14.0f, 21.0f});
    Check(velocity.words[27u] == 0u, "unbounded velocity leaves timer");
    const auto applied = runtime.applied.size();
    (void)pal4::ScaleTask::Stop(velocity, runtime);
    Check(runtime.applied.size() == applied && velocity.words[25u] == 0u,
          "velocity stop does not snap");

    runtime = Runtime{};
    auto biased = State(true, true);
    Put(biased, 29u, 1.0f);
    runtime.random = 32767;
    (void)pal4::ScaleTask::Start(biased, runtime);
    Check(Near(Get(biased, 28u), 3.0f) &&
          Near(Get(biased, 19u), 8.0f / 3.0f),
          "maximum CRT random applies positive bias");
}
