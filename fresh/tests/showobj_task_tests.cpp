#include "pal4/showobj_task.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IShowobjTaskRuntime {
    std::vector<std::string> calls;
    void ConstructBase(pal4::ShowobjTaskState& state) override {
        calls.emplace_back("construct"); state.words[1u] = 0x12345678u;
    }
    void DestroyBase(pal4::ShowobjTaskState& state) override {
        Check(state.words[0u] == pal4::ShowobjTask::kVtable,
              "base destructor sees derived vtable");
        calls.emplace_back("destroy");
    }
    std::uint32_t ResolveFacet(std::uint32_t owner,
                                std::string_view name) override {
        Check(owner == 0x5000u && name == "ShowobjTask",
              "owner and native facet name");
        calls.emplace_back("resolve"); return 0x6000u;
    }
    void Show(std::uint32_t facet) override {
        Check(facet == 0x6000u, "show dispatches resolved facet");
        calls.emplace_back("show");
    }
};
}  // namespace

int main() {
    pal4::ShowobjTaskState state{};
    state.words.fill(0xdeadbeefu);
    Runtime runtime;
    Check(&pal4::ShowobjTask::Construct(state, runtime) == &state &&
          sizeof(state) == 76u && state.words[0u] == 0x84aee8u &&
          state.words[1u] == 0x12345678u && state.words[18u] == 0u &&
          state.words[17u] == 0xdeadbeefu &&
          pal4::ShowobjTask::ClassName() == "ShowobjTask",
          "native layout and constructor");
    state.words[16u] = 0x5000u;
    Check(pal4::ShowobjTask::Start(state, runtime) == 1u &&
          state.words[18u] == 0x6000u &&
          runtime.calls == std::vector<std::string>{
              "construct", "resolve", "show"},
          "start resolves and shows facet");
    state.words[0u] = 0u;
    Check(&pal4::ShowobjTask::Destroy(state, runtime) == &state &&
          state.words[0u] == 0x84aee8u &&
          runtime.calls.back() == "destroy",
          "derived cleanup precedes base destructor");
}
