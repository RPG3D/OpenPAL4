#include "pal4/start_camera_task.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IStartCameraTaskRuntime {
    std::vector<std::string> calls;
    void ConstructBase(pal4::StartCameraTaskState& state) override {
        calls.emplace_back("construct"); state.words[1u] = 0x12345678u;
    }
    void DestroyBase(pal4::StartCameraTaskState& state) override {
        Check(state.words[0u] == pal4::StartCameraTask::kVtable,
              "base sees camera task vtable");
        calls.emplace_back("destroy");
    }
    std::uint32_t ResolveFacet(std::uint32_t owner,
                                std::string_view name) override {
        Check(owner == 0x5000u && name == "StartCameraTask",
              "owner and class facet name");
        calls.emplace_back("resolve"); return 0x6000u;
    }
    void StartCamera(std::uint32_t facet) override {
        Check(facet == 0x6000u, "facet start dispatch");
        calls.emplace_back("start");
    }
};
}  // namespace

int main() {
    pal4::StartCameraTaskState state{};
    state.words.fill(0xdeadbeefu);
    Runtime runtime;
    Check(&pal4::StartCameraTask::Construct(state, runtime) == &state &&
          sizeof(state) == 76u && state.words[0u] == 0x84af28u &&
          state.words[1u] == 0x12345678u && state.words[18u] == 0u &&
          state.words[17u] == 0xdeadbeefu &&
          pal4::StartCameraTask::ClassName() == "StartCameraTask",
          "constructor layout and native name");
    state.words[16u] = 0x5000u;
    Check(pal4::StartCameraTask::Start(state, runtime) == 1u &&
          state.words[18u] == 0x6000u &&
          runtime.calls == std::vector<std::string>{
              "construct", "resolve", "start"},
          "start resolves and activates facet");
    state.words[0u] = 0u;
    Check(&pal4::StartCameraTask::Destroy(state, runtime) == &state &&
          state.words[0u] == 0x84af28u &&
          runtime.calls.back() == "destroy",
          "derived vtable restored before base cleanup");
}
