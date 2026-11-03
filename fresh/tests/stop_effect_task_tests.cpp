#include "pal4/stop_effect_task.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
void Put(pal4::StopEffectTaskState& state, std::size_t word, float value) {
    state.words[word] = std::bit_cast<std::uint32_t>(value);
}
float Get(const pal4::StopEffectTaskState& state, std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}
struct Runtime final : pal4::IStopEffectTaskRuntime {
    std::vector<std::string> calls;
    float remaining = 1.5f;
    bool complete = true;
    unsigned base_checks = 0u;
    void ConstructBase(pal4::StopEffectTaskState& state) override {
        calls.emplace_back("construct"); state.words[1u] = 0x12345678u;
    }
    void DestroyBase(pal4::StopEffectTaskState& state) override {
        Check(state.words[0u] == pal4::StopEffectTask::kVtable,
              "base destructor sees derived vtable");
        calls.emplace_back("destroy");
    }
    std::uint32_t ResolveFacet(std::uint32_t owner,
                                std::string_view name) override {
        Check(owner == 0x5000u && name == "StopEffectTask",
              "facet owner and class name");
        calls.emplace_back("resolve"); return 0x6000u;
    }
    float RemainingEffectTime(std::uint32_t facet) override {
        Check(facet == 0x6000u, "remaining time facet");
        calls.emplace_back("remaining"); return remaining;
    }
    void StopEffect(std::uint32_t facet) override {
        Check(facet == 0x6000u, "stop facet slot");
        calls.emplace_back("stop");
    }
    bool BaseDurationComplete(const pal4::StopEffectTaskState&) override {
        ++base_checks; return complete;
    }
};
}  // namespace

int main() {
    pal4::StopEffectTaskState state{};
    state.words.fill(0xdeadbeefu);
    Runtime runtime;
    Check(&pal4::StopEffectTask::Construct(state, runtime) == &state &&
          sizeof(state) == 92u && state.words[0u] == 0x84afa8u &&
          state.words[1u] == 0x12345678u && state.words[22u] == 0u &&
          state.words[21u] == 0xdeadbeefu &&
          pal4::StopEffectTask::ClassName() == "StopEffectTask",
          "native size, vtable and constructor fields");
    state.words[16u] = 0x5000u;
    state.words[18u] = 1u;
    runtime.calls.clear();
    Check(pal4::StopEffectTask::Start(state, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{"resolve", "remaining"} &&
          state.words[22u] == 0x6000u && Get(state, 19u) == 1.5f &&
          state.words[20u] == 0u && (state.words[21u] & 0xffu) == 0u,
          "smooth start captures duration and clears completion state");
    Check(!pal4::StopEffectTask::IsDurationComplete(state, runtime) &&
          runtime.base_checks == 0u,
          "smooth completion uses local done byte");
    Check(pal4::StopEffectTask::Update(state, 0.5f, runtime) == 1u &&
          Get(state, 20u) == 0.5f && runtime.calls.size() == 2u,
          "smooth update accumulates below threshold");
    Check(pal4::StopEffectTask::Update(state, 1.0f, runtime) == 1u &&
          Get(state, 20u) == 1.5f && (state.words[21u] & 0xffu) == 1u &&
          runtime.calls == std::vector<std::string>{
              "resolve", "remaining", "stop"},
          "threshold equality stops exactly once");
    Check(pal4::StopEffectTask::IsDurationComplete(state, runtime) &&
          runtime.base_checks == 0u, "smooth complete uses done byte");
    (void)pal4::StopEffectTask::Update(state, 4.0f, runtime);
    Check(Get(state, 20u) == 1.5f && runtime.calls.size() == 3u,
          "completed update leaves timer and facet alone");
    Check(pal4::StopEffectTask::Stop(state, runtime) == 1u &&
          state.words[22u] == 0u && (state.words[21u] & 0xffu) == 1u &&
          runtime.calls.back() == "stop",
          "stop dispatches and clears facet");

    pal4::StopEffectTaskState immediate{};
    immediate.words[16u] = 0x5000u;
    Put(immediate, 19u, 7.0f);
    Put(immediate, 20u, 2.0f);
    runtime.calls.clear();
    runtime.complete = false;
    Check(!pal4::StopEffectTask::IsDurationComplete(immediate, runtime) &&
          runtime.base_checks == 1u, "non-smooth delegates incomplete base");
    runtime.complete = true;
    Check(pal4::StopEffectTask::IsDurationComplete(immediate, runtime) &&
          runtime.base_checks == 2u, "non-smooth delegates complete base");
    Check(pal4::StopEffectTask::Start(immediate, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{"resolve", "stop"} &&
          Get(immediate, 19u) == 7.0f && Get(immediate, 20u) == 2.0f,
          "non-smooth start stops immediately without timer reset");
    (void)pal4::StopEffectTask::Update(immediate, 1.0f, runtime);
    Check(runtime.calls.size() == 2u && Get(immediate, 20u) == 2.0f,
          "non-smooth update is inert");

    runtime.calls.clear();
    state.words[0u] = 0u;
    Check(&pal4::StopEffectTask::Destroy(state, runtime) == &state &&
          state.words[0u] == 0x84afa8u &&
          runtime.calls == std::vector<std::string>{"destroy"},
          "derived vtable restored before base cleanup");
}
