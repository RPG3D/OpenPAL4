#include "pal4/music_ui_state.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IMusicUiRuntime {
    std::vector<std::string> calls;
    bool allocated = true;
    bool throw_on_fade = false;
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        if (address == 0x1000u) {
            Check(value == pal4::MusicUiState::kVtable,
                  "music UI vtable write");
            calls.emplace_back("construct");
        } else {
            Check(address == 0x3000u && value == 0u,
                  "music UI console reset");
            calls.emplace_back("clear-console");
        }
    }
    std::uint32_t UIFrameManager() override {
        calls.emplace_back("ui"); return 0x2000u;
    }
    void SetUIFlag(std::uint32_t manager, std::uint32_t value) override {
        Check(manager == 0x2000u && value == 1u, "music UI flag");
        calls.emplace_back("set-flag");
    }
    std::uint32_t Console() override {
        calls.emplace_back("console"); return 0x3000u;
    }
    bool AllocateTrack(std::string_view track) override {
        Check(track == "P01-1.mp3", "track allocation name");
        calls.emplace_back("allocate-track"); return allocated;
    }
    void ReleaseTrack() override {
        calls.emplace_back("release-track");
    }
    std::uint32_t MusicPlayer() override {
        calls.emplace_back("music-player"); return 0x4000u;
    }
    void FadeInMusic(std::uint32_t player,
                     std::string_view track,
                     double seconds, std::uint32_t flags) override {
        Check(player == 0x4000u && seconds == 3.0 && flags == 0u &&
              track == (allocated ? "P01-1.mp3" : ""),
              "music fade arguments including allocation fallback");
        calls.emplace_back("fade-music");
        if (throw_on_fade) throw std::runtime_error("injected fade failure");
    }
    void UpdateUI(std::uint32_t manager, double delta) override {
        Check(manager == 0x2000u && delta == 0.25,
              "music UI update delta");
        calls.emplace_back("update-ui");
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("script"); return 0x5000u;
    }
    std::uint32_t ProcessLog(
        std::uint32_t logger, double delta) override {
        Check(logger == 0x5054u && delta == 0.25,
              "music UI log arguments");
        calls.emplace_back("process-log"); return 0x77u;
    }
};

void TestMusicUi() {
    Runtime runtime;
    Check(pal4::MusicUiState::Construct(0x1000u, runtime) == 0x1000u &&
          runtime.calls == std::vector<std::string>{"construct"},
          "music UI constructor");
    runtime.calls.clear();
    Check(pal4::MusicUiState::Enter(99u, 98u, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{
              "ui", "set-flag", "console", "clear-console",
              "allocate-track", "music-player", "fade-music", "release-track"},
          "music UI entry order and owned buffer release");
    runtime.calls.clear(); runtime.allocated = false;
    Check(pal4::MusicUiState::Enter(0u, 0u, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{
              "ui", "set-flag", "console", "clear-console",
              "allocate-track", "music-player", "fade-music"},
          "allocation failure still fades with empty buffer");
    runtime.calls.clear(); runtime.allocated = true;
    runtime.throw_on_fade = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::MusicUiState::Enter(0u, 0u, runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.calls.back() == "release-track",
          "owned music buffer released on fade exception");
    runtime.calls.clear();
    Check(pal4::MusicUiState::Update(0.25f, 99u, 98u, runtime) == 0x77u &&
          runtime.calls == std::vector<std::string>{
              "ui", "update-ui", "script", "process-log"},
          "UI advances before log processing and forwards log result");
    Check(pal4::MusicUiState::GetTypeCode() == 11u,
          "music UI type code");
}

}  // namespace

int main() {
    try {
        TestMusicUi();
        std::cout << "music UI state tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
