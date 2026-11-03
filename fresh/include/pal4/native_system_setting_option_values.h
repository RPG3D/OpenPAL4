#pragma once
#include "pal4/system_setting_options.h"
#include <windows.h>

namespace pal4 {
class NativeCeguiUiSession;
class DialogManagerState;
class IGlobalDataScaleRuntime;

// Borrows live DLL windows, global settings and the real rendering services.
// Must be destroyed before the UI session and these services.
class NativeSystemSettingOptionValues final : private ISystemSettingOptionActionsRuntime {
public:
    NativeSystemSettingOptionValues(HMODULE, NativeCeguiUiSession&,
        DialogManagerState&, IGlobalDataScaleRuntime&);
    ~NativeSystemSettingOptionValues();
    NativeSystemSettingOptionValues(const NativeSystemSettingOptionValues&) = delete;
    NativeSystemSettingOptionValues& operator=(const NativeSystemSettingOptionValues&) = delete;
    bool OnClick(std::uint32_t event);
    void Click(std::uint32_t window);
private:
    std::uint32_t Frame() const;
    std::uint32_t ReadWord(std::uint32_t) override;
    void WriteWord(std::uint32_t, std::uint32_t) override;
    void SetSelected(std::uint32_t, bool) override;
    std::uint32_t GlobalData() override;
    void ApplyClipDistance(std::uint32_t, std::uint32_t) override;
    HMODULE module_;
    NativeCeguiUiSession& ui_;
    DialogManagerState& globals_;
    IGlobalDataScaleRuntime& rendering_;
};
}
