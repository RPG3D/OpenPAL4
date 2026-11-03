#include "pal4/script_movie_playback.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptMovieRuntime {
public:
    pal4::MovieWaitState wait{};
    std::vector<std::string> calls;
    bool player_ready = false;
    bool audio_ready = false;
    bool allocation_fails = false;
    float expected_volume = 0.0f;
    std::uint8_t expected_return_to_game = 0u;
    std::string_view expected_path = "gamedata\\video\\opening.bik";

    std::uint32_t Player() override {
        calls.emplace_back("player");
        return 0x4000u;
    }
    bool PlayerReady(std::uint32_t player) override {
        Check(player == 0x4000u, "Bink player readiness receiver");
        calls.emplace_back("player ready");
        return player_ready;
    }
    std::uint32_t AudioManager() override {
        calls.emplace_back("audio manager");
        return 0x5000u;
    }
    bool AudioReady(std::uint32_t audio) override {
        Check(audio == 0x5000u, "audio readiness receiver");
        calls.emplace_back("audio ready");
        return audio_ready;
    }
    void InitializeAudio(std::uint32_t audio,
                         std::uint32_t mode) override {
        Check(audio == 0x5000u && mode == 0u,
              "audio initialization mode");
        calls.emplace_back("initialize audio");
    }
    std::uint32_t AudioDevice(std::uint32_t audio) override {
        Check(audio == 0x5000u, "audio device manager");
        calls.emplace_back("audio device");
        return 0x6000u;
    }
    void SetSoundSystem(std::uint32_t player, std::uint32_t mode,
                        std::uint32_t device) override {
        Check(player == 0x4000u && mode == 1u && device == 0x6000u,
              "Bink sound system operands");
        calls.emplace_back("sound system");
    }
    void CloseVideo(std::uint32_t player) override {
        Check(player == 0x4000u, "Bink close receiver");
        calls.emplace_back("close video");
    }
    void OpenVideo(std::uint32_t player, std::string_view path,
                   std::uint32_t mode) override {
        Check(player == 0x4000u && path == expected_path && mode == 1u,
              "Bink open path and mode");
        calls.emplace_back("open video");
    }
    void InstallCompletionCallback(std::uint32_t player,
                                   std::uint32_t offset,
                                   std::uint32_t address) override {
        Check(player == 0x4000u && offset == 0x200394u &&
                  address == 0x5e6970u,
              "Bink completion callback slot");
        calls.emplace_back("install callback");
    }
    pal4::MovieWaitState* AllocateWait(std::uint32_t bytes) override {
        Check(bytes == 0x28u, "movie wait allocation size");
        calls.emplace_back("allocate wait");
        return allocation_fails ? nullptr : &wait;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x7000u;
    }
    void ConstructWait(pal4::MovieWaitState& state,
                       std::uint8_t mode,
                       std::uint32_t queue) override {
        Check(&state == &wait && mode == 1u && queue == 0x7054u,
              "movie wait constructor");
        calls.emplace_back("construct wait");
    }
    void FlushWait(pal4::MovieWaitState& state) override {
        Check(&state == &wait && state.vtable_address == 0x8495e8u &&
                  state.return_to_game == expected_return_to_game,
              "movie wait state before flush");
        calls.emplace_back("flush wait");
    }
    void CloseWait(pal4::MovieWaitState& state) override {
        Check(&state == &wait, "movie wait close receiver");
        calls.emplace_back("close wait");
    }
    float ReadVolume(std::uint32_t audio) override {
        Check(audio == 0x5000u, "saved audio volume receiver");
        calls.emplace_back("read volume");
        return 0.75f;
    }
    void WriteVolume(std::uint32_t audio, float value) override {
        Check(audio == 0x5000u && value == expected_volume,
              "movie audio volume write");
        calls.emplace_back("write volume");
    }
    std::uint32_t ChannelCount(std::uint32_t audio) override {
        Check(audio == 0x5000u, "channel collection receiver");
        calls.emplace_back("channel count");
        return 3u;
    }
    std::uint32_t ChannelAt(std::uint32_t audio,
                            std::uint32_t index) override {
        Check(audio == 0x5000u && index < 3u,
              "channel collection index");
        calls.emplace_back("channel:" + std::to_string(index));
        return index == 0u ? 0x9000u :
               index == 1u ? 0u : 0x9002u;
    }
    std::uint32_t ChannelType(std::uint32_t channel) override {
        Check(channel == 0x9000u || channel == 0x9002u,
              "non-null channel type receiver");
        calls.emplace_back("type");
        return channel == 0x9000u ? 2u : 1u;
    }
    void InvokeChannelSlot44(std::uint32_t channel) override {
        Check(channel == 0x9000u, "type-two channel callback");
        calls.emplace_back("channel slot 44");
    }
    void AddHandler(std::uint32_t queue,
                    pal4::MovieWaitState* state) override {
        Check(queue == 0x7054u &&
                  state == (allocation_fails ? nullptr : &wait),
              "movie wait queue");
        calls.emplace_back("add handler");
    }
    std::uint32_t CreateStateDocument() override {
        calls.emplace_back("create document");
        return 0xe000u;
    }
    std::uint8_t EnterState(std::uint32_t state,
                            std::uint32_t document) override {
        Check((state == 12u && document == 0xe000u) ||
                  (state == 0u && document == 0u),
              "movie game-state transition");
        calls.emplace_back(state == 12u ? "enter movie" : "enter game");
        return 0x5au;
    }
    void DestroyStateDocument(std::uint32_t document) override {
        Check(document == 0xe000u, "movie document destructor");
        calls.emplace_back("destroy document");
    }
    std::uint32_t CleanupPath() override {
        calls.emplace_back("cleanup path");
        return 0x1234u;
    }
    void ResetScreenEffect() override {
        calls.emplace_back("reset screen");
    }
};

