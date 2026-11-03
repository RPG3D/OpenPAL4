#pragma once

#include "pal4/dialog_manager_settings.h"
#include "pal4/script_global_callbacks.h"

namespace pal4 {

// Binds the script's movie flag to the live dialog/global-data object.
class NativeMovieFlagRuntime final : public IScriptMovieFlagRuntime {
public:
    NativeMovieFlagRuntime(DialogManagerState& state,
                           IDialogManagerSettingsWriteRuntime& settings)
        : state_(state), settings_(settings) {}

    [[nodiscard]] std::uint32_t GlobalData() override;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    void SaveDialogSettings(std::uint32_t global_data) override;

private:
    void CheckMovieFlagAddress(std::uint32_t address) const;

    DialogManagerState& state_;
    IDialogManagerSettingsWriteRuntime& settings_;
};

}  // namespace pal4
