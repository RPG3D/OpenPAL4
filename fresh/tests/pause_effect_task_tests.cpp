#include "pal4/pause_effect_task.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPauseEffectTaskRuntime {
    std::vector<std::string> calls;
    std::uint32_t resolved = 0x6000u;

    void ConstructBase(pal4::PauseEffectTaskState& state) override {
        calls.emplace_back("construct");
        state.words[0u] = 0xdeadbeefu;
    }
    void DestroyBase(pal4::PauseEffectTaskState& state) override {
        Check(state.words[0u] == pal4::PauseEffectTask::kVtable,
              "base destructor sees derived vtable");
        calls.emplace_back("destroy");
    }
    std::uint32_t ResolveFacet(std::uint32_t object,
                                std::string_view name) override {
        Check(object == 0x5000u && name == "PauseEffectTask",
              "resolve class facet through owner +64");
        calls.emplace_back("resolve");
        return resolved;
    }
    void Pause(std::uint32_t facet) override {
        Check(facet == resolved, "pause receives resolved facet");
        calls.emplace_back("pause");
    }
};

}  // namespace

int main() {
    pal4::PauseEffectTaskState state{};
    state.words.fill(0xabcdef01u);
    Runtime runtime;
    Check(&pal4::PauseEffectTask::Construct(state, runtime) == &state &&
              state.words[0u] == pal4::PauseEffectTask::kVtable &&
              state.words[18u] == 0u &&
              state.words[17u] == 0xabcdef01u &&
              runtime.calls == std::vector<std::string>{"construct"},
          "constructor clears only facet after base construction");
    Check(pal4::PauseEffectTask::ClassName() == "PauseEffectTask",
          "class name matches IDA literal");

    state.words[16u] = 0x5000u;
    runtime.calls.clear();
    Check(pal4::PauseEffectTask::Start(state, runtime) == 1u &&
              state.words[18u] == 0x6000u &&
              runtime.calls == std::vector<std::string>{"resolve", "pause"},
          "start resolves, stores and pauses facet");

    runtime.resolved = 0u;
    runtime.calls.clear();
    Check(pal4::PauseEffectTask::Start(state, runtime) == 1u &&
              state.words[18u] == 0u &&
              runtime.calls == std::vector<std::string>{"resolve", "pause"},
          "native start dispatches even when lookup yields null");

    runtime.calls.clear();
    Check(&pal4::PauseEffectTask::Destroy(state, runtime) == &state &&
              runtime.calls == std::vector<std::string>{"destroy"},
          "destructor restores vtable before base cleanup");
}
