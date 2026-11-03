#pragma once
#include "pal4/native_common_dialog_layout.h"
#include <functional>
#include <memory>

namespace pal4 {
class NativeSystemSettingFrame;
class NativeGameDatabase;

// Domain actions must be supplied by the application. Confirm means release
// arena, close UI, reinitialize game/UI; it does not terminate the process.
struct NativeSystemSettingQuitServices {
    std::function<void()> release_arena;
    std::function<void()> close_ui;
    std::function<void()> initialize_game;
};

class NativeSystemSettingQuitConfirmation final {
public:
    NativeSystemSettingQuitConfirmation(NativeSystemSettingFrame&, const NativeGameDatabase&,
        NativeCommonDialogLayout&, NativeCommonDialogLayout::SoundPlayer,
        NativeSystemSettingQuitServices);
    ~NativeSystemSettingQuitConfirmation();
    NativeSystemSettingQuitConfirmation(const NativeSystemSettingQuitConfirmation&) = delete;
    NativeSystemSettingQuitConfirmation& operator=(const NativeSystemSettingQuitConfirmation&) = delete;
    std::uint32_t Show();
private:
    class Runtime;
    std::shared_ptr<Runtime> runtime_;
};
}
