#pragma once
#include "pal4/native_common_dialog_layout.h"

namespace pal4 {
class NativeCeguiUiSession;
class NativeGameDatabase;

// Borrows the live save page, database and dialog. Result handlers are copied
// into the prompt and must own any state they need after Show returns.
class NativeSystemSettingSaveConfirmation final {
public:
    NativeSystemSettingSaveConfirmation(HMODULE module, NativeCeguiUiSession& ui,
        const NativeGameDatabase& database, NativeCommonDialogLayout& dialog,
        NativeCommonDialogLayout::SoundPlayer sound,
        NativeCommonDialogLayout::SelectionHandler save,
        NativeCommonDialogLayout::SelectionHandler load);
    [[nodiscard]] std::uint32_t Show();
private:
    class Runtime;
    HMODULE module_;
    NativeCeguiUiSession& ui_;
    const NativeGameDatabase& database_;
    NativeCommonDialogLayout& dialog_;
    NativeCommonDialogLayout::SoundPlayer sound_;
    NativeCommonDialogLayout::SelectionHandler save_, load_;
};
} // namespace pal4
