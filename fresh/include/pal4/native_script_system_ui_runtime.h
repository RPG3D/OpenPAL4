#pragma once

#include "pal4/script_system_ui.h"

#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace pal4 {

class NativePalGameIVTextureStorage;
class IUiFrameManagerPortraitRuntime;

// Application endpoints borrow the live state dispatcher and registered pages.
// The runtime validates original owner/payload identities before calling them.
struct NativeScriptSystemUiServices {
    std::function<void()> enter_system;
    std::function<void(std::uint32_t, std::uint32_t, std::uint32_t)> event;
};

class NativeScriptSystemUiRuntime final : public IScriptSystemUiRuntime {
public:
    NativeScriptSystemUiRuntime(NativePalGameIVTextureStorage& game,
                                std::uint32_t ui_manager,
                                IUiFrameManagerPortraitRuntime* portrait = nullptr,
                                NativeScriptSystemUiServices services = {});

    std::uint32_t Game() override;
    void SetGameDataEntry(std::uint32_t game, std::uint32_t entry,
                          std::uint8_t value) override;
    void InitializeStateAndUi(std::uint32_t game, std::uint32_t state,
                              std::uint32_t flags) override;
    std::uint32_t UiFrameManager() override;
    void ProcessEvent(std::uint32_t event_tree,
                      std::uint32_t event_descriptor,
                      std::uint32_t menu_index,
                      std::uint32_t reserved) override;
    void SetFrameFlag(std::uint32_t frames,
                      std::uint8_t enabled) override;
    [[nodiscard]] const std::vector<std::pair<std::uint32_t, std::uint8_t>>&
    Writes() const noexcept { return writes_; }

private:
    NativePalGameIVTextureStorage& game_;
    std::uint32_t ui_manager_;
    IUiFrameManagerPortraitRuntime* portrait_;
    NativeScriptSystemUiServices services_;
    std::vector<std::pair<std::uint32_t, std::uint8_t>> writes_;
};

}  // namespace pal4
