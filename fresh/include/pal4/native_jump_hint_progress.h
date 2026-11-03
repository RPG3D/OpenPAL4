#pragma once

#include "pal4/dialog_layout_base.h"
#include <array>
#include <cstdint>
#include <windows.h>

namespace pal4 {

using NativeJumpHintProgressState = std::array<std::uint32_t, 14>;

// Widgets belong to the CEGUI session. The record retains the original baseline
// geometry once; updates must not recapture the shrinking mask's dimensions.
void BindNativeJumpHintProgress(HMODULE module, NativeJumpHintProgressState& state,
    std::uint32_t root, std::uint32_t image, IDialogLayoutParentRuntime& parents);
void SetNativeJumpHintProgressVisible(HMODULE module,
    NativeJumpHintProgressState& state, bool visible,
    IDialogLayoutParentRuntime& parents);
void AdvanceNativeJumpHintProgress(HMODULE module,
    NativeJumpHintProgressState& state, float delta,
    IDialogLayoutParentRuntime& parents);

}  // namespace pal4
