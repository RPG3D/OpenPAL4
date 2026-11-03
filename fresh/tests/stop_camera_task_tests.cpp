#include "pal4/stop_camera_task.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IStopCameraTaskRuntime {
    std::vector<std::string> calls;
    std::uint32_t parent = 0u;
    void ConstructBase(pal4::StopCameraTaskState& state) override {
        calls.emplace_back("construct"); state.words[1u] = 0x12345678u;
    }
    void DestroyBase(pal4::StopCameraTaskState& state) override {
        Check(state.words[0u] == pal4::StopCameraTask::kVtable,
              "base destructor sees derived vtable");
        calls.emplace_back("destroy");
    }
    std::uint32_t ResolveFacet(std::uint32_t owner,
                                std::string_view name) override {
        Check(owner == 0x5000u && name == "StopCameraTask",
              "owner and facet class name");
        calls.emplace_back("resolve"); return 0x6000u;
    }
    void StopCamera(std::uint32_t facet) override {
        Check(facet == 0x6000u, "stop facet call");
        calls.emplace_back("stop");
    }
    std::uint32_t GetOwnerFrame(std::uint32_t owner) override {
        Check(owner == 0x5000u, "owner frame query");
        calls.emplace_back("frame"); return 0x7000u;
    }
    std::uint32_t GetFrameParent(std::uint32_t frame) override {
        Check(frame == 0x7000u, "RenderWare parent query");
        calls.emplace_back("parent"); return parent;
    }
    void RemoveFrameChild(std::uint32_t frame) override {
        Check(frame == 0x7000u, "RenderWare remove child");
        calls.emplace_back("remove");
    }
};
}  // namespace

int main() {
    pal4::StopCameraTaskState state{};
    state.words.fill(0xdeadbeefu);
    Runtime runtime;
    Check(&pal4::StopCameraTask::Construct(state, runtime) == &state &&
          sizeof(state) == 76u && state.words[0u] == 0x84af68u &&
          state.words[1u] == 0x12345678u && state.words[18u] == 0u &&
          state.words[17u] == 0xdeadbeefu &&
          pal4::StopCameraTask::ClassName() == "StopCameraTask",
          "native layout, constructor and class name");
    state.words[16u] = 0x5000u;
    runtime.calls.clear();
    Check(pal4::StopCameraTask::Start(state, runtime) == 1u &&
          state.words[18u] == 0x6000u &&
          runtime.calls == std::vector<std::string>{
              "resolve", "stop", "frame", "parent"},
          "detached frame skips RenderWare child removal");
    runtime.calls.clear();
    runtime.parent = 0x8000u;
    Check(pal4::StopCameraTask::Start(state, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{
              "resolve", "stop", "frame", "parent", "frame", "remove"},
          "parented frame repeats owner query and removes child");
    state.words[0u] = 0u;
    Check(&pal4::StopCameraTask::Destroy(state, runtime) == &state &&
          state.words[0u] == 0x84af68u &&
          runtime.calls.back() == "destroy",
          "derived vtable restored before base cleanup");
}
