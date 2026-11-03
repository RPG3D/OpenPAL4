#include "pal4/dialog_manager_lifecycle.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IDialogManagerResetRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> inserted;
    bool has_story_list = true;
    std::uint32_t manager_calls = 0;
    float atomic_value = 0.0f;

    void WriteBytes(std::uint32_t, const void*, std::size_t) override {
        throw std::runtime_error("reset must not write archive");
    }
    void ReadBytes(std::uint32_t, void*, std::size_t) override {
        throw std::runtime_error("reset must not read archive");
    }
    std::uint32_t AudioManager() override {
        ++manager_calls;
        calls.emplace_back("audio manager");
        return 0x3000;
    }
    float AudioLevel(std::uint32_t, std::uint32_t) override { return 0; }
    void SetAudioLevel(std::uint32_t manager, std::uint32_t index,
                       float value) override {
        Check(manager == 0x3000 && index < 3,
              "default audio level operands");
        Check(std::bit_cast<std::uint32_t>(value) ==
                  (index == 0 ? 0x3e99999au : 0x3f4ccccdu),
              "default audio level bits");
        calls.emplace_back("set audio");
    }
    std::size_t AudioStreamCount(
        std::uint32_t,
        pal4::GlobalDataAudioCollection) override { return 0; }
    std::uint32_t AudioStreamAt(
        std::uint32_t, pal4::GlobalDataAudioCollection,
        std::size_t) override { return 0; }
    std::uint32_t AudioStreamKind(std::uint32_t) override { return 0; }
    void SetStreamVolume(std::uint32_t, float) override {}
    void RefreshStream(std::uint32_t) override {}
    void SetAtomicPluginFloatData(float value) override {
        atomic_value = value;
        calls.emplace_back("atomic");
    }
    bool HasEffectSystem() override { return true; }
    std::int32_t SetEffectSystemSquaredValue(float value) override {
        Check(value == atomic_value, "effect uses matching scale");
        calls.emplace_back("effect");
        return 77;
    }
    void ClearTree(pal4::DialogManagerState& state,
                   std::size_t offset) override {
        Check(state.Word(0) == 0 && state.Word(4) == 2 &&
                  state.Word(8) == 1 && state.Word(12) == 1 &&
                  state.Word(16) == 1 && state.Byte(20) == 1,
              "tree clearing occurs after global-data defaults");
        calls.emplace_back("clear:" + std::to_string(offset));
    }
    void ClearStoryArray(pal4::DialogManagerState& state) override {
        Check(state.Word(184) == 0x6000, "existing story storage retained");
        state.SetWord(188, state.Word(184));
        calls.emplace_back("clear story");
    }
    std::uint32_t GameInstance() override {
        calls.emplace_back("game");
        return 0x4000;
    }
    std::uint32_t GameStoryBegin(std::uint32_t game) override {
        Check(game == 0x4000, "story list game instance");
        calls.emplace_back("begin");
        return has_story_list ? 0x7000 : 0;
    }
    std::uint32_t GameStoryEnd(std::uint32_t game) override {
        Check(game == 0x4000, "story list end game instance");
        calls.emplace_back("end");
        return 0x7008;
    }
    std::uint32_t GameStoryValue(std::uint32_t address) override {
        Check(address == 0x7000 || address == 0x7004,
              "story list index stride");
        return address == 0x7000 ? 101 : 202;
    }
    std::uint32_t InsertStory(pal4::DialogManagerState& state,
                              std::uint32_t value) override {
        inserted.push_back(value);
        state.SetWord(188, state.Word(188) + 4);
        calls.emplace_back("insert");
        return 0x8000 + value;
    }
};

void Seed(pal4::DialogManagerState& state) {
    for (std::size_t offset = 0; offset <= 112; offset += 4)
        state.SetWord(offset, 0xffffffff);
    state.SetWord(184, 0x6000);
    state.SetWord(188, 0x6008);
}

void TestResetWithStoryList() {
    pal4::DialogManagerState state;
    Seed(state);
    Runtime runtime;
    Check(pal4::DialogManagerLifecycle::ResetState(state, runtime) == 2,
          "completed loop returns game list count");
    Check(runtime.manager_calls == 3 && runtime.atomic_value == 1200.0f,
          "reset composes audio and scale defaults");
    for (std::size_t offset = 24; offset <= 112; offset += 4) {
        if (offset == 84) continue;
        Check(state.Word(offset) == 0,
              "reset clears manager fixed fields and flags");
    }
    Check(state.Word(84) == 0 && state.Byte(20) == 1 &&
              state.Word(188) == 0x6008 &&
              runtime.inserted == std::vector<std::uint32_t>{101, 202},
          "story array rebuilt from game list");
    const auto clear_start = runtime.calls.begin() + 8;
    Check(std::vector<std::string>(clear_start, clear_start + 5) ==
              std::vector<std::string>{"clear:132", "clear:164",
                                       "clear:148", "clear:116",
                                       "clear story"},
          "tree and array clear order");
}

void TestResetWithoutStoryList() {
    pal4::DialogManagerState state;
    Seed(state);
    Runtime runtime;
    runtime.has_story_list = false;
    Check(pal4::DialogManagerLifecycle::ResetState(state, runtime) == 0x4000 &&
              runtime.inserted.empty() && state.Word(188) == 0x6000,
          "missing game list returns singleton and leaves story array empty");
}

}  // namespace

int main() {
    try {
        TestResetWithStoryList();
        TestResetWithoutStoryList();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
