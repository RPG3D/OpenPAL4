#include "pal4/stop_sound3d_task.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IStopSound3DTaskRuntime {
    std::vector<std::string> calls;
    std::uint32_t facet = 0x6000u;
    void ConstructBase(pal4::StopSound3DTaskState& state) override {
        calls.emplace_back("construct"); state.words[1u] = 0x12345678u;
    }
    void DestroyBase(pal4::StopSound3DTaskState& state) override {
        Check(state.words[0u] == pal4::StopSound3DTask::kVtable,
              "base destructor sees derived vtable");
        calls.emplace_back("destroy");
    }
    std::uint32_t ResolveFacet(std::uint32_t owner,
                                std::string_view name) override {
        Check(owner == 0x5000u && name == "StopSound3DTask",
              "owner and native facet class");
        calls.emplace_back("resolve"); return facet;
    }
    void StopSound(std::uint32_t value) override {
        Check(value == facet, "slot +8 dispatches resolved facet");
        calls.emplace_back("stop");
    }
};
}  // namespace

int main() {
    pal4::StopSound3DTaskState state{};
    state.words.fill(0xdeadbeefu);
    Runtime runtime;
    Check(&pal4::StopSound3DTask::Construct(state, runtime) == &state &&
          sizeof(state) == 76u && state.words[0u] == 0x84afe8u &&
          state.words[1u] == 0x12345678u && state.words[18u] == 0u &&
          state.words[17u] == 0xdeadbeefu &&
          pal4::StopSound3DTask::ClassName() == "StopSound3DTask",
          "native size, constructor and class name");
    state.words[16u] = 0x5000u;
    runtime.calls.clear();
    Check(pal4::StopSound3DTask::Start(state, runtime) == 1u &&
          state.words[18u] == 0x6000u &&
          runtime.calls == std::vector<std::string>{"resolve", "stop"},
          "start resolves and stops sound facet");
    runtime.calls.clear();
    runtime.facet = 0u;
    Check(pal4::StopSound3DTask::Start(state, runtime) == 1u &&
          state.words[18u] == 0u &&
          runtime.calls == std::vector<std::string>{"resolve", "stop"},
          "null facet follows native unconditional dispatch");
    state.words[0u] = 0u;
    runtime.calls.clear();
    Check(&pal4::StopSound3DTask::Destroy(state, runtime) == &state &&
          state.words[0u] == 0x84afe8u &&
          runtime.calls == std::vector<std::string>{"destroy"},
          "derived vtable restored before base cleanup");
}
