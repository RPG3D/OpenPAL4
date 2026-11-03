#include "pal4/dialog_object.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IDialogObjectPauseRuntime {
    std::vector<std::string> calls;
    std::uint32_t ReadEffectHandle(std::uint32_t record) override {
        Check(record == 0x3000u || record == 0x3014u,
              "20-byte effect record stride");
        calls.emplace_back("read:" + std::to_string(record));
        return record == 0x3000u ? 0x4000u : 0x5000u;
    }
    void PauseEffect(std::uint32_t effect) override {
        Check(effect == 0x4000u || effect == 0x5000u,
              "effect pause virtual call");
        calls.emplace_back("pause:" + std::to_string(effect));
    }
    void RefreshFrame(pal4::DialogObjectState& state) override {
        Check((state.words[20u] & 0xffffffu) == 0x000100u,
              "status bytes set before frame refresh");
        calls.emplace_back("refresh");
    }
    void BeginWaiting(std::uint32_t container) override {
        Check(container == 0x2000u, "task container waiting dispatch");
        calls.emplace_back("wait");
    }
};
pal4::DialogObjectState State() {
    pal4::DialogObjectState state{};
    state.words[2u] = 0x2000u;
    state.words[20u] = 0xddcc0001u;
    state.words[28u] = 0x3000u;
    state.words[29u] = 0x3028u;
    return state;
}
}  // namespace

int main() {
    auto state = State();
    Runtime runtime;
    state.words[2u] = 0u;
    Check(!pal4::DialogObject::PausePlayback(state, runtime) &&
          state.words[20u] == 0xddcc0001u && runtime.calls.empty(),
          "missing task container leaves state unchanged");
    state = State();
    state.words[20u] = 0xddcc0000u;
    Check(!pal4::DialogObject::PausePlayback(state, runtime) &&
          runtime.calls.empty(), "not-playing state cannot pause");
    state = State();
    state.words[20u] = 0xddcc0201u;
    Check(pal4::DialogObject::PausePlayback(state, runtime) &&
          state.words[20u] == 0xddcc0200u && runtime.calls.empty(),
          "already waiting only clears playback byte");

    state = State();
    Check(pal4::DialogObject::PausePlayback(state, runtime) &&
          state.words[20u] == 0xdd000100u &&
          runtime.calls == std::vector<std::string>{
              "read:12288", "pause:16384", "read:12308", "pause:20480",
              "refresh", "wait"},
          "pause traverses effects then refreshes and begins waiting");

    runtime.calls.clear();
    state = State();
    state.words[29u] = state.words[28u];
    Check(pal4::DialogObject::PausePlayback(state, runtime) &&
          state.words[20u] == 0xdd000100u &&
          runtime.calls == std::vector<std::string>{"refresh", "wait"},
          "empty effect list still enters waiting state");
}
