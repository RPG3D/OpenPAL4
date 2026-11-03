#include "pal4/native_miles_digital_driver.h"
#include "pal4/native_background_music_runtime.h"
#include "pal4/native_miles_music_stream.h"
#include "pal4/native_package_archive.h"
#include "pal4/native_script_music_runtime.h"
#include "pal4/script_music_transport.h"

#include <iostream>
#include <stdexcept>
#include <chrono>
#include <bit>
#include <cmath>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Check(argc == 2, "provide original PAL4 installation path");
        pal4::NativeMilesDigitalDriver driver(argv[1]);
        Check(driver.Open(), "open original Miles digital driver");
        driver.SetMasterVolume(0.0f);
        pal4::NativePackageArchive archive;
        Check(archive.Open((std::string(argv[1]) +
            "\\gamedata\\Music\\p01-1.smp").c_str()),
            "open original music package");
        const auto mp3 = archive.ReadLoadedMember("P01-1.mp3");
        Check(mp3 && mp3->size() > 4u && (*mp3)[0] == 0xffu &&
                  ((*mp3)[1] & 0xe0u) == 0xe0u,
              "original encrypted music decodes to an MP3 frame");
        (void)archive.Close();
        pal4::NativeMilesMusicStream music(argv[1], driver);
        Check(!music.Open("../P01-1.mp3") && !music.Active(),
              "reject music path outside the original Music directory");
        if (!music.Open("P01-1.mp3"))
            throw std::runtime_error("open original P01-1.smp: " +
                                     music.LastError());
        music.SetVolume(0.35f);
        Check(music.VolumeLevels() == std::array<float, 2>{0.35f, 0.35f},
              "live Miles music volume changes independently of muted driver");
        bool rejected = false;
        try { music.SetVolume(-0.1f); } catch (const std::invalid_argument&) { rejected = true; }
        Check(rejected && music.VolumeLevels()[0] == 0.35f, "invalid music level preserves live volume");
        Check(music.Active() && music.Pause() && music.Resume(),
              "original Miles DLL plays and controls packaged music");
        music.Stop();
        Check(!music.Active() && !music.Pause() && !music.Resume(),
              "music stream releases its Miles and temporary-file handles");
        pal4::NativeScriptMusicRuntime script_music(music, "P01-1.mp3");
        Check(script_music.Start() && music.Active(),
              "start original background music for script callback");

        Check(music.Active() &&
                  pal4::ScriptMusicTransport::Pause(script_music) == 1u &&
                  !music.Active() &&
                  pal4::ScriptMusicTransport::Pause(script_music) == 0u &&
                  pal4::ScriptMusicTransport::Resume(script_music) == 1u &&
                  music.Active(),
              "script music pause and resume control the original Miles stream");
        music.Stop();
        if (!music.Open("P23.mp3"))
            throw std::runtime_error("open original P23.smp: " +
                                     music.LastError());
        Check(music.VolumeLevels()[0] == 0.35f, "reopened music inherits current BGM setting");
        Check(music.FadeIn(0.1f) && music.Active(),
              "Q01 music starts a native Miles fade in");
        music.Update(0.05f);
        Check(music.Active() && std::abs(music.VolumeLevels()[0] - 0.175f) < 0.0001f,
              "fade targets configured BGM level while digital driver remains muted");
        music.Update(0.1f);
        Check(music.Active() && music.FadeOut(0.1f),
              "Q01 music reaches playback before fade out");
        music.Update(0.05f);
        Check(music.Active(), "Q01 music remains active during fade out");
        music.Update(0.1f);
        Check(!music.Active(), "Q01 music fade out releases its stream");
        Check(script_music.Start(), "restart opening music for scene transition");
        pal4::NativeBackgroundMusicRuntime background(argv[1], driver, music);
        background.SetVolume(0.6f);
        pal4::BackgroundMusicPlayer player;
        player.SetCurrentTrack("P01-1");
        player.SetAudioHandle(
            pal4::NativeBackgroundMusicRuntime::kOpeningHandle);
        (void)player.PlayScriptTrack("P23", 2u, 0.0f,
                                     std::bit_cast<std::uint32_t>(2.5f),
                                     background);
        const auto scene_handle = player.AudioHandle();
        Check(scene_handle != 0u && scene_handle !=
                  pal4::NativeBackgroundMusicRuntime::kOpeningHandle &&
                  player.PendingTrack() == "P23" &&
                  background.IsAudioActive(scene_handle) && music.Active(),
              "Q01 script starts P23 while opening music fades independently");
        for (auto* stream : background.Streams())
            Check(stream->VolumeLevels()[0] == 0.6f, "new and opening music inherit background setting");
        background.SetVolume(0.4f);
        for (auto* stream : background.Streams())
            Check(stream->VolumeLevels()[0] == 0.4f, "BGM update reaches both live crossfade streams");
        background.Update(0.5f);
        Check(music.Active() && background.IsAudioActive(scene_handle),
              "both original Miles streams remain live during crossfade");
        background.Update(0.6f);
        Check(!music.Active() && background.IsAudioActive(scene_handle),
              "opening stream releases after fade while P23 keeps playing");
        Check(background.StopAudioImmediately(scene_handle) == 1u &&
                  !background.IsAudioActive(scene_handle),
              "scene music handle stops independently");
        std::cout << "PASS: original P01-1.smp music stream\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
