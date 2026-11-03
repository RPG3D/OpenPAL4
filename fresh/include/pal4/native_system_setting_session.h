#pragma once
#include "pal4/native_system_setting_save_result.h"
#include "pal4/native_system_setting_quit_confirmation.h"
#include <vector>
namespace pal4 {
class NativeDialogSettingsSnapshot;
class NativeBackgroundMusicRuntime;
class NativeMilesSound2dRuntime;
class NativeUiSoundPlayback;
class NativeEventHandlerArray;
class IGlobalDataScaleRuntime;
class ISystemSettingStatsDataRuntime;
class NativeSystemSettingFrame;
// The domain endpoints retain the original save/load/quit algorithms. Missing
// game save/load backends must fail explicitly when confirmed, never report success.
struct NativeSystemSettingSessionServices {
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> event;
    std::function<void(std::uint32_t)> tutorial;
    NativeSystemSettingQuitServices quit;
    NativeSystemSettingSaveResult::SaveSlot save_slot;
    NativeSystemSettingSaveResult::EnterLoading enter_loading;
    std::function<std::vector<NativeMilesSound2dRuntime*>()> samples;
    // Empty preserves the asset root. All summary/list controllers use this
    // archive location; the host's save/load callbacks must use the same root.
    std::filesystem::path archive_root;
};
// Owns all settings page controllers and borrows the shared confirmation dialog. UI,
// live data/audio/rendering/activity and callbacks outlive it. Close before
// retiring these owners; activity dispatch must route this session while live.
class NativeSystemSettingSession final {
public:
    NativeSystemSettingSession(NativeCeguiUiSession&,const std::filesystem::path&,
        const NativeGameDatabase&,NativeDialogSettingsSnapshot&,IDialogManagerSettingsWriteRuntime&,
        ISystemSettingStatsDataRuntime&,IGlobalDataScaleRuntime&,NativeEventHandlerArray&,
        std::uint32_t manager,std::uint32_t arena,NativeBackgroundMusicRuntime&,
        NativeUiSoundPlayback&,NativeCommonDialogLayout&,NativeSystemSettingSessionServices);
    ~NativeSystemSettingSession();
    NativeSystemSettingSession(const NativeSystemSettingSession&)=delete;
    NativeSystemSettingSession& operator=(const NativeSystemSettingSession&)=delete;
    void SetVisible(bool,std::uint32_t parent=0u);
    bool OwnsActivity(std::uint32_t) const noexcept;
    std::uint32_t Callback(std::uint32_t,std::uint32_t);
    std::uint8_t Predicate(std::uint32_t,std::uint32_t,std::uint32_t);
    NativeSystemSettingFrame& Frame();
    NativeCommonDialogLayout& Dialog();
    std::uint32_t MainInput() const;
    std::uint32_t OptionsInput() const;
    std::uint32_t DataInput() const;
    void RethrowError();
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
