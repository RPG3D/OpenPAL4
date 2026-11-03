#include "pal4/dialog_object.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IDialogObjectResumeRuntime {
    std::vector<std::string> calls;
    std::uint32_t effect = 0x6000u;
    std::uint32_t frame = 0x7000u;
    bool enabled = true;
    bool active = false;
    bool restart = true;
    bool tasks = true;
    void ResetBeforePlayback(pal4::DialogObjectState&) override {
        calls.emplace_back("reset");
    }
    void DestroyFrame(std::uint32_t value) override {
        calls.emplace_back("destroy-frame:" + std::to_string(value));
    }
    std::uint32_t CreateFrame() override { std::abort(); }
    void RefreshFrame(pal4::DialogObjectState&) override {
        calls.emplace_back("refresh");
    }
    void DestroyTaskContainer(std::uint32_t value) override {
        calls.emplace_back("destroy-container:" + std::to_string(value));
    }
    void Free(std::uint32_t value) override {
        calls.emplace_back("free:" + std::to_string(value));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x10u, "container allocation size");
        calls.emplace_back("allocate"); return 0x7000u;
    }
    std::uint32_t InitializeTaskContainer(std::uint32_t memory) override {
        Check(memory == 0x7000u, "container memory");
        calls.emplace_back("init-container"); return 0x8000u;
    }
    bool ActivateEffects(pal4::DialogObjectState&) override { std::abort(); }
    void StopEffects(pal4::DialogObjectState&) override {
        calls.emplace_back("stop");
    }
    bool InitializeTasks(pal4::DialogObjectState&) override {
        calls.emplace_back("tasks"); return tasks;
    }
    void ResetTaskSlots(pal4::DialogObjectState&) override {
        calls.emplace_back("reset-slots");
    }
    std::uint32_t ReadEffectHandle(std::uint32_t record) override {
        Check(record == 0x3000u, "effect record stride");
        calls.emplace_back("read-effect"); return effect;
    }
    void SetEffectMode(std::uint32_t value, std::uint32_t mode) override {
        Check(value == effect && mode == 0u, "effect virtual +48 mode zero");
        calls.emplace_back("mode:0");
    }
    bool EffectsEnabled() override {
        calls.emplace_back("enabled"); return enabled;
    }
    bool EffectIsActive(std::uint32_t value) override {
        Check(value == effect, "effect activity check");
        calls.emplace_back("active"); return active;
    }
    void WriteEffectWord(std::uint32_t value, std::uint32_t offset,
                         std::uint32_t written) override {
        Check(value == effect &&
              ((offset == 16u && written == 0x9000u) ||
               (offset == 12u && written == 0xa000u)),
              "effect owner pointers");
        calls.emplace_back("write:" + std::to_string(offset));
    }
    bool RestartEffect(std::uint32_t value) override {
        Check(value == effect, "restart effect");
        calls.emplace_back("restart"); return restart;
    }
    void ProcessEffectEvent(std::uint32_t owner,
                            std::uint32_t value) override {
        Check(owner == 0x9000u && value == effect, "effect event arguments");
        calls.emplace_back("process");
    }
    std::uint32_t EffectFrame(std::uint32_t value) override {
        Check(value == effect, "effect frame query");
        calls.emplace_back("frame"); return frame;
    }
    void AddFrameChild(std::uint32_t parent,
                       std::uint32_t child) override {
        Check(parent == 0x4000u && child == frame, "frame attachment");
        calls.emplace_back("add-child");
    }
};
pal4::DialogObjectState State() {
    pal4::DialogObjectState state{};
    state.words[1u] = 1u;
    state.words[2u] = 0x5000u;
    state.words[3u] = 0x9000u;
    state.words[20u] = 0xffffffffu;
    state.words[28u] = 0x3000u;
    state.words[29u] = 0x3014u;
    state.words[43u] = 0x4000u;
    return state;
}
const std::vector<std::string> prefix{
    "reset", "destroy-container:20480", "free:20480", "allocate",
    "init-container"};
std::vector<std::string> WithPrefix(std::vector<std::string> tail) {
    auto result = prefix;
    result.insert(result.end(), tail.begin(), tail.end());
    return result;
}
}  // namespace

