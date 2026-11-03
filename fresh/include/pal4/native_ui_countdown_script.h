#pragma once

#include "pal4/minimap_scene_ui.h"

#include <cstdint>
#include <functional>
#include <string_view>

namespace pal4 {
class NativeGameTimerQueue;
class NativeTimedScriptStack;

struct NativeUiCountdownScriptServices {
    std::uint32_t ui_manager;
    std::uint32_t arena;
    NativeGameTimerQueue& timers;
    NativeTimedScriptStack& scripts;
    // Borrow the actual arena+ D4 script source and the UI text endpoint.
    std::string_view arena_script;
    std::function<void(std::uint32_t window, std::string_view text)> set_text;
};

class NativeUiCountdownScript final {
public:
    // IDA47CAA0: display/decrement the live portrait state; on expiry register
    // and queue PFNTimerScript[TimerScript] using the existing SDK script stack.
    // This prepares a context; it does not execute that context synchronously.
    [[nodiscard]] static std::uint32_t Execute(MinimapSceneUiState& state,
        std::uint32_t owner, NativeUiCountdownScriptServices services);
};
}  // namespace pal4
