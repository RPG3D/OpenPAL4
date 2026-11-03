#include "pal4/dialog_object.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IDialogObjectUpdateRuntime {
    std::vector<std::string> calls;
    bool ready = false;
    int UpdateTaskContainer(std::uint32_t container,
                            float elapsed) override {
        Check(container == 0x3000u && elapsed == 1.5f,
              "task container tick inputs");
        calls.emplace_back("update"); return 0x12345678;
    }
    bool TaskContainerReady(std::uint32_t container) override {
        Check(container == 0x3000u, "task container readiness");
        calls.emplace_back("ready"); return ready;
    }
    int ResetRuntimeState(pal4::DialogObjectState&) override {
        calls.emplace_back("reset"); return 0x5000;
    }
    std::array<float, 3u> ObjectPosition(
        std::uint32_t object) override {
        Check(object == 0x4000u, "tracked object handle");
        calls.emplace_back("position"); return {1.0f, 2.0f, 3.0f};
    }
    int SetAbsolutePosition(
        pal4::DialogObjectState&,
        const std::array<float, 3u>& position) override {
        Check(position == std::array<float, 3u>{1.0f, 2.0f, 3.0f},
              "absolute tracked position");
        calls.emplace_back("set-position"); return 0x77;
    }
};
pal4::DialogObjectState State() {
    pal4::DialogObjectState state{};
    state.words[2u] = 0x3000u;
    state.words[20u] = 0x00000100u;
    state.words[54u] = 0x4000u;
    return state;
}
}  // namespace

int main() {
    auto state = State();
    Runtime runtime;
    state.words[20u] = 0x00020100u;
    Check(pal4::DialogObject::UpdatePlayback(state, 1.5f, runtime) == 2 &&
          runtime.calls.empty(), "stopped byte blocks update");
    state = State();
    state.words[20u] = 0u;
    Check(pal4::DialogObject::UpdatePlayback(state, 1.5f, runtime) == 0 &&
          runtime.calls.empty(), "non-waiting state blocks update");
    state = State();
    state.words[2u] = 0u;
    Check(pal4::DialogObject::UpdatePlayback(state, 1.5f, runtime) == 0 &&
          runtime.calls.empty(), "null task container blocks update");

    state = State();
    Check(pal4::DialogObject::UpdatePlayback(state, 1.5f, runtime) ==
              0x12345600 &&
          runtime.calls == std::vector<std::string>{"update", "ready"},
          "unfinished task without tracking returns updated high bytes");

    runtime.calls.clear();
    runtime.ready = true;
    state.words[53u] = 1u;
    Check(pal4::DialogObject::UpdatePlayback(state, 1.5f, runtime) ==
              0x5000 &&
          runtime.calls == std::vector<std::string>{
              "update", "ready", "reset"},
          "completed tasks reset before tracked-position follow");

    runtime.calls.clear();
    runtime.ready = false;
    Check(pal4::DialogObject::UpdatePlayback(state, 1.5f, runtime) ==
              0x77 &&
          runtime.calls == std::vector<std::string>{
              "update", "ready", "position", "set-position"},
          "unfinished tasks follow tracked object's absolute position");
}
