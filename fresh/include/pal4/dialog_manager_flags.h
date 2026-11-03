#pragma once

#include "pal4/dialog_manager_settings.h"

#include <cstdint>

namespace pal4 {

class IDialogManagerFlagRuntime {
public:
    virtual ~IDialogManagerFlagRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual std::uint32_t GameWord(
        std::uint32_t address) = 0;
    virtual void SetGameWord(std::uint32_t address,
                             std::uint32_t value) = 0;
};

class DialogManagerFlags final {
public:
    // IDA 0x5ce1e0 / 0x5ce210 / 0x5ce270.
    static void ClearAndSave(DialogManagerState& state,
                             IDialogManagerFlagRuntime& game,
                             IDialogManagerSettingsWriteRuntime& writer);
    static void SetAndSave(DialogManagerState& state, std::uint32_t position,
                           IDialogManagerFlagRuntime& game,
                           IDialogManagerSettingsWriteRuntime& writer);
    [[nodiscard]] static bool IsSet(std::uint32_t position,
                                    IDialogManagerFlagRuntime& game);
};

}  // namespace pal4
