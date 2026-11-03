#pragma once

#include "pal4/main_window_commands.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/ol_info_commands.h"
#include "pal4/native_load_window.h"

#include <array>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace pal4 {

enum class NativeMenuActionKind { start_new_game, exit_game, load_slot };

struct NativeMenuAction {
    NativeMenuActionKind kind;
    std::int32_t slot = -1;
};

// Connects original CEGUI menu events to the reviewed game command dispatchers.
class NativeMainMenuController final : private MainWindowCommandRuntimeStub,
                                       private OlInfoCommandRuntimeStub {
public:
    NativeMainMenuController(NativeCeguiUiSession& ui,
                             std::filesystem::path game_root,
                             std::uint32_t frame_manager = 0u);
    void BindLoadWindow(const NativeGameDatabase&, DialogManagerState&,
        IDialogManagerSettingsWriteRuntime&, NativeEventHandlerArray&, NativeLoadWindow::DialogFactory,
        NativeCommonDialogLayout::SoundPlayer);

    [[nodiscard]] bool ProcessMouseMessage(HWND window, std::uint32_t message,
                                           std::uint32_t wparam,
                                           std::uint32_t lparam);
    [[nodiscard]] std::optional<NativeMenuAction> TakeAction();
    [[nodiscard]] bool LoadConfirmationVisible() const { return load_window_ && load_window_->ConfirmationVisible(); }
    void CancelLoadConfirmation() { if(load_window_)load_window_->Cancel(); }
    [[nodiscard]] std::int32_t HoveredLoadSlot() const { return load_window_?load_window_->HoveredSlot():-1; }
    [[nodiscard]] const std::array<std::optional<NativeGameSaveSummary>, 10>&
    PageSummaries() const;
    [[nodiscard]] bool OwnsActivity(std::uint32_t p) const { return load_window_ && load_window_->OwnsActivity(p); }
    [[nodiscard]] std::uint32_t Callback(std::uint32_t slot) { return load_window_->Callback(slot); }
    [[nodiscard]] std::uint8_t Predicate(std::uint32_t slot,std::uint32_t event) { return load_window_->Predicate(slot,event); }
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }

private:
    [[nodiscard]] bool PopulatePage(std::int32_t page);
    std::uint32_t FrameManager() override { return frame_manager_; }
    void ProcessEvent(std::uint32_t tree, std::string_view command,
                      MainWindowEventWords& words) override;
    void InitializeGame() override;
    void HideCurrentWindow(std::uint32_t showing,
                           std::uint32_t parent) override;
    void HideWindow(std::uint32_t object, std::uint32_t showing,
                    std::uint32_t parent) override;
    void ProcessCommand(std::uint32_t tree, std::string_view command,
                        OlInfoEventWords& words) override;

    NativeCeguiUiSession& ui_;
    std::filesystem::path game_root_;
    std::uint32_t frame_manager_;
    mutable std::array<std::optional<NativeGameSaveSummary>, 10> summaries_{};
    mutable std::int32_t summary_page_ = -1;
    std::deque<NativeMenuAction> actions_;
    std::string last_error_;
    std::unique_ptr<NativeLoadWindow> load_window_;
};

}  // namespace pal4
