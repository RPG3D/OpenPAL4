#pragma once
#include "pal4/system_setting_audio.h"
#include <windows.h>
#include <span>
#include <vector>
#include <functional>

namespace pal4 {
class NativeCeguiUiSession;
class NativeDialogSettingsSnapshot;
class NativeBackgroundMusicRuntime;
class NativeMilesSound2dRuntime;
class NativeUiSoundPlayback;

// Borrows the live settings and current music/2D sound owners. Collections hold
// native pipeline identities, never original executable vtables or Miles handles.
// This adapter currently registers music and kind1 sample owners; kind2 spatial
// sound pipelines still need their own volume endpoint.
class NativeSystemSettingAudio final : private ISystemSettingAudioRuntime {
public:
    using SamplesProvider=std::function<std::vector<NativeMilesSound2dRuntime*>()>;
    NativeSystemSettingAudio(HMODULE, NativeCeguiUiSession&, NativeDialogSettingsSnapshot&,
        NativeBackgroundMusicRuntime&, std::span<NativeMilesSound2dRuntime* const>, NativeUiSoundPlayback&);
    // Persistent pages resolve scene sound owners per event; a scene can retire
    // its script sample owner while the UI and HUD sound owner remain alive.
    NativeSystemSettingAudio(HMODULE, NativeCeguiUiSession&, NativeDialogSettingsSnapshot&,
        NativeBackgroundMusicRuntime&, SamplesProvider, NativeUiSoundPlayback&);
    ~NativeSystemSettingAudio();
    NativeSystemSettingAudio(const NativeSystemSettingAudio&) = delete;
    NativeSystemSettingAudio& operator=(const NativeSystemSettingAudio&) = delete;
    void SetSliderValue(std::uint32_t slider, float value);
    bool OnEvent(std::uint32_t event, bool preview);
private:
    std::uint32_t Frame() const;
    std::uint32_t ReadWord(std::uint32_t) override;
    float ReadFloat(std::uint32_t) override;
    void WriteFloat(std::uint32_t, float) override;
    std::uint32_t AudioManager() override;
    void SetStreamVolume(std::uint32_t, float) override;
    std::uint32_t StreamType(std::uint32_t) override;
    void RefreshStream(std::uint32_t) override;
    void PlayMusic(std::string_view, std::uint32_t) override;
    HMODULE module_;
    NativeCeguiUiSession& ui_;
    NativeDialogSettingsSnapshot& settings_;
    NativeBackgroundMusicRuntime& music_;
    NativeUiSoundPlayback& preview_;
    std::vector<NativeMilesSound2dRuntime*> samples_;
    SamplesProvider samples_provider_;
    void RefreshSamples();
    std::uint32_t music_identity_;
};
}