int main() {
    auto state = State();
    Runtime runtime;
    state.words[1u] = 0u;
    Check(!pal4::DialogObject::ResumePlayback(state, 0xa000u, runtime) &&
          runtime.calls.empty(), "non-paused object cannot resume");
    state = State();
    state.words[3u] = 0u;
    Check(!pal4::DialogObject::ResumePlayback(state, 0xa000u, runtime) &&
          runtime.calls.empty(), "missing owner cannot resume");

    state = State();
    state.words[29u] = state.words[28u];
    Check(pal4::DialogObject::ResumePlayback(state, 0xa000u, runtime) &&
          state.words[20u] == 0xffff0001u &&
          runtime.calls == WithPrefix({"tasks", "refresh"}),
          "empty effect list still reinitializes tasks and status");

    runtime = Runtime{};
    runtime.effect = 0u;
    state = State();
    Check(!pal4::DialogObject::ResumePlayback(state, 0xa000u, runtime) &&
          state.words[1u] == 0u && state.words[20u] == 0xffff0000u &&
          state.words[2u] == 0u && state.words[43u] == 0u &&
          runtime.calls == WithPrefix({
              "read-effect", "stop", "reset-slots", "destroy-frame:16384",
              "destroy-container:32768", "free:32768"}),
          "missing effect clears flags, frame, slots and container");

    runtime = Runtime{};
    state = State();
    Check(pal4::DialogObject::ResumePlayback(state, 0xa000u, runtime) &&
          state.words[2u] == 0x8000u && state.words[43u] == 0x4000u &&
          state.words[20u] == 0xffff0001u &&
          runtime.calls == WithPrefix({
              "read-effect", "mode:0", "enabled", "write:16", "write:12",
              "restart", "process", "frame", "frame", "add-child",
              "tasks", "refresh"}),
          "enabled effect restarts, processes and attaches twice-read frame");

    runtime = Runtime{};
    runtime.restart = false;
    state = State();
    Check(!pal4::DialogObject::ResumePlayback(state, 0xa000u, runtime) &&
          state.words[2u] == 0u && state.words[43u] == 0x4000u &&
          runtime.calls == WithPrefix({
              "read-effect", "mode:0", "enabled", "write:16", "write:12",
              "restart", "destroy-container:32768", "free:32768"}),
          "enabled restart failure releases only new container");

    runtime = Runtime{};
    runtime.frame = 0u;
    state = State();
    Check(!pal4::DialogObject::ResumePlayback(state, 0xa000u, runtime) &&
          runtime.calls == WithPrefix({
              "read-effect", "mode:0", "enabled", "write:16", "write:12",
              "restart", "process", "frame", "destroy-container:32768",
              "free:32768"}),
          "enabled missing frame also releases only container");

    runtime = Runtime{};
    runtime.enabled = false;
    runtime.active = true;
    state = State();
    Check(pal4::DialogObject::ResumePlayback(state, 0xa000u, runtime) &&
          runtime.calls == WithPrefix({
              "read-effect", "mode:0", "enabled", "active", "tasks",
              "refresh"}),
          "disabled active effect skips restart and attachment");

    runtime = Runtime{};
    runtime.enabled = false;
    runtime.restart = false;
    state = State();
    Check(!pal4::DialogObject::ResumePlayback(state, 0xa000u, runtime) &&
          state.words[43u] == 0u && state.words[2u] == 0u &&
          runtime.calls == WithPrefix({
              "read-effect", "mode:0", "enabled", "active", "write:16",
              "write:12", "restart", "stop", "destroy-frame:16384",
              "destroy-container:32768", "free:32768"}),
          "disabled restart failure stops and releases frame/container");

    runtime = Runtime{};
    runtime.enabled = false;
    runtime.frame = 0u;
    state = State();
    Check(!pal4::DialogObject::ResumePlayback(state, 0xa000u, runtime) &&
          runtime.calls == WithPrefix({
              "read-effect", "mode:0", "enabled", "active", "write:16",
              "write:12", "restart", "process", "frame", "stop",
              "destroy-frame:16384", "destroy-container:32768", "free:32768"}),
          "disabled missing frame takes effect stop path");

    runtime = Runtime{};
    runtime.tasks = false;
    state = State();
    Check(!pal4::DialogObject::ResumePlayback(state, 0xa000u, runtime) &&
          state.words[43u] == 0u && state.words[2u] == 0u &&
          runtime.calls == WithPrefix({
              "read-effect", "mode:0", "enabled", "write:16", "write:12",
              "restart", "process", "frame", "frame", "add-child",
              "tasks", "stop", "reset-slots", "destroy-frame:16384",
              "destroy-container:32768", "free:32768"}),
          "task failure stops effects and resets slots before release");
}
