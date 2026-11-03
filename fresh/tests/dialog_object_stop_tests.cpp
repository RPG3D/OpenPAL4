#include "pal4/dialog_object.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IDialogObjectStopRuntime {
    std::vector<std::string> calls;
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> words;
    std::map<std::uint32_t, double> values;
    std::uint32_t parent = 0x9000u;
    void CleanupObjectCollection(std::uint32_t container) override {
        Check(container == 0x1000u, "object cleanup receives this dialog's task container");
        calls.emplace_back("objects");
    }
    void DestroyTaskContainer(std::uint32_t container) override {
        Check(container == 0x1000u, "task container");
        calls.emplace_back("destroy-container");
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x1000u, "container allocation");
        calls.emplace_back("free");
    }
    std::uint32_t FrameParent(std::uint32_t frame) override {
        Check(frame == 0x2000u, "stored frame");
        calls.emplace_back("parent"); return parent;
    }
    void RemoveFrameChild(std::uint32_t frame) override {
        Check(frame == 0x2000u, "frame detach");
        calls.emplace_back("detach");
    }
    std::uint32_t ReadWord(std::uint32_t address,
                           std::uint32_t offset) override {
        Check(offset == 16u, "effect record handle offset");
        calls.emplace_back("read:" + std::to_string(address));
        return words.at({address, offset});
    }
    void WriteWord(std::uint32_t address, std::uint32_t offset,
                   std::uint32_t value) override {
        Check(offset == 4u && value == 0u, "task record handle clear");
        calls.emplace_back("write:" + std::to_string(address));
        words[{address, offset}] = value;
    }
    void InitializeEffectSystem() override {
        calls.emplace_back("init-effects");
    }
    void SetEffectMode(std::uint32_t effect,
                       std::uint32_t mode) override {
        Check(mode == 1u && values.contains(effect), "effect mode one");
        calls.emplace_back("mode:" + std::to_string(effect));
    }
    double EffectValue(std::uint32_t effect) override {
        calls.emplace_back("value:" + std::to_string(effect));
        return values.at(effect);
    }
};
pal4::DialogObjectState State() {
    pal4::DialogObjectState state{};
    state.words[2u] = 0x1000u;
    state.words[24u] = 0x3000u;
    state.words[25u] = 0x3010u;
    state.words[28u] = 0x6000u;
    state.words[29u] = 0x603cu;
    state.words[43u] = 0x2000u;
    return state;
}
Runtime MakeRuntime() {
    Runtime runtime;
    runtime.words[{0x3000u, 4u}] = 0x1111u;
    runtime.words[{0x3008u, 4u}] = 0x2222u;
    runtime.words[{0x6000u, 16u}] = 0x4000u;
    runtime.words[{0x6014u, 16u}] = 0x5000u;
    runtime.words[{0x6028u, 16u}] = 0u;
    runtime.values[0x4000u] = 2.25;
    runtime.values[0x5000u] = 1.5;
    return runtime;
}
}  // namespace

int main() {
    auto state = State();
    auto runtime = MakeRuntime();
    state.words[2u] = 0u;
    Check(pal4::DialogObject::StopPlayback(state, true, runtime) == 0.0 &&
          runtime.calls.empty(), "missing container exits without effects");

    state = State();
    Check(pal4::DialogObject::StopPlayback(state, true, runtime) == 2.25 &&
          state.words[2u] == 0u && state.words[43u] == 0x2000u &&
          runtime.words.at({0x3000u, 4u}) == 0u &&
          runtime.words.at({0x3008u, 4u}) == 0u &&
          runtime.calls == std::vector<std::string>{
              "objects", "destroy-container", "free", "parent", "detach",
              "write:12288", "write:12296", "read:24576", "init-effects",
              "mode:16384", "value:16384", "read:24596", "init-effects",
              "mode:20480", "value:20480", "read:24616"},
          "stop cleans container, detaches frame and tracks maximum");

    state = State();
    runtime = MakeRuntime();
    Check(pal4::DialogObject::StopPlayback(state, false, runtime) == 2.25 &&
          runtime.calls == std::vector<std::string>{
              "objects", "destroy-container", "free", "write:12288",
              "write:12296", "read:24576", "init-effects", "mode:16384",
              "value:16384", "read:24596", "init-effects", "mode:20480",
              "value:20480", "read:24616"},
          "detach flag false skips frame-parent lookup");

    state = State();
    runtime = MakeRuntime();
    runtime.parent = 0u;
    Check(pal4::DialogObject::StopPlayback(state, true, runtime) == 2.25 &&
          runtime.calls[3u] == "parent" &&
          runtime.calls[4u] == "write:12288",
          "unparented frame is not detached");

    state = State();
    runtime = MakeRuntime();
    runtime.values[0x4000u] = -1.0;
    runtime.values[0x5000u] = -2.0;
    Check(pal4::DialogObject::StopPlayback(state, false, runtime) == 0.0,
          "negative effect values leave zero maximum");

    state = State();
    runtime = MakeRuntime();
    runtime.values[0x4000u] = std::numeric_limits<double>::quiet_NaN();
    runtime.values[0x5000u] = 0.1;
    Check(pal4::DialogObject::StopPlayback(state, false, runtime) ==
              static_cast<double>(static_cast<float>(0.1)),
          "unordered comparison replaces value and return is float-rounded");

    state = State();
    runtime = MakeRuntime();
    state.words[25u] = state.words[24u];
    state.words[29u] = state.words[28u];
    Check(pal4::DialogObject::StopPlayback(state, false, runtime) == 0.0 &&
          runtime.calls == std::vector<std::string>{
              "objects", "destroy-container", "free"},
          "empty task and effect arrays still destroy container");
}
