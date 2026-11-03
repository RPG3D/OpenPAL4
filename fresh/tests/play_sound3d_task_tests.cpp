#include "pal4/play_sound3d_task.h"

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

struct Runtime final : pal4::IPlaySound3DTaskRuntime,
                       pal4::IPlaySound3DConfigRuntime {
    std::vector<std::string> calls;
    std::int32_t loop = 0;
    std::uint32_t facet = 0x6000u;
    std::uint8_t non_looping = 0xffu;

    void ConstructBase(pal4::PlaySound3DTaskState& state) override {
        calls.emplace_back("construct");
        state.words[0u] = 0xdeadbeefu;
    }
    void DestroyBase(pal4::PlaySound3DTaskState& state) override {
        Check(state.words[0u] == pal4::PlaySound3DTask::kVtable,
              "base cleanup sees derived vtable");
        calls.emplace_back("destroy");
    }
    std::uint32_t ResolveFacet(std::uint32_t object,
                                std::string_view name) override {
        Check(object == 0x5000u && name == "PlaySound3DTask",
              "facet lookup uses owner and class name");
        calls.emplace_back("resolve");
        return facet;
    }
    void StartSound(std::uint32_t resolved,
                    std::uint8_t flag) override {
        Check(resolved == facet, "sound dispatch uses resolved facet");
        calls.emplace_back("start");
        non_looping = flag;
    }
    std::int32_t ReadInt(std::uint32_t config,
                         std::string_view key,
                         std::int32_t fallback) override {
        Check(config == 0x7000u &&
                  key == "Attr_Task_PlaySound3D_Loop" && fallback == 0,
              "loop XML key and zero default");
        calls.emplace_back("read-loop");
        return loop;
    }
};

}  // namespace

int main() {
    pal4::PlaySound3DTaskState state{};
    state.words.fill(0xabcdef01u);
    Runtime runtime;
    Check(&pal4::PlaySound3DTask::Construct(state, runtime) == &state &&
              state.words[0u] == pal4::PlaySound3DTask::kVtable &&
              state.words[18u] == 0xabcdef00u &&
              state.words[19u] == 0u &&
              runtime.calls == std::vector<std::string>{"construct"},
          "constructor clears loop byte and facet after base call");
    Check(pal4::PlaySound3DTask::ClassName() == "PlaySound3DTask",
          "class name matches native string");

    runtime.calls.clear();
    Check(!pal4::PlaySound3DTask::LoadConfig(state, 0u, runtime) &&
              runtime.calls.empty(), "null config does not read XML");
    Check(pal4::PlaySound3DTask::LoadConfig(state, 0x7000u, runtime) &&
              state.words[18u] == 0xabcdef00u &&
              runtime.calls == std::vector<std::string>{"read-loop"},
          "zero loop config preserves neighboring bytes");

    state.words[16u] = 0x5000u;
    runtime.calls.clear();
    Check(pal4::PlaySound3DTask::Start(state, runtime) == 1u &&
              state.words[19u] == 0x6000u && runtime.non_looping == 1u &&
              runtime.calls == std::vector<std::string>{"resolve", "start"},
          "zero loop flag sends one to sound facet");

    runtime.loop = 5;
    runtime.calls.clear();
    Check(pal4::PlaySound3DTask::LoadConfig(state, 0x7000u, runtime) &&
              state.words[18u] == 0xabcdef01u,
          "positive loop config writes one byte");
    runtime.calls.clear();
    Check(pal4::PlaySound3DTask::Start(state, runtime) == 1u &&
              runtime.non_looping == 0u,
          "positive loop flag sends zero to sound facet");

    runtime.loop = -1;
    runtime.facet = 0u;
    (void)pal4::PlaySound3DTask::LoadConfig(state, 0x7000u, runtime);
    runtime.calls.clear();
    Check(pal4::PlaySound3DTask::Start(state, runtime) == 1u &&
              state.words[19u] == 0u && runtime.non_looping == 1u &&
              runtime.calls == std::vector<std::string>{"resolve", "start"},
          "native path dispatches when facet lookup yields null");

    runtime.calls.clear();
    Check(&pal4::PlaySound3DTask::Destroy(state, runtime) == &state &&
              runtime.calls == std::vector<std::string>{"destroy"},
          "destructor restores vtable before base cleanup");
}
