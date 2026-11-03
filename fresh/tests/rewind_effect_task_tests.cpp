#include "pal4/rewind_effect_task.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}

struct Runtime final : pal4::IRewindEffectTaskRuntime {
    std::vector<std::string> calls;
    std::uint32_t facet = 0x6000u;

    void ConstructBase(pal4::RewindEffectTaskState& state) override {
        calls.emplace_back("base-construct");
        state.words[0u] = 0xdeadbeefu;
        state.words[18u] = 0x12345678u;
    }
    void DestroyBase(pal4::RewindEffectTaskState& state) override {
        Check(state.words[0u] == pal4::RewindEffectTask::kVtable,
              "base destructor sees derived vtable");
        calls.emplace_back("base-destroy");
    }
    std::uint32_t ResolveFacet(std::uint32_t owner,
                                std::string_view name) override {
        Check(owner == 0x5000u && name == "RewindEffectTask",
              "lookup uses owner and native class name");
        calls.emplace_back("resolve");
        return facet;
    }
    void Rewind(std::uint32_t value) override {
        Check(value == facet, "rewind uses resolved facet");
        calls.emplace_back("rewind");
    }
};

}  // namespace

int main() {
    pal4::RewindEffectTaskState state{};
    state.words.fill(0xabcdef01u);
    Runtime runtime;
    Check(&pal4::RewindEffectTask::Construct(state, runtime) == &state &&
          state.words[0u] == pal4::RewindEffectTask::kVtable &&
          state.words[18u] == 0u && state.words[17u] == 0xabcdef01u &&
          runtime.calls == std::vector<std::string>{"base-construct"},
          "constructor clears facet after base and preserves other fields");
    Check(pal4::RewindEffectTask::ClassName() == "RewindEffectTask",
          "IDA string at 0x8c5848");

    state.words[16u] = 0x5000u;
    runtime.calls.clear();
    Check(pal4::RewindEffectTask::Start(state, runtime) == 1u &&
          state.words[18u] == 0x6000u &&
          runtime.calls == std::vector<std::string>{"resolve", "rewind"},
          "start stores facet and dispatches virtual slot 12");

    runtime.facet = 0u;
    runtime.calls.clear();
    Check(pal4::RewindEffectTask::Start(state, runtime) == 1u &&
          state.words[18u] == 0u &&
          runtime.calls == std::vector<std::string>{"resolve", "rewind"},
          "native path dispatches even when lookup returns null");

    runtime.calls.clear();
    Check(&pal4::RewindEffectTask::Destroy(state, runtime) == &state &&
          runtime.calls == std::vector<std::string>{"base-destroy"},
          "destructor restores vtable before base cleanup");
}