void PlayCases() {
    pal4::MoviePlaybackSharedState shared{1u};
    Runtime runtime;
    pal4::ScriptMoviePlayback playback(shared, runtime);
    Check(playback.Play("opening.bik") == 0x1234u &&
              shared.finished == 0u && runtime.wait.saved_volume == 0.75f &&
              runtime.wait.return_to_game == 0u &&
              runtime.calls == std::vector<std::string>{
                  "player", "player ready", "audio manager",
                  "audio ready", "audio manager", "initialize audio",
                  "audio manager", "audio device", "player",
                  "sound system", "player", "close video", "player",
                  "open video", "player", "install callback",
                  "allocate wait", "executor", "construct wait",
                  "flush wait", "audio manager", "read volume",
                  "audio manager", "write volume", "channel count",
                  "channel:0", "type", "channel slot 44", "channel:1",
                  "channel:2", "type", "executor", "add handler",
                  "create document", "enter movie", "destroy document",
                  "cleanup path"},
          "movie startup initializes sound, mutes channels and enters state 12");

    pal4::MoviePlaybackSharedState ready_shared{1u};
    Runtime ready;
    ready.player_ready = true;
    pal4::ScriptMoviePlayback ready_playback(ready_shared, ready);
    Check(ready_playback.Play("opening.bik") == 0x1234u &&
              ready.calls[0] == "player" &&
              ready.calls[1] == "player ready" &&
              ready.calls[2] == "player" &&
              ready.calls[3] == "close video" &&
              ready.calls.back() == "cleanup path",
          "ready Bink player skips audio sound-system initialization");

    pal4::MoviePlaybackSharedState audio_ready_shared{1u};
    Runtime audio_ready;
    audio_ready.audio_ready = true;
    audio_ready.expected_path = "gamedata\\video\\";
    pal4::ScriptMoviePlayback audio_ready_playback(
        audio_ready_shared, audio_ready);
    Check(audio_ready_playback.Play("") == 0x1234u &&
              audio_ready.calls[2] == "audio manager" &&
              audio_ready.calls[3] == "audio ready" &&
              audio_ready.calls[4] == "audio manager" &&
              audio_ready.calls[5] == "audio device" &&
              audio_ready.calls[7] == "sound system",
          "ready audio skips initialization and empty movie name keeps prefix");

    pal4::MoviePlaybackSharedState failed_shared{1u};
    Runtime failed;
    failed.player_ready = true;
    failed.allocation_fails = true;
    pal4::ScriptMoviePlayback failed_playback(failed_shared, failed);
    Check(failed_playback.Play("opening.bik") == 0x1234u &&
              failed_shared.finished == 1u &&
              failed.calls == std::vector<std::string>{
                  "player", "player ready", "player", "close video",
                  "player", "open video", "player", "install callback",
                  "allocate wait", "executor", "add handler",
                  "create document", "enter movie", "destroy document",
                  "cleanup path"},
          "allocation failure still queues null and enters movie state");
}

