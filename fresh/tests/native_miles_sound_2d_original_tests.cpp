#include "pal4/native_miles_digital_driver.h"
#include "pal4/native_miles_sound_2d_runtime.h"
#include "pal4/script_sound_2d.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Check(argc == 2, "provide original PAL4 installation path");
        pal4::NativeMilesDigitalDriver driver(argv[1]);
        bool closed_rejected=false;
        try {pal4::NativeMilesSound2dRuntime requires_open(argv[1],driver);}
        catch(const std::invalid_argument&){closed_rejected=true;}
        Check(closed_rejected,"ordinary sound owner still requires an opened driver");
        pal4::NativeMilesSound2dRuntime prepared(argv[1],driver,{},
            pal4::NativeMilesSound2dRuntime::DriverRequirement::Deferred);
        Check(!driver.Handle() && prepared.AudioManager()==0u &&
                  prepared.PlayNamedSound("WJ001.wav",1)==0u && !driver.Handle() &&
                  prepared.ActiveCount()==0u && !prepared.LastError().empty(),
              "prepared sound owner cannot start audio or allocate a sample before initialization");
        Check(driver.Open(), "open original Miles digital driver");
        driver.SetMasterVolume(0.0f);
        Check(prepared.PlayNamedSound("WJ001.wav",1)!=0u,
              "same prepared owner uses the explicitly opened shared driver");
        prepared.StopAll();
        std::string name = "WJ001";
        const auto source = reinterpret_cast<std::uint32_t>(&name);
        pal4::NativeMilesSound2dRuntime sound(argv[1], driver,
            [](std::uint32_t address) -> std::string_view {
                return *reinterpret_cast<const std::string*>(address);
            });
        Check(pal4::ScriptSound2d::Play(source, 1, sound) == -1 &&
                  sound.CurrentHandle() != 0u &&
                  sound.ActiveCount() == 1u &&
                  sound.CheckActive(sound.ScriptExecutor() + 0x20u) == 1u,
              "original WJ001.wav plays through Miles sample API");
        Check(pal4::ScriptSound2d::StopCurrent(sound) == 1u &&
                  sound.CurrentHandle() == 0u && sound.ActiveCount() == 0u,
              "current 2D sound releases its Miles sample");
        // Leave an unrelated current sample alive: failed detached playback must
        // not register or stop its handle, even after a previous successful play.
        Check(pal4::ScriptSound2d::Play(source, 0, sound) == -1,
              "current looping sound starts independently");
        const auto current = sound.CurrentHandle();
        name = "PAL4_MISSING_SOUND_RESET_TEST";
        Check(pal4::ScriptSound2d::Play(source, -1, sound) == -1 &&
                  sound.ScriptSoundCount() == 0u && sound.ActiveCount() == 1u &&
                  sound.CurrentHandle() == current && sound.IsPlaying(current),
              "failed playback does not register the previous sample");
        name = "WJ001";
        const auto id = pal4::ScriptSound2d::Play(source, -1, sound);
        Check(id == 0 && sound.ActiveCount() == 2u && sound.ScriptSoundCount() == 1u &&
                  pal4::ScriptSound2d::StopId(id, sound) == 1u &&
                  sound.ScriptSoundCount() == 0u && sound.ActiveCount() == 1u &&
                  sound.CurrentHandle() == current && sound.IsPlaying(current) &&
                  pal4::ScriptSound2d::StopId(id, sound) == 0u,
              "script ID zero stops only its detached sample, repeat removal is safe");
        Check(pal4::ScriptSound2d::Play(source, -1, sound) == 1 &&
                  pal4::ScriptSound2d::Play(source, -1, sound) == 2,
              "script sequence remains independent of released sample handles");
        const auto borrowed_ui = sound.PlayNamedSound("WA22.wav", 0);
        sound.ResetScriptData();
        Check(sound.ScriptSoundCount() == 0u && sound.ActiveCount() == 2u &&
                  sound.CurrentHandle() == current && sound.IsPlaying(current) &&
                  sound.IsPlaying(borrowed_ui),
              "scene reset stops detached sounds while preserving current and UI samples");
        sound.ResetScriptData();
        Check(pal4::ScriptSound2d::Play(source, -1, sound) == 0,
              "repeated scene reset restarts the original script sequence at zero");
        sound.StopAll();
        Check(sound.ActiveCount() == 0u && sound.ScriptSoundCount() == 0u &&
                  sound.CurrentHandle() == 0u,
              "final sound teardown drains both registries");
        pal4::NativeMilesSound2dRuntime ui_sound(argv[1], driver);
        const auto uppercase = ui_sound.PlayNamedSound("WA06.WAV", 1);
        Check(uppercase && ui_sound.StopSample(uppercase) && !ui_sound.ActiveCount(),
              "original Combat money uppercase WA06.WAV plays and releases through the same Miles owner");
        const auto first_ui = ui_sound.PlayNamedSound("WA22.wav", 1);
        Check(first_ui != 0u &&
                  ui_sound.ActiveCount() == 1u && ui_sound.CurrentHandle() == 0u,
              "native UI sound shares Miles sample path without script current handle");
        ui_sound.SetVolume(0.25f);
        Check(ui_sound.VolumeLevels(first_ui) == std::array<float, 2>{0.25f, 0.25f},
              "live Miles sample responds to SFX level with muted driver");
        const auto second_ui = ui_sound.PlayNamedSound("WA22.wav", 1);
        Check(second_ui && ui_sound.VolumeLevels(second_ui)[0] == 0.25f,
              "new Miles sample inherits SFX level");
        bool rejected = false;
        try { ui_sound.SetVolume(1.1f); } catch (const std::invalid_argument&) { rejected = true; }
        Check(rejected && ui_sound.VolumeLevels(first_ui)[0] == 0.25f,
              "invalid SFX level retains current Miles sample volume");
        Check(ui_sound.PlayNamedSound("../WA22.wav", 1) == 0u &&
                  ui_sound.PlayNamedSound("WA22", 1) == 0u &&
                  ui_sound.PlayNamedSound("WA22.wav", -1) == 0u &&
                  ui_sound.ActiveCount() == 2u,
              "invalid UI filenames/modes retain the active sample");
        ui_sound.StopAll();
        Check(ui_sound.ActiveCount() == 0u, "native UI samples release before driver shutdown");
        const auto effect=ui_sound.Play("gamedata\\PALSound\\WA22.wav",false,false,{},600,300);
        Check(effect && ui_sound.Exists(effect) && ui_sound.Playing(effect) &&
                  !ui_sound.SetPosition(effect,{1,2,3}) && ui_sound.Stop(effect) &&
                  !ui_sound.Exists(effect) && !ui_sound.Stop(effect),
              "effect audio loops through real Miles and stop removes its exact handle");
        bool missing=false, spatial=false;
        try {(void)ui_sound.Play("gamedata\\PALSound\\PAL4_MISSING_EFFECT.wav",false,true,{},600,300);}
        catch(const std::runtime_error&) {missing=true;}
        try {(void)ui_sound.Play("gamedata\\PALSound\\WA22.wav",true,true,{},600,300);}
        catch(const std::logic_error&) {spatial=true;}
        Check(missing && spatial && ui_sound.ActiveCount()==0,
              "missing effect files and unbound spatial provider fail without successful handles");
        for (unsigned cycle=0; cycle<128u; ++cycle) {
            const auto sample=ui_sound.PlayNamedSound("WA47.wav",0);
            if (!sample) throw std::runtime_error("Miles reuse cycle "+std::to_string(cycle)+": "+ui_sound.LastError());
            Check(ui_sound.IsPlaying(sample) && ui_sound.StopSample(sample) && !ui_sound.ActiveCount(),
                  "repeated reward loop releases and reuses the actual Miles sample slot");
        }
        std::cout << "PASS: muted original WJ001.wav and native WA22.wav Miles 2D sound\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
