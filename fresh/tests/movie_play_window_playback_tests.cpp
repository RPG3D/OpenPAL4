#include "pal4/movie_play_window_playback.h"

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

struct Runtime final : pal4::MoviePlayWindowPlaybackRuntimeStub {
    std::vector<std::string> calls;
    pal4::MoviePlayWindowState frame_state{};
    std::string path;
    bool player_sound_initialized{};
    bool audio_initialized{};
    bool mutate_event_words{};
    std::uint32_t event_action = 1u;
    std::uint32_t text_address = 0x3000u;
    std::uint8_t command_result = 6u;

    std::uint32_t BinkPlayer() override {
        calls.emplace_back("player");
        return 0x5000u;
    }
    bool PlayerSoundInitialized(std::uint32_t player) override {
        Check(player == 0x5000u, "sound flag belongs to Bink player");
        calls.emplace_back("sound-check");
        return player_sound_initialized;
    }
    std::uint32_t AudioManager() override {
        calls.emplace_back("audio");
        return 0x6000u;
    }
    bool AudioInitialized(std::uint32_t manager) override {
        Check(manager == 0x6000u, "audio readiness is read from manager");
        calls.emplace_back("audio-check");
        return audio_initialized;
    }
    void InitializeAudio(std::uint32_t manager,
                         std::uint32_t flags) override {
        Check(manager == 0x6000u && flags == 0u,
              "audio system initializes with zero flags");
        calls.emplace_back("initialize-audio");
    }
    std::uint32_t AudioBackend(std::uint32_t manager) override {
        Check(manager == 0x6000u, "sound backend comes from audio manager");
        calls.emplace_back("backend");
        return 0x6060u;
    }
    void SetSoundSystem(std::uint32_t player,
                        std::uint32_t mode,
                        std::uint32_t backend) override {
        Check(player == 0x5000u && mode == 1u && backend == 0x6060u,
              "Bink uses audio backend in mode one");
        calls.emplace_back("sound-system");
    }
    void SetPlayerCallback(std::uint32_t player,
                           std::uint32_t offset,
                           std::uint32_t handler) override {
        Check(player == 0x5000u &&
                  offset == pal4::MoviePlayWindowPlayback::kPlayerCallbackOffset &&
                  (handler == 0u || handler ==
                      pal4::MoviePlayWindowPlayback::kProcessMovieEvent),
              "Bink callback uses observed player word +0x200394");
        calls.emplace_back(handler == 0u ? "clear-callback" : "set-callback");
    }
    void CloseVideo(std::uint32_t player) override {
        Check(player == 0x5000u, "close uses Bink player instance");
        calls.emplace_back("close");
    }
    void OpenVideo(std::uint32_t player,
                   std::uint32_t path_address,
                   std::uint32_t flags) override {
        Check(player == 0x5000u && path_address == 0x4000u && flags == 1u,
              "open uses MoviePlayWindow path pointer and flag one");
        calls.emplace_back("open");
    }
    std::uint32_t TogglePause(std::uint32_t player) override {
        Check(player == 0x5000u, "pause uses Bink player instance");
        calls.emplace_back("pause");
        return 0x99u;
    }
    void HandleWindowEvent(std::string_view name,
                           pal4::MoviePlaybackEventWords& words) override {
        Check(name == "MoviePlayWindow", "event addresses movie window");
        calls.emplace_back("window-event:" + std::to_string(words.first) +
                           ":" + std::to_string(words.second));
        if (mutate_event_words) words.second = 9u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x7000u;
    }
    std::uint8_t ProcessCommand(
        std::uint32_t tree, std::string_view command,
        pal4::MoviePlaybackEventWords& words) override {
        Check(tree == 0x7104u && command == "show_movie_preview" &&
                  words.first == 1u &&
                  words.second == (mutate_event_words ? 9u : 0u),
              "completion dispatches preview with the shared event words");
        calls.emplace_back("preview");
        return command_result;
    }
    std::uint32_t FindFrame(std::uint32_t manager,
                            std::string_view name) override {
        Check(manager == 0x7000u && name == "MoviePlayWindow",
              "playback resolves registered movie window");
        calls.emplace_back("find-frame");
        return 0x8000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        if (address == 0x1000u) return event_action;
        if (address == 0x1004u) return 0x2000u;
        Check(address == 0x2028u, "filename pointer is record +0x28");
        return text_address;
    }
    std::string_view CString(std::uint32_t address) override {
        Check(address == 0x3000u, "nonzero filename pointer is decoded");
        calls.emplace_back("cstring");
        return "intro.bik";
    }
    pal4::MoviePlayWindowState& FrameState(
        std::uint32_t frame) override {
        Check(frame == 0x8000u, "frame state belongs to found movie window");
        calls.emplace_back("frame-state");
        return frame_state;
    }
    void InitializeVideoPath(pal4::MoviePlayWindowState& state,
                             std::string_view prefix) override {
        Check(&state == &frame_state && prefix == "gamedata\\video\\",
              "video path starts at original game-data directory");
        calls.emplace_back("init-path");
        path = prefix;
        state.Write(0x38u, 0x4000u);
    }
    void AppendVideoPath(pal4::MoviePlayWindowState& state,
                         std::string_view suffix) override {
        Check(&state == &frame_state, "append targets frame path member");
        calls.emplace_back("append-path");
        path.append(suffix);
    }
};

