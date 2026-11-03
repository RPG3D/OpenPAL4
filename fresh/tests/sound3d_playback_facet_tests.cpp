#include "pal4/sound3d_playback_facet.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISound3DPlaybackRuntime {
    std::vector<std::string> events;
    bool initialized = false;
    bool audio_active = true;
    std::uint8_t is_playing = 1u;
    std::uint8_t play_existing_result = 0x5au;
    std::string_view expected_path;

    bool AudioManagerInitialized() override {
        events.emplace_back("manager-ready");
        return initialized;
    }
    void InitializeAudioManager(std::uint32_t mode) override {
        Check(mode == 0u, "audio manager initialized with mode zero");
        events.emplace_back("manager-init");
    }
    std::string_view ReadSoundPath(std::uint32_t pointer) override {
        Check(pointer == 0x4000u, "sound path data pointer");
        events.emplace_back("read-path");
        return "file.wav";
    }
    std::uint32_t PlayMusic(std::string_view path,
                            std::uint8_t volume) override {
        Check(path == expected_path && volume == 0x81u,
              "music path and byte volume");
        events.emplace_back("play-music");
        return 0x1111u;
    }
    std::uint32_t ObjectTransform(std::uint32_t frame) override {
        Check(frame == 0x5000u, "3D sound frame");
        events.emplace_back("transform");
        return 0x6000u;
    }
    std::uint32_t Play3DSound(std::string_view path,
                              std::uint32_t position_address,
                              std::uint8_t volume, float max_distance,
                              float min_distance) override {
        Check(path == expected_path && position_address == 0x6030u &&
                  volume == 0x81u && max_distance == 600.0f &&
                  min_distance == 300.0f,
              "3D path, transform translation, volume and distances");
        events.emplace_back("play-3d");
        return 0x2222u;
    }
    bool AudioActive(std::uint32_t handle) override {
        Check(handle == 0x1111u || handle == 0x2222u,
              "new audio handle checked after storage");
        events.emplace_back("audio-active");
        return audio_active;
    }
    std::uint8_t IsPlaying(std::uint32_t handle) override {
        Check(handle == 0x2222u, "stop checks current audio handle");
        events.emplace_back("is-playing");
        return is_playing;
    }
    std::uint8_t PlayExisting(std::uint32_t handle) override {
        Check(handle == 0x2222u, "existing sample dispatch handle");
        events.emplace_back("play-existing");
        return play_existing_result;
    }
};

void TestPlayMusic() {
    pal4::Sound3DObjectState state{};
    Runtime runtime;
    Check(pal4::Sound3DPlaybackFacet::Play(state, 0x81u, runtime) == 0u &&
              runtime.events.empty(), "inactive object does not access audio");
    state.words[1u] = 1u;
    state.words[14u] = 0x00007e00u;
    Check(pal4::Sound3DPlaybackFacet::Play(state, 0x81u, runtime) == 0x7eu &&
              runtime.events.empty(), "already playing returns flag byte");
    state.words[14u] = 0u;
    runtime.expected_path = "";
    Check(pal4::Sound3DPlaybackFacet::Play(state, 0x81u, runtime) == 1u &&
              state.words[15u] == 0x1111u &&
              ((state.words[14u] >> 8u) & 0xffu) == 1u,
          "missing path falls back to empty music name");
    Check(runtime.events == std::vector<std::string>{
              "manager-ready", "manager-init", "play-music", "audio-active"},
          "manager initialization and music call order");
    runtime.events.clear();
    runtime.initialized = true;
    runtime.audio_active = false;
    runtime.expected_path = "file.wav";
    state.words[17u] = 0x4000u;
    state.words[14u] = 0u;
    Check(pal4::Sound3DPlaybackFacet::Play(state, 0x81u, runtime) == 0u &&
              state.words[15u] == 0x1111u &&
              ((state.words[14u] >> 8u) & 0xffu) == 0u,
          "audio lookup failure clears playing flag");
    Check(runtime.events == std::vector<std::string>{
              "manager-ready", "read-path", "play-music", "audio-active"},
          "initialized manager reads stored path");
}

void TestPlayAndStop3D() {
    pal4::Sound3DObjectState state{};
    state.words[1u] = 1u;
    state.words[13u] = 0x5000u;
    state.words[14u] = 0x00000001u;
    state.words[17u] = 0x4000u;
    Runtime runtime;
    runtime.initialized = true;
    runtime.expected_path = "file.wav";
    Check(pal4::Sound3DPlaybackFacet::Play(state, 0x81u, runtime) == 1u &&
              state.words[15u] == 0x2222u &&
              state.words[14u] == 0x00000101u,
          "3D play stores handle and preserves mode byte");
    Check(runtime.events == std::vector<std::string>{
              "manager-ready", "read-path", "transform", "play-3d",
              "audio-active"}, "3D play call order");

    runtime.events.clear();
    Check(pal4::Sound3DPlaybackFacet::Stop(state, runtime) == 0x5au &&
              state.words[14u] == 0x00000001u &&
              runtime.events == std::vector<std::string>{
                  "is-playing", "play-existing"},
          "playing sound dispatches existing sample then clears flag");
    runtime.events.clear();
    Check(pal4::Sound3DPlaybackFacet::Stop(state, runtime) == 0u &&
              runtime.events.empty(), "stopped sound has no callbacks");
    state.words[14u] = 0x00000101u;
    runtime.is_playing = 0u;
    Check(pal4::Sound3DPlaybackFacet::Stop(state, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"is-playing"} &&
              state.words[14u] == 0x00000001u,
          "missing active sample still clears playing flag");
    state.words[1u] = 0u;
    runtime.events.clear();
    Check(pal4::Sound3DPlaybackFacet::Stop(state, runtime) == 0u &&
              runtime.events.empty(), "inactive owner skips stop path");
}

}  // namespace

int main() {
    TestPlayMusic();
    TestPlayAndStop3D();
}
