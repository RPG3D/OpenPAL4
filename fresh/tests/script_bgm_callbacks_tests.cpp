#include "pal4/script_bgm_callbacks.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptBgmRuntime {
    std::uint32_t name = 0x4000u;
    std::int32_t comparison = 0;
    std::vector<std::string> calls;

    std::uint32_t MusicPlayer() override {
        calls.emplace_back("music"); return 0x1000u;
    }
    std::uint8_t SetMusicBuffer(std::uint32_t player,
                                std::uint32_t source) override {
        Check(player == 0x1000u && source == 0x5000u,
              "music buffer source and player");
        calls.emplace_back("buffer"); return 0x91u;
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena"); return 0x2000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x20e0u) {
            calls.emplace_back("area"); return 0x3000u;
        }
        Check(address == 0x6004u, "script area name payload offset");
        calls.emplace_back("name"); return name;
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty"); return 0x7000u;
    }
    std::int32_t CompareIgnoreCase(std::uint32_t lhs,
                                   std::uint32_t rhs) override {
        Check(lhs == (name == 0u ? 0x7000u : name) && rhs == 0x3000u,
              "compare script name against arena +0xe0 string");
        calls.emplace_back("compare"); return comparison;
    }
    std::uint8_t SetProcessingState(std::uint32_t player,
                                    std::uint8_t muted) override {
        Check(player == 0x1000u && muted == 2u,
              "mute forwards raw nonzero byte");
        calls.emplace_back("mute"); return 0x92u;
    }
    std::uint32_t PlayTrack(std::uint32_t player,
                            std::uint32_t source, std::int32_t mode,
                            float first, float second) override {
        Check(player == 0x1000u && source == 0x5000u && mode == -3 &&
                  first == 0.25f && second == 1.5f,
              "music play forwards all four script arguments");
        calls.emplace_back("play"); return 0x93u;
    }
    std::uint32_t StopTrack(std::uint32_t player,
                            std::int32_t mode, float fade) override {
        Check(player == 0x1000u && mode == 2 && fade == 0.75f,
              "music stop forwards mode and float");
        calls.emplace_back("stop"); return 0x94u;
    }
    std::uint32_t FadeOut(std::uint32_t player,
                          float duration) override {
        Check(player == 0x1000u && duration == 2.5f,
              "arena music fade forwards duration");
        calls.emplace_back("fade"); return 0x95u;
    }
};

void MusicForwarders() {
    Runtime runtime;
    Check(pal4::ScriptBgmCallbacks::SetMusic(0x5000u, runtime) == 0x91u &&
              runtime.calls == std::vector<std::string>{"music", "buffer"},
          "music buffer forwarding");
    runtime.calls.clear();
    Check(pal4::ScriptBgmCallbacks::Mute(2u, runtime) == 0x92u &&
              runtime.calls == std::vector<std::string>{"music", "mute"},
          "mute forwarding");
    runtime.calls.clear();
    Check(pal4::ScriptBgmCallbacks::Play(
                  0x5000u, -3, 0.25f, 1.5f, runtime) == 0x93u &&
              runtime.calls == std::vector<std::string>{"music", "play"},
          "music play forwarding");
    runtime.calls.clear();
    Check(pal4::ScriptBgmCallbacks::Stop(2, 0.75f, runtime) == 0x94u &&
              runtime.calls == std::vector<std::string>{"music", "stop"},
          "music stop forwarding");
    runtime.calls.clear();
    Check(pal4::ScriptBgmCallbacks::StopArenaMusic(2.5f, runtime) == 0x95u &&
              runtime.calls == std::vector<std::string>{"music", "fade"},
          "arena music fade forwarding");
}

void AreaComparison() {
    for (const bool empty : {false, true}) {
        Runtime runtime;
        if (empty) runtime.name = 0u;
        auto expected = std::vector<std::string>{"arena", "area", "name"};
        if (empty) expected.emplace_back("empty");
        expected.emplace_back("compare");
        Check(pal4::ScriptBgmCallbacks::IsInArea(0x6000u, runtime) &&
                  runtime.calls == expected,
              "area match and name fallback");
        runtime.calls.clear();
        runtime.comparison = -1;
        Check(!pal4::ScriptBgmCallbacks::IsInArea(0x6000u, runtime),
              "nonzero case-insensitive comparison is false");
    }
}

}  // namespace

int main() {
    try {
        MusicForwarders();
        AreaComparison();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
