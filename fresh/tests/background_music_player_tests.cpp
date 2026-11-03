#include "pal4/background_music_player.h"

#include <bit>
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

struct Call {
    std::string name;
    std::string text;
    std::uint32_t first = 0u;
    std::uint32_t second = 0u;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IBackgroundMusicRuntime {
    std::vector<Call> calls;
    std::string default_track = "";
    std::string script_track;
    std::uint32_t interpreter = 7u;
    bool execution_exception = false;
    std::uint32_t load_result = 91u;
    std::uint32_t property_result = 92u;
    std::uint32_t cleanup_result = 93u;
    std::uint32_t next_handle = 0x1234u;
    bool active = false;

    std::string_view DefaultTrack() override {
        calls.push_back({"default"});
        return default_track;
    }
    void InitializeScript() override { calls.push_back({"initialize"}); }
    std::uint32_t CreateInterpreter(std::string_view entry,
                                    std::string_view script_name,
                                    std::string_view path) override {
        Check(entry == "BGMIniter" &&
                  path == "gamedata\\editData\\script\\music.csb",
              "script constants");
        calls.push_back({"create", std::string(script_name)});
        return interpreter;
    }
    bool ExecuteInterpreter(std::uint32_t value,
                            pal4::BackgroundMusicPlayer& player) override {
        calls.push_back({"execute", "", value});
        player.SetSavedTrack(script_track);
        return execution_exception;
    }
    void HandleException(std::uint32_t value) override {
        calls.push_back({"handle", "", value});
    }
    void ReleaseException(std::uint32_t value) override {
        calls.push_back({"release", "", value});
    }
    void CleanupScripts() override { calls.push_back({"cleanup-scripts"}); }
    std::string_view MusicDirectory() override { return "gamedata\\Music\\"; }
    std::uint32_t PlaySound(std::string_view path, bool restore,
                            bool on_script_finish) override {
        calls.push_back({"load", std::string(path),
                         restore ? 1u : 0u, on_script_finish ? 1u : 0u});
        return next_handle;
    }
    std::uint32_t FadeInAudio(std::uint32_t handle, float fade) override {
        calls.push_back({"fade-in", "", handle,
                         std::bit_cast<std::uint32_t>(fade)});
        return 14u;
    }
    std::uint32_t FadeOutAudio(std::uint32_t handle, float fade) override {
        calls.push_back({"stop", "", handle,
                         std::bit_cast<std::uint32_t>(fade)});
        return 14u;
    }
    std::uint32_t StopAudioImmediately(std::uint32_t handle) override {
        calls.push_back({"stop-now", "", handle});
        return 15u;
    }
    bool IsAudioActive(std::uint32_t handle) override {
        calls.push_back({"active", "", handle});
        return active;
    }
    std::uint8_t PauseAudio(std::uint32_t handle) override {
        calls.push_back({"pause", "", handle});
        return 16u;
    }
    std::uint32_t SetAudioProperty(std::uint32_t offset,
                                   std::uint32_t bits) override {
        calls.push_back({"property", "", offset, bits});
        return property_result;
    }
    std::uint32_t CleanupTemporaryText() override {
        calls.push_back({"cleanup-text"});
        return cleanup_result;
    }
    std::uint8_t SavedTextAssignmentResult(std::string_view text,
                                           bool alias) override {
        calls.push_back({"assign-saved", std::string(text), alias ? 1u : 0u});
        return 17u;
    }
    std::uint32_t CleanupTemporaryPath() override { return load_result; }
};

void SceneChange() {
    pal4::BackgroundMusicPlayer player;
    Runtime runtime;
    player.SetCurrentTrack("OLD");
    runtime.script_track = "NEW";
    Check(player.SetBackgroundMusic("q01", "ignored", runtime) == 93u,
          "scene return");
    Check(player.SavedTrack() == "NEW" && player.CurrentTrack() == "NEW" &&
              player.State() == 0u,
          "scene saved track");
    Check(runtime.calls == std::vector<Call>{
              {"default"}, {"initialize"}, {"create", "Q01_MUSIC"},
              {"execute", "", 7u}, {"cleanup-scripts"},
              {"default"}, {"stop", "", 0u, 0x3f800000u},
              {"load", "gamedata\\Music\\NEW.mp3"},
              {"fade-in", "", 0x1234u, 0x40400000u},
              {"cleanup-text"}},
          "scene script and playback order");

    runtime.calls.clear();
    Check(player.SetBackgroundMusic("q01", "", runtime) == 93u &&
              runtime.calls.size() == 6u &&
              runtime.calls.back().name == "cleanup-text",
          "unchanged track skips fade and load");
}

void ScriptModeGateAndException() {
    pal4::BackgroundMusicPlayer player;
    Runtime runtime;
    player.SetState(2u);
    player.SetMode(0u);
    runtime.script_track = "SCENE";
    runtime.execution_exception = true;
    Check(player.SetBackgroundMusic("map", "", runtime) == 93u &&
              player.State() == 2u && runtime.calls.size() == 8u &&
              runtime.calls[4].name == "handle" &&
              runtime.calls[5].name == "release",
          "active script mode zero keeps its track and handles exception");
    player.SetMode(1u);
    player.SetFadeBits(0x40200000u);
    runtime.calls.clear();
    Check(player.SetBackgroundMusic("map", "", runtime) == 93u &&
              player.State() == 0u &&
              runtime.calls[8] == Call{"stop", "", 0u, 0x40200000u} &&
              runtime.calls[10] == Call{"stop", "", 0u, 0x3f800000u} &&
              runtime.calls[11] ==
                  Call{"load", "gamedata\\Music\\SCENE.mp3"},
          "replace script music using stored fade");
    runtime.interpreter = 0u;
    runtime.script_track.clear();
    runtime.calls.clear();
    Check(player.SetBackgroundMusic("map", "", runtime) == 93u &&
              runtime.calls[3].name == "cleanup-scripts" &&
              runtime.calls[4].name == "default" &&
              runtime.calls[5].name == "stop" &&
              runtime.calls[6].name == "cleanup-text",
          "null interpreter and empty saved track");

    runtime.default_track = "FALLBACK";
    runtime.calls.clear();
    Check(player.SetBackgroundMusic("map", "", runtime) == 93u &&
              player.SavedTrack() == "FALLBACK" &&
              player.CurrentTrack() == "FALLBACK" &&
              runtime.calls.size() == 7u &&
              runtime.calls[5].name == "stop",
          "default track survives a missing interpreter and matches current");
}

void CombatAndScriptTrack() {
    pal4::BackgroundMusicPlayer player;
    Runtime runtime;
    runtime.script_track = "BATTLE";
    Check(player.RestoreNormalBackgroundMusic("boss", runtime) == 93u &&
              player.State() == 1u &&
              runtime.calls[2] == Call{"create", "BOSS_COMBAT_MUSIC"} &&
              runtime.calls[6] == Call{"stop", "", 0u, 0x3f800000u} &&
              runtime.calls[7] ==
                  Call{"load", "gamedata\\Music\\BATTLE.mp3"},
          "combat music script, stop and immediate load");
    runtime.script_track.clear();
    runtime.calls.clear();
    Check(player.RestoreNormalBackgroundMusic("boss", runtime) == 93u &&
              runtime.calls[6] ==
                  Call{"stop", "", 0x1234u, 0x3f800000u} &&
              runtime.calls[7].name == "cleanup-text",
          "empty combat track only stops audio");
    runtime.calls.clear();
    Check(player.PlayScriptTrack(std::string("CUTSCENE"), 1u, 2.5f,
                                 0x3f000000u, runtime) == 91u &&
              player.PendingTrack() == "CUTSCENE" && player.State() == 2u &&
              player.Mode() == 1u && player.FadeBits() == 0x3f000000u &&
              runtime.calls == std::vector<Call>{
                  {"default"}, {"stop", "", 0x1234u, 0x3f800000u},
                  {"load", "gamedata\\Music\\CUTSCENE.mp3", 1u, 1u},
                  {"fade-in", "", 0x1234u, 0x40200000u}},
          "script music one shot mode");
    runtime.calls.clear();
    const auto& alias = player.PendingTrack();
    Check(player.PlayScriptTrack(alias, 3u, 0.0f, 0u, runtime) == 91u &&
              player.PendingTrack() == "CUTSCENE" &&
              runtime.calls == std::vector<Call>{
                  {"default"}, {"stop", "", 0x1234u, 0x3f800000u},
                  {"load", "gamedata\\Music\\CUTSCENE.mp3", 0u, 1u}},
          "self source remains valid and stop clears current track");
    runtime.calls.clear();
    Check(player.PlayScriptTrack(std::string("NEXT"), 3u, 0.0f, 0u,
                                 runtime) == 91u &&
              player.CurrentTrack() == "NEXT" &&
              runtime.calls[2] ==
                  Call{"load", "gamedata\\Music\\NEXT.mp3", 0u, 1u},
          "other script modes start a changed track without restore flag");
    runtime.calls.clear();
    Check(player.PlayScriptTrack(std::string{}, 2u, 0.0f, 5u, runtime) == 0u &&
              player.FadeBits() == 5u && runtime.calls.empty(),
          "empty script track never starts audio");
}

void StopModes() {
    pal4::BackgroundMusicPlayer player;
    Runtime runtime;
    player.SetSavedTrack("FIELD");
    player.SetState(2u);
    Check(player.ApplyScriptStopMode(0u, 0.25f, runtime) == 91u &&
              player.State() == 0u &&
              runtime.calls == std::vector<Call>{
                  {"default"}, {"stop", "", 0u, 0x3e800000u},
                  {"load", "gamedata\\Music\\FIELD.mp3"},
                  {"fade-in", "", 0x1234u, 0x40400000u}},
          "mode zero restores saved music");
    runtime.calls.clear();
    Check(player.ApplyScriptStopMode(1u, 1.5f, runtime) == 92u &&
              player.Mode() == 1u && player.FadeBits() == 0x3fc00000u &&
              runtime.calls == std::vector<Call>{
                  {"property", "", 0x1234u, 0x3fc00000u}},
          "mode one applies audio property");
    runtime.calls.clear();
    Check(player.ApplyScriptStopMode(2u, 4.0f, runtime) == 2u &&
              player.Mode() == 2u && player.FadeBits() == 0x40800000u &&
              runtime.calls.empty(), "mode two defers property");
    Check(player.ApplyScriptStopMode(7u, 8.0f, runtime) == 7u &&
              player.Mode() == 2u && runtime.calls.empty(),
          "unknown stop mode is inert");
    player.SetSavedTrack("");
    player.SetState(2u);
    runtime.calls.clear();
    Check(player.ApplyScriptStopMode(0u, 0.5f, runtime) == 0u &&
              player.State() == 2u &&
              runtime.calls == std::vector<Call>{
                  {"default"}, {"stop", "", 0x1234u, 0x3f000000u}},
          "empty saved track stops without clearing script state");
}

void FadeOutAndStopThreshold() {
    pal4::BackgroundMusicPlayer player;
    Runtime runtime;
    player.SetAudioHandle(0x9876u);
    player.SetState(2u);
    Check(player.FadeOutUnlessScript(2.0f, runtime) == 0u &&
              runtime.calls.empty(),
          "script state blocks the fade-out wrapper");
    player.SetState(0u);
    player.SetCurrentTrack("CURRENT");
    Check(player.FadeOutUnlessScript(0.1f, runtime) == 15u &&
              player.CurrentTrack().empty() &&
              runtime.calls == std::vector<Call>{
                  {"default"}, {"stop-now", "", 0x9876u}},
          "fade threshold uses immediate stop at 0.1 seconds");
    runtime.calls.clear();
    runtime.default_track = "DEFAULT";
    Check(player.StopCurrentWithFade(0.10001f, runtime) == 14u &&
              player.CurrentTrack() == "DEFAULT" &&
              runtime.calls == std::vector<Call>{
                  {"default"},
                  {"stop", "", 0x9876u,
                   std::bit_cast<std::uint32_t>(0.10001f)}},
          "greater fade time selects fade-out and resets current text");
}

void TrackPathAndFadeIn() {
    pal4::BackgroundMusicPlayer player;
    Runtime runtime;
    player.SetState(2u);
    Check(player.LoadTrack("intro.MP3", 0.1f, true, runtime) == 91u &&
              player.CurrentTrack() == "intro.MP3" &&
              player.AudioHandle() == 0x1234u &&
              runtime.calls == std::vector<Call>{
                  {"load", "gamedata\\Music\\intro.MP3", 1u, 1u}},
          "existing case-insensitive mp3 extension and fade boundary");
    runtime.calls.clear();
    Check(player.LoadTrack("intro.MP3", 5.0f, true, runtime) == 0u &&
              runtime.calls.empty(), "same current track does not restart");
    runtime.calls.clear();
    player.SetState(0u);
    Check(player.LoadTrack("ambient.ogg", 0.11f, false, runtime) == 91u &&
              runtime.calls == std::vector<Call>{
                  {"load", "gamedata\\Music\\ambient.ogg.mp3"},
                  {"fade-in", "", 0x1234u,
                   std::bit_cast<std::uint32_t>(0.11f)}},
          "other extension appends mp3 and positive fade applies");
    runtime.calls.clear();
    Check(player.LoadTrack("town", 0.0f, false, runtime) == 91u &&
              runtime.calls == std::vector<Call>{
                  {"load", "gamedata\\Music\\town.mp3"}},
          "missing extension appends mp3 without fade-in");
}

void ResumeAndSpecialMusic() {
    pal4::BackgroundMusicPlayer player;
    Runtime runtime;
    player.SetAudioHandle(0x4444u);
    runtime.active = true;
    Check(player.LoadAndPlayTrack(runtime) == 16u &&
              runtime.calls == std::vector<Call>{
                  {"active", "", 0x4444u}, {"pause", "", 0x4444u}},
          "active audio pauses without creating a new sound");
    runtime.active = false;
    runtime.calls.clear();
    Check(player.LoadAndPlayTrack(runtime) == 0u &&
              runtime.calls == std::vector<Call>{{"active", "", 0x4444u}},
          "inactive empty current track does not play");
    player.SetCurrentTrack("field");
    runtime.calls.clear();
    Check(player.LoadAndPlayTrack(runtime) == 91u &&
              player.AudioHandle() == 0x1234u &&
              runtime.calls == std::vector<Call>{
                  {"active", "", 0x4444u},
                  {"load", "gamedata\\Music\\field"}},
          "resume path keeps original suffix behavior");

    player.SetState(2u);
    player.SetFadeBits(0x3f000000u);
    runtime.calls.clear();
    Check(player.FadeInSpecialBackgroundMusic("special", 2.0f,
                                               true, runtime) == 91u &&
              player.State() == 3u &&
              runtime.calls == std::vector<Call>{
                  {"default"}, {"stop", "", 0x1234u, 0x3f000000u},
                  {"load", "gamedata\\Music\\special.mp3", 1u, 0u},
                  {"fade-in", "", 0x1234u, 0x40000000u}},
          "special music uses stored script fade then state-three load");
    runtime.calls.clear();
    Check(player.FadeInSpecialBackgroundMusic("other", 0.0f,
                                               false, runtime) == 91u &&
              runtime.calls[1] ==
                  Call{"stop", "", 0x1234u, 0x3f800000u} &&
              runtime.calls[2] ==
                  Call{"load", "gamedata\\Music\\other.mp3"},
          "non-script special music uses fixed one-second fade");
}

void SavedTextAndFinishCallback() {
    pal4::BackgroundMusicPlayer player;
    Runtime runtime;
    const std::string track = "FIELD";
    Check(player.AssignSavedTrack(track, runtime) == 17u &&
              player.SavedTrack() == track &&
              runtime.calls == std::vector<Call>{
                  {"assign-saved", "FIELD"}},
          "saved track assignment copies a distinct source");
    runtime.calls.clear();
    const auto& alias = player.SavedTrack();
    Check(player.AssignSavedTrack(alias, runtime) == 17u &&
              player.SavedTrack() == "FIELD" &&
              runtime.calls == std::vector<Call>{
                  {"assign-saved", "FIELD", 1u}},
          "self assignment preserves text after legacy detach path");
    player.SetSavedTrack("FIELD");
    player.SetState(2u);
    player.SetMode(2u);
    runtime.calls.clear();
    Check(player.OnScriptTrackFinished(runtime) ==
                  reinterpret_cast<std::uintptr_t>(&player) &&
              runtime.calls.empty(),
          "non-one-shot script completion returns player unchanged");
    player.SetMode(1u);
    player.SetFadeBits(0x3e800000u);
    player.SetAudioHandle(0x5678u);
    Check(player.OnScriptTrackFinished(runtime) == 91u &&
              player.State() == 0u && player.CurrentTrack() == "FIELD" &&
              runtime.calls == std::vector<Call>{
                  {"default"}, {"stop", "", 0x5678u, 0x3e800000u},
                  {"load", "gamedata\\Music\\FIELD.mp3"},
                  {"fade-in", "", 0x1234u, 0x40400000u}},
          "one-shot completion restores scene music using stored fade");
}

}  // namespace

int main() {
    try {
        SceneChange();
        ScriptModeGateAndException();
        CombatAndScriptTrack();
        StopModes();
        FadeOutAndStopThreshold();
        TrackPathAndFadeIn();
        ResumeAndSpecialMusic();
        SavedTextAndFinishCallback();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