pal4::MoviePlayWindowState MakeState() {
    pal4::MoviePlayWindowState state{};
    state.Write(0x38u, 0x4000u);
    return state;
}

void CheckPlayBranches() {
    auto state = MakeState();
    Runtime runtime;
    Check(pal4::MoviePlayWindowPlayback::Play(state, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{
                  "player", "sound-check", "audio", "audio-check",
                  "audio", "initialize-audio", "audio", "backend",
                  "player", "sound-system", "player", "clear-callback",
                  "player", "close", "player", "open", "player",
                  "set-callback"},
          "cold playback initializes audio, clears callback, reopens and binds completion");

    runtime = Runtime{};
    runtime.audio_initialized = true;
    (void)pal4::MoviePlayWindowPlayback::Play(state, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "player", "sound-check", "audio", "audio-check",
              "audio", "backend", "player", "sound-system",
              "player", "clear-callback", "player", "close",
              "player", "open", "player", "set-callback"},
          "ready audio skips only audio-system initialization");

    runtime = Runtime{};
    runtime.player_sound_initialized = true;
    (void)pal4::MoviePlayWindowPlayback::Play(state, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "player", "sound-check", "player", "clear-callback",
              "player", "close", "player", "open", "player",
              "set-callback"},
          "ready Bink sound skips audio-manager path entirely");
}

void CheckEventsAndWrappers() {
    auto state = MakeState();
    Runtime runtime;
    runtime.mutate_event_words = true;
    Check(pal4::MoviePlayWindowPlayback::ProcessMovieEvent(runtime) == 6u &&
              runtime.calls == std::vector<std::string>{
                  "window-event:0:0", "manager", "preview"},
          "movie completion closes window then dispatches preview");
    runtime.calls.clear();
    runtime.command_result = 0u;
    Check(pal4::MoviePlayWindowPlayback::ProcessMovieEvent(runtime) == 0u,
          "completion forwards a false preview-dispatch result");

    runtime = Runtime{};
    runtime.event_action = 0u;
    Check(pal4::MoviePlayWindowPlayback::HandlePlaybackCommand(
              0x1000u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "read:4096", "player", "close"},
          "zero event closes Bink without frame lookup");

    runtime = Runtime{};
    runtime.player_sound_initialized = true;
    Check(pal4::MoviePlayWindowPlayback::HandlePlaybackCommand(
              0x1000u, runtime) == 1u &&
              runtime.path == "gamedata\\video\\intro.bik" &&
              runtime.calls == std::vector<std::string>{
                  "read:4096", "window-event:1:0", "manager",
                  "find-frame", "read:4100", "read:8232",
                  "frame-state", "init-path", "cstring", "append-path",
                  "player", "sound-check", "player", "clear-callback",
                  "player", "close", "player", "open", "player",
                  "set-callback"},
          "start event resolves filename and begins playback in order");

    runtime = Runtime{};
    runtime.text_address = 0u;
    runtime.player_sound_initialized = true;
    Check(pal4::MoviePlayWindowPlayback::HandlePlaybackCommand(
              0x1000u, runtime) == 1u &&
              runtime.path == "gamedata\\video\\" &&
              runtime.calls[8] == "append-path",
          "null filename appends the empty fallback without CString read");

    runtime = Runtime{};
    Check(pal4::MoviePlayWindowPlayback::TogglePause(runtime) == 0x99u &&
              runtime.calls == std::vector<std::string>{"player", "pause"},
          "direct pause helper forwards Bink toggle result");
    runtime.calls.clear();
    Check(pal4::MoviePlayWindowPlayback::StopButton(runtime) == 1u &&
              runtime.calls == std::vector<std::string>{"player", "close"},
          "close button closes video and returns one");
    runtime.calls.clear();
    Check(pal4::MoviePlayWindowPlayback::PauseButton(runtime) == 1u &&
              runtime.calls == std::vector<std::string>{"player", "pause"},
          "pause button discards toggle result and returns one");
    runtime = Runtime{};
    runtime.player_sound_initialized = true;
    Check(pal4::MoviePlayWindowPlayback::PlayButton(state, runtime) == 1u &&
              runtime.calls.back() == "set-callback",
          "play button invokes playback and returns one");
}

}  // namespace

int main() {
    CheckPlayBranches();
    CheckEventsAndWrappers();
    std::cout << "Movie play window playback tests passed\n";
}
