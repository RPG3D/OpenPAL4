#include "pal4/combat_end_sound_playback.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatEndSoundRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::string> text;
    std::vector<std::string> calls;
    std::uint32_t next_text = 0x7000u;
    bool inactive = false;
    bool fail_audio = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    std::uint32_t CombatSystemInstance() override {
        calls.emplace_back("system");
        return 0x2000u;
    }
    bool IsCombatSystemInactive(std::uint32_t system) override {
        Check(system == 0x2000u, "combat system identity");
        calls.emplace_back("inactive");
        return inactive;
    }
    std::uint32_t BackgroundMusicPlayer() override {
        calls.emplace_back("bgm");
        return 0x3000u;
    }
    void FadeInSpecialMusic(std::uint32_t player,
                            std::uint32_t record, float seconds,
                            std::uint32_t mode) override {
        Check(player == 0x3000u && seconds == 0.0f,
              "special music player and fade");
        calls.emplace_back("fade:" + std::to_string(record) + ":" +
                           std::to_string(mode));
    }
    void RestoreNormalMusic(std::uint32_t player,
                            std::uint32_t music) override {
        Check(player == 0x3000u, "normal music player");
        calls.emplace_back("restore:" + std::to_string(music));
    }
    std::uint32_t CreateText(std::string_view literal) override {
        const auto record = next_text;
        next_text += 0x100u;
        text[record] = literal;
        words[record + 8u] = static_cast<std::uint32_t>(literal.size());
        calls.emplace_back("text:" + std::string(literal));
        return record;
    }
    void AppendText(std::uint32_t destination,
                    std::uint32_t source) override {
        text[destination] += text[source];
        calls.emplace_back("append:" + text[destination]);
    }
    std::uint32_t ReleaseText(std::uint32_t record) override {
        calls.emplace_back("release:" + std::to_string(record));
        return 0x77u;
    }
    std::uint32_t AudioManagerInstance() override {
        calls.emplace_back("audio");
        return 0x4000u;
    }
    std::uint32_t TemporaryOutputSlot() override {
        calls.emplace_back("output");
        return 0x5000u;
    }
    void PlayAudio(std::uint32_t manager, std::uint32_t output,
                   std::uint32_t path_record,
                   std::uint32_t mode) override {
        Check(manager == 0x4000u && mode == 1u &&
                  text[path_record].find("gamedata\\palSound\\") == 0u,
              "audio manager, mode and path prefix");
        calls.emplace_back("play:" + std::to_string(output) + ":" +
                           text[path_record]);
        if (fail_audio) throw std::runtime_error("audio");
    }
};

bool Contains(const std::vector<std::string>& calls,
              const std::string& expected) {
    for (const auto& call : calls)
        if (call == expected) return true;
    return false;
}

void BattleEndBranches() {
    Runtime special;
    special.words[0x2000u + 460u] = 0x7001u;
    Check(pal4::CombatEndSoundPlayback::OnBattleEnd(
              0x1000u, special) == 0x77u &&
              Contains(special.calls, "fade:8644:0") &&
              Contains(special.calls,
                       "play:20480:gamedata\\palSound\\WA14.wav") &&
              Contains(special.calls, "release:28928") &&
              Contains(special.calls, "release:28672"),
          "active battle fades special music and plays WA14 effect");

    Runtime normal;
    normal.words[0x2000u + 128u] = 0x1234u;
    static_cast<void>(pal4::CombatEndSoundPlayback::OnBattleEnd(
        0x1000u, normal));
    Check(Contains(normal.calls, "restore:4660"),
          "no configured special music restores normal track");

    Runtime inactive;
    inactive.inactive = true;
    static_cast<void>(pal4::CombatEndSoundPlayback::OnBattleEnd(
        0x1000u, inactive));
    Check(!Contains(inactive.calls, "bgm") &&
              inactive.words[0x5000u] == 0u &&
              !Contains(inactive.calls,
                        "play:20480:gamedata\\palSound\\WA14.wav"),
          "inactive battle skips music and clears effect output");
}

void MusicCueAndEffect() {
    for (std::uint32_t cue = 0u; cue <= 2u; ++cue) {
        Runtime runtime;
        runtime.words[0x1000u] = cue;
        Check(pal4::CombatEndSoundPlayback::OnMusicCue(
                  0x1000u, runtime) == 0x77u &&
                  Contains(runtime.calls,
                      cue == 0u ? "text:wa16.mp3" : "text:wa15.mp3") &&
                  Contains(runtime.calls, "fade:28672:1") &&
                  Contains(runtime.calls, "release:28672"),
              "music cue chooses exact lowercase filename and fade mode");
    }
    Runtime other;
    other.words[0x1000u] = 0x103u;
    Check(pal4::CombatEndSoundPlayback::OnMusicCue(
              0x1000u, other) == 1u && other.calls.empty(),
          "unsupported cue returns low byte of cue minus two");

    Runtime empty;
    Check(pal4::CombatEndSoundPlayback::PlayEffect(
              0x5000u, 0x6000u, 1u, empty) == 0x5000u &&
              empty.words[0x5000u] == 0u && empty.calls.size() == 1u,
          "empty sound name clears output without system lookup");
}

void AudioFailureReleasesBothTexts() {
    Runtime runtime;
    runtime.fail_audio = true;
    bool raised = false;
    try {
        static_cast<void>(pal4::CombatEndSoundPlayback::OnBattleEnd(
            0x1000u, runtime));
    } catch (const std::runtime_error&) {
        raised = true;
    }
    Check(raised &&
              Contains(runtime.calls, "release:28928") &&
              Contains(runtime.calls, "release:28672"),
          "audio failure releases path and effect text");
}

}  // namespace

int main() {
    try {
        BattleEndBranches();
        MusicCueAndEffect();
        AudioFailureReleasesBothTexts();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