void FinalMovieCases() {
    pal4::MoviePlaybackSharedState shared{1u};
    Runtime runtime;
    runtime.expected_return_to_game = 1u;
    pal4::ScriptMoviePlayback playback(shared, runtime);
    Check(playback.PlayFinal("opening.bik") == 0x1234u &&
              shared.finished == 0u &&
              runtime.wait.return_to_game == 1u &&
              runtime.wait.saved_volume == 0.75f &&
              runtime.calls.size() >= 4u &&
              runtime.calls[runtime.calls.size() - 4u] == "enter movie" &&
              runtime.calls[runtime.calls.size() - 3u] == "enter movie" &&
              runtime.calls[runtime.calls.size() - 2u] == "destroy document" &&
              runtime.calls.back() == "cleanup path",
          "final movie enters state twelve twice and marks return to game");

    runtime.calls.clear();
    playback.MarkFinished();
    runtime.expected_volume = 0.75f;
    Check(playback.Update(runtime.wait) == 0x5au &&
              runtime.calls[runtime.calls.size() - 2u] == "reset screen" &&
              runtime.calls.back() == "enter game",
          "final movie completion returns to game");

    pal4::MoviePlaybackSharedState failed_shared{1u};
    Runtime failed;
    failed.player_ready = true;
    failed.allocation_fails = true;
    failed.expected_path = "gamedata\\video\\";
    pal4::ScriptMoviePlayback failed_playback(failed_shared, failed);
    Check(failed_playback.PlayFinal("") == 0x1234u &&
              failed_shared.finished == 1u &&
              failed.calls == std::vector<std::string>{
                  "player", "player ready", "player", "close video",
                  "player", "open video", "player", "install callback",
                  "allocate wait", "executor", "add handler",
                  "create document", "enter movie", "enter movie",
                  "destroy document", "cleanup path"},
          "final movie still enters state twice after null wait allocation");
}

void CallbackCases() {
    pal4::MoviePlaybackSharedState shared{};
    Runtime runtime;
    runtime.wait.saved_volume = 0.75f;
    runtime.expected_volume = 0.75f;
    pal4::ScriptMoviePlayback playback(shared, runtime);
    Check(playback.Update(runtime.wait) == 0u && runtime.calls.empty(),
          "movie update waits for completion callback");
    playback.MarkFinished();
    Check(shared.finished == 1u && runtime.calls.empty(),
          "Bink completion callback only sets global finish flag");
    Check(playback.Update(runtime.wait) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "close wait", "audio manager", "write volume",
                  "channel count", "channel:0", "type", "channel slot 44",
                  "channel:1", "channel:2", "type"},
          "completed movie restores audio and leaves no state transition by default");

    runtime.calls.clear();
    runtime.wait.return_to_game = 1u;
    Check(playback.Update(runtime.wait) == 0x5au &&
              runtime.calls[runtime.calls.size() - 2u] == "reset screen" &&
              runtime.calls.back() == "enter game",
          "return flag resets screen effect and re-enters game state");

    shared.finished = 0u;
    runtime.calls.clear();
    runtime.wait.return_to_game = 0u;
    Check(playback.Stop(runtime.wait) == 0u && shared.finished == 1u &&
              runtime.calls == std::vector<std::string>{
                  "close wait", "player", "close video", "audio manager",
                  "write volume", "channel count", "channel:0", "type",
                  "channel slot 44", "channel:1", "channel:2", "type"},
          "explicit stop closes Bink and restores audio");

    runtime.calls.clear();
    runtime.wait.return_to_game = 1u;
    Check(playback.Stop(runtime.wait) == 0x5au &&
              runtime.calls[runtime.calls.size() - 2u] == "reset screen" &&
              runtime.calls.back() == "enter game",
          "explicit stop honors return-to-game flag");
}

}  // namespace

int main() {
    try {
        PlayCases();
        FinalMovieCases();
        CallbackCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
