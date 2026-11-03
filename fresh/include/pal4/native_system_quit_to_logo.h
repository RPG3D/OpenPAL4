#pragma once

#include "pal4/native_system_setting_quit_confirmation.h"
#include <functional>

namespace pal4 {
class NativeArenaLoadRuntime;
class NativeSystemToolbarActions;

// Binds the original481630 release -> B4/Scene -> Logo sequence. Borrowed
// transition/toolbar owners and the release callback dependencies must outlive
// the returned services and their confirmation controller.
[[nodiscard]] NativeSystemSettingQuitServices BindNativeSystemQuitToLogo(
    NativeArenaLoadRuntime& transitions, NativeSystemToolbarActions& toolbar,
    std::function<void()> release_arena);
}  // namespace pal4
