#include "pal4/dialog_object.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IDialogObjectPlaybackRuntime {
    std::vector<std::string> calls;
    std::uint32_t allocated = 0x7000u;
    std::uint32_t frame = 0x6000u;
    bool activate = true;
    bool tasks = true;
    bool throw_tasks = false;
    void ResetBeforePlayback(pal4::DialogObjectState&) override {
        calls.emplace_back("reset");
    }
    void DestroyFrame(std::uint32_t value) override {
        calls.emplace_back("destroy-frame:" + std::to_string(value));
    }
    std::uint32_t CreateFrame() override {
        calls.emplace_back("create-frame"); return frame;
    }
    void RefreshFrame(pal4::DialogObjectState& state) override {
        Check(state.words[43u] == frame, "refresh new frame");
        calls.emplace_back("refresh");
    }
    void DestroyTaskContainer(std::uint32_t container) override {
        calls.emplace_back("destroy-container:" + std::to_string(container));
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x10u, "task container allocation size");
        calls.emplace_back("allocate"); return allocated;
    }
    std::uint32_t InitializeTaskContainer(
        std::uint32_t memory) override {
        Check(memory == allocated, "initialize allocated container");
        calls.emplace_back("init-container"); return 0x8000u;
    }
    bool ActivateEffects(pal4::DialogObjectState&) override {
        calls.emplace_back("activate"); return activate;
    }
    void StopEffects(pal4::DialogObjectState&) override {
        calls.emplace_back("stop");
    }
    bool InitializeTasks(pal4::DialogObjectState&) override {
        calls.emplace_back("tasks");
        if (throw_tasks) throw std::runtime_error("task exception");
        return tasks;
    }
    void ResetTaskSlots(pal4::DialogObjectState&) override {
        calls.emplace_back("reset-slots");
    }
};
pal4::DialogObjectState State() {
    pal4::DialogObjectState state{};
    state.words[2u] = 0x5000u;
    state.words[3u] = 0x9000u;
    state.words[20u] = 0xffffff00u;
    state.words[24u] = 0x2000u;
    state.words[25u] = 0x2008u;
    state.words[28u] = 0x3000u;
    state.words[29u] = 0x3014u;
    state.words[43u] = 0x4000u;
    return state;
}
}  // namespace

int main() {
    auto state = State();
    Runtime runtime;
    state.words[1u] = 1u;
    Check(!pal4::DialogObject::PreparePlayback(state, runtime) &&
          runtime.calls.empty(), "object byte +4 blocks preparation");
    state = State();
    state.words[3u] = 0u;
    Check(!pal4::DialogObject::PreparePlayback(state, runtime) &&
          runtime.calls.empty(), "missing owner blocks preparation");
    state = State();
    state.words[20u] |= 1u;
    Check(pal4::DialogObject::PreparePlayback(state, runtime) &&
          runtime.calls.empty(), "ready state returns without work");

    state = State();
    state.words[24u] = 0u;
    Check(!pal4::DialogObject::PreparePlayback(state, runtime) &&
          runtime.calls == std::vector<std::string>{"reset"},
          "null task-array pointer fails after reset");
    runtime.calls.clear();
    state = State();
    state.words[25u] = state.words[24u];
    Check(!pal4::DialogObject::PreparePlayback(state, runtime) &&
          runtime.calls == std::vector<std::string>{"reset"},
          "empty task array fails before frame changes");
    runtime.calls.clear();
    state = State();
    state.words[29u] = state.words[28u];
    Check(!pal4::DialogObject::PreparePlayback(state, runtime) &&
          runtime.calls == std::vector<std::string>{"reset"},
          "empty effect array fails before frame changes");

    runtime.calls.clear();
    state = State();
    Check(pal4::DialogObject::PreparePlayback(state, runtime) &&
          state.words[43u] == 0x6000u && state.words[2u] == 0x8000u &&
          state.words[20u] == 0xffff0001u &&
          runtime.calls == std::vector<std::string>{
              "reset", "destroy-frame:16384", "create-frame", "refresh",
              "destroy-container:20480", "free:20480", "allocate",
              "init-container", "activate", "tasks"},
          "success rebuilds frame and container then sets exact status bytes");

    runtime = Runtime{};
    runtime.activate = false;
    state = State();
    Check(!pal4::DialogObject::PreparePlayback(state, runtime) &&
          state.words[43u] == 0u && state.words[2u] == 0u &&
          runtime.calls == std::vector<std::string>{
              "reset", "destroy-frame:16384", "create-frame", "refresh",
              "destroy-container:20480", "free:20480", "allocate",
              "init-container", "activate", "stop", "destroy-frame:24576",
              "destroy-container:32768", "free:32768"},
          "effect activation failure stops effects and releases new resources");

    runtime = Runtime{};
    runtime.tasks = false;
    state = State();
    Check(!pal4::DialogObject::PreparePlayback(state, runtime) &&
          state.words[43u] == 0u && state.words[2u] == 0u &&
          runtime.calls == std::vector<std::string>{
              "reset", "destroy-frame:16384", "create-frame", "refresh",
              "destroy-container:20480", "free:20480", "allocate",
              "init-container", "activate", "tasks", "stop", "reset-slots",
              "destroy-frame:24576", "destroy-container:32768", "free:32768"},
          "task failure also clears task slots before resource release");

    runtime = Runtime{};
    runtime.allocated = 0u;
    runtime.activate = false;
    state = State();
    state.words[2u] = 0u;
    state.words[43u] = 0u;
    Check(!pal4::DialogObject::PreparePlayback(state, runtime) &&
          runtime.calls == std::vector<std::string>{
              "reset", "create-frame", "refresh", "allocate", "activate",
              "stop", "destroy-frame:24576"},
          "null container allocation still reaches activation and cleans frame");

    runtime = Runtime{};
    runtime.throw_tasks = true;
    state = State();
    bool threw = false;
    try { (void)pal4::DialogObject::PreparePlayback(state, runtime); }
    catch (const std::runtime_error&) { threw = true; }
    Check(threw && state.words[43u] == 0x6000u &&
          state.words[2u] == 0x8000u && runtime.calls.back() == "tasks",
          "native task exception propagates without return-path rollback");
}
