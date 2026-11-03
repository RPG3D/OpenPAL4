#include "pal4/play_effect_task.h"

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

struct Runtime final : pal4::IPlayEffectTaskRuntime,
                       pal4::IPlayEffectConfigRuntime {
    std::vector<std::string> calls;
    std::int32_t loop = 1;
    std::int32_t inherit = 1;
    std::uint32_t facet = 0x6000u;
    std::uint8_t sent_loop = 0xffu;

    void ConstructBase(pal4::PlayEffectTaskState& state) override {
        calls.emplace_back("construct");
        state.words[0u] = 0xdeadbeefu;
    }
    void DestroyBase(pal4::PlayEffectTaskState& state) override {
        Check(state.words[0u] == pal4::PlayEffectTask::kVtable,
              "base cleanup sees derived vtable");
        calls.emplace_back("destroy");
    }
    std::uint32_t ResolveFacet(std::uint32_t object,
                                std::string_view name) override {
        Check(object == 0x5000u && name == "PlayEffectTask",
              "resolve play facet through owner");
        calls.emplace_back("resolve");
        return facet;
    }
    void SetLoop(std::uint32_t resolved,
                 std::uint8_t enabled) override {
        Check(resolved == facet, "loop setter receives resolved facet");
        calls.emplace_back("set-loop");
        sent_loop = enabled;
    }
    std::int32_t ReadInt(std::uint32_t config, std::string_view key,
                         std::int32_t fallback) override {
        Check(config == 0x7000u && fallback == 0,
              "config reads use owner and zero default");
        calls.emplace_back(std::string(key));
        if (key == "Attr_Task_PlayEffect_Loop") return loop;
        Check(key == "Attr_Inherit_Pre_Task", "inherit attribute key");
        return inherit;
    }
};

}  // namespace

int main() {
    pal4::PlayEffectTaskState state{};
    state.words.fill(0xabcdef01u);
    Runtime runtime;
    Check(&pal4::PlayEffectTask::Construct(state, runtime) == &state &&
              state.words[0u] == pal4::PlayEffectTask::kVtable &&
              state.words[19u] == 0u &&
              state.words[18u] == 0xabcdef01u &&
              runtime.calls == std::vector<std::string>{"construct"},
          "constructor initializes facet after base construction");
    Check(pal4::PlayEffectTask::ClassName() == "PlayEffectTask",
          "class name matches IDA literal");

    runtime.calls.clear();
    Check(!pal4::PlayEffectTask::LoadConfig(state, 0u, runtime) &&
              runtime.calls.empty() && state.words[18u] == 0xabcdef01u,
          "null config leaves flags untouched");
    Check(pal4::PlayEffectTask::LoadConfig(state, 0x7000u, runtime) &&
              state.words[18u] == 0xabcdef01u &&
              state.words[15u] == 0xabcd0101u &&
              runtime.calls == std::vector<std::string>{
                                   "Attr_Task_PlayEffect_Loop",
                                   "Attr_Inherit_Pre_Task"},
          "positive values write +72 and +61 bytes in order");

    state.words[16u] = 0x5000u;
    runtime.calls.clear();
    Check(pal4::PlayEffectTask::Start(state, runtime) == 1u &&
              state.words[19u] == 0x6000u && runtime.sent_loop == 1u &&
              runtime.calls == std::vector<std::string>{
                                   "resolve", "set-loop"},
          "start resolves facet and forwards loop flag");
    Check(pal4::PlayEffectTask::Reset(state) == 1u &&
              state.words[19u] == 0u && state.words[18u] == 0xabcdef01u,
          "reset clears facet without changing loop flag");

    runtime.loop = -1;
    runtime.inherit = 0;
    runtime.calls.clear();
    Check(pal4::PlayEffectTask::LoadConfig(state, 0x7000u, runtime) &&
              state.words[18u] == 0xabcdef00u &&
              state.words[15u] == 0xabcd0001u,
          "nonpositive values clear only configured bytes");
    runtime.facet = 0u;
    runtime.calls.clear();
    Check(pal4::PlayEffectTask::Start(state, runtime) == 1u &&
              runtime.sent_loop == 0u && state.words[19u] == 0u &&
              runtime.calls == std::vector<std::string>{
                                   "resolve", "set-loop"},
          "native start forwards loop even when facet lookup returns null");

    runtime.calls.clear();
    Check(&pal4::PlayEffectTask::Destroy(state, runtime) == &state &&
              runtime.calls == std::vector<std::string>{"destroy"},
          "destructor restores vtable before base cleanup");
}
