#include "pal4/system_setting_audio.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemSettingAudioRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, float> floats;
    std::map<std::uint32_t, std::uint32_t> types;
    std::vector<std::pair<std::uint32_t, float>> volumes;
    std::vector<std::uint32_t> refreshed;
    std::vector<std::string> calls;
    std::uint32_t manager_count = 0u;
    std::uint32_t slider_reads = 0u;
    std::string sound;
    std::uint32_t sound_mode = 0u;

    Runtime() {
        words[0x4000u + 496u] = 0x5000u;
        words[0x4000u + 500u] = 0x6000u;
        floats[0x5000u + 1200u] = 0.25f;
        floats[0x6000u + 1200u] = 0.625f;
        words[0x7000u + 36u] = 0x8000u;
        words[0x7000u + 40u] = 0x800cu;
        words[0x8000u] = 0xa000u;
        words[0x8004u] = 0u;
        words[0x8008u] = 0xa001u;
        words[0x7000u + 68u] = 0x9000u;
        words[0x7000u + 72u] = 0x9010u;
        words[0x9000u] = 0xb000u;
        words[0x9004u] = 0u;
        words[0x9008u] = 0xb001u;
        words[0x900cu] = 0xb002u;
        types[0xb000u] = 2u;
        types[0xb001u] = 1u;
        types[0xb002u] = 2u;
        words[0x7000u + 52u] = 0x9100u;
        words[0x7000u + 56u] = 0x910cu;
        words[0x9100u] = 0xc000u;
        words[0x9104u] = 0xc001u;
        words[0x9108u] = 0u;
        types[0xc000u] = 1u;
        types[0xc001u] = 2u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    float ReadFloat(std::uint32_t address) override {
        calls.emplace_back("read-float");
        ++slider_reads;
        return floats.at(address);
    }
    void WriteFloat(std::uint32_t address, float value) override {
        calls.emplace_back("write-float");
        floats[address] = value;
    }
    std::uint32_t AudioManager() override {
        calls.emplace_back("audio");
        ++manager_count;
        return 0x7000u;
    }
    void SetStreamVolume(std::uint32_t stream, float value) override {
        calls.emplace_back("volume");
        volumes.emplace_back(stream, value);
    }
    std::uint32_t StreamType(std::uint32_t stream) override {
        calls.emplace_back("type");
        return types.at(stream);
    }
    void RefreshStream(std::uint32_t stream) override {
        calls.emplace_back("refresh");
        refreshed.push_back(stream);
    }
    void PlayMusic(std::string_view resource,
                   std::uint32_t mode) override {
        calls.emplace_back("play");
        sound = resource;
        sound_mode = mode;
    }
};

}  // namespace

int main() {
    Runtime bgm;
    Check(pal4::SystemSettingAudio::OnBgmVolumeChanged(
          0x4000u, 0xdeadbeefu, bgm) == 1u &&
          bgm.manager_count == 1u && bgm.slider_reads == 1u &&
          bgm.floats.at(0x7000u) == 0.75f &&
          bgm.volumes == std::vector<std::pair<std::uint32_t, float>>{
              {0xa000u, 0.75f}, {0xa001u, 0.75f}},
          "BGM inverted level updates every nonnull stream");
    Check(bgm.refreshed.empty(), "BGM does not refresh type-specific streams");

    Runtime sfx;
    Check(pal4::SystemSettingAudio::OnSfxVolumeChanged(
          0x4000u, 0xabcdefu, sfx) == 1u &&
          sfx.manager_count == 2u && sfx.slider_reads == 2u &&
          sfx.floats.at(0x7008u) == 0.375f &&
          sfx.floats.at(0x7004u) == 0.375f &&
          sfx.refreshed == std::vector<std::uint32_t>{
              0xb000u, 0xb002u, 0xc000u} &&
          sfx.volumes.empty(),
          "SFX updates two fields and refreshes only matching stream types");

    Runtime empty;
    empty.words[0x7024u] = 0u;
    empty.words[0x7044u] = 0u;
    empty.words[0x7034u] = 0u;
    (void)pal4::SystemSettingAudio::OnBgmVolumeChanged(0x4000u, 0u, empty);
    (void)pal4::SystemSettingAudio::OnSfxVolumeChanged(0x4000u, 0u, empty);
    Check(empty.volumes.empty() && empty.refreshed.empty(),
          "null stream collections skipped");

    Runtime sound;
    Check(pal4::SystemSettingAudio::PlayButtonSound(
          0x1234u, sound) == 1u &&
          sound.sound == "WA01.wav" && sound.sound_mode == 1u &&
          sound.calls == std::vector<std::string>{"play"},
          "button sound resource and mode");
}
