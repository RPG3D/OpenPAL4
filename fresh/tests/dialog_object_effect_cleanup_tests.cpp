#include "pal4/dialog_object.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IDialogObjectEffectCleanupRuntime {
    std::vector<std::string> calls;
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> words;
    bool enabled = true;
    bool active = false;
    std::uint32_t frame = 0x7000u;
    std::uint32_t parent = 0x8000u;
    std::uint32_t ReadWord(std::uint32_t address,
                           std::uint32_t offset) override {
        calls.emplace_back("read:" + std::to_string(address));
        Check(offset == 16u, "effect slot offset");
        return words.at({address, offset});
    }
    void WriteWord(std::uint32_t address, std::uint32_t offset,
                   std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address));
        Check(offset == 16u && value == 0u, "clear effect slot");
        words[{address, offset}] = value;
    }
    bool EffectsEnabled() override {
        calls.emplace_back("enabled"); return enabled;
    }
    bool EffectIsActive(std::uint32_t effect) override {
        Check(effect == 0x6000u, "activity effect");
        calls.emplace_back("active"); return active;
    }
    std::uint32_t EffectFrame(std::uint32_t effect) override {
        Check(effect == 0x6000u, "frame effect");
        calls.emplace_back("frame"); return frame;
    }
    std::uint32_t FrameParent(std::uint32_t value) override {
        Check(value == frame, "frame parent lookup");
        calls.emplace_back("parent"); return parent;
    }
    void RemoveFrameChild(std::uint32_t value) override {
        Check(value == frame, "frame detach");
        calls.emplace_back("detach");
    }
    void ProcessEffectEvent(std::uint32_t owner,
                            std::uint32_t effect) override {
        Check(owner == 0x9000u && effect == 0x6000u,
              "process event arguments");
        calls.emplace_back("process");
    }
    void DestroyEffect(std::uint32_t effect) override {
        Check(effect == 0x6000u, "destroy effect handle");
        calls.emplace_back("destroy");
    }
};
pal4::DialogObjectState State() {
    pal4::DialogObjectState state{};
    state.words[3u] = 0x9000u;
    state.words[28u] = 0x5000u;
    state.words[29u] = 0x5014u;
    return state;
}
Runtime MakeRuntime() {
    Runtime runtime;
    runtime.words[{0x5000u, 16u}] = 0x6000u;
    return runtime;
}
}  // namespace

int main() {
    auto state = State();
    auto runtime = MakeRuntime();
    state.words[29u] = state.words[28u];
    Check(pal4::DialogObject::StopEffects(state, runtime) == 0x5000u &&
          runtime.calls.empty(), "empty range returns end pointer");

    state = State();
    runtime.words[{0x5000u, 16u}] = 0u;
    Check(pal4::DialogObject::StopEffects(state, runtime) == 0x5014u &&
          runtime.calls == std::vector<std::string>{"read:20480"},
          "null effect skips all cleanup");

    runtime = MakeRuntime();
    Check(pal4::DialogObject::StopEffects(state, runtime) == 0x5014u &&
          runtime.words.at({0x5000u, 16u}) == 0u &&
          runtime.calls == std::vector<std::string>{
              "read:20480", "enabled", "process", "frame", "parent",
              "detach", "destroy", "write:20480"},
          "enabled effect processes, detaches and destroys in native order");

    runtime = MakeRuntime();
    runtime.enabled = false;
    runtime.active = true;
    Check(pal4::DialogObject::StopEffects(state, runtime) == 0x5014u &&
          runtime.calls == std::vector<std::string>{
              "read:20480", "enabled", "active", "destroy", "write:20480"},
          "disabled active effect skips event and frame cleanup");

    runtime = MakeRuntime();
    runtime.enabled = false;
    runtime.active = false;
    runtime.frame = 0u;
    Check(pal4::DialogObject::StopEffects(state, runtime) == 0x5014u &&
          runtime.calls == std::vector<std::string>{
              "read:20480", "enabled", "active", "process", "frame",
              "destroy", "write:20480"},
          "disabled inactive effect processes without a frame");

    runtime = MakeRuntime();
    runtime.parent = 0u;
    Check(pal4::DialogObject::StopEffects(state, runtime) == 0x5014u &&
          runtime.calls == std::vector<std::string>{
              "read:20480", "enabled", "process", "frame", "parent",
              "destroy", "write:20480"},
          "detached frame is not removed again");
}
