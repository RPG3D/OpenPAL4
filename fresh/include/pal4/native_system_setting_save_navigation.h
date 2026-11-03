#pragma once

#include "pal4/system_setting_save_actions.h"
#include <windows.h>
#include <filesystem>
#include <functional>

namespace pal4 {
class NativeCeguiUiSession;
class NativeGameDatabase;
class DialogManagerState;
class NativeEventHandlerArray;

// Exclusive page callback owner. UI, database, settings and activity must outlive
// this binding. The frame controller supplies its real confirmation-dialog action.
class NativeSystemSettingSaveNavigation final : private ISystemSettingSaveNavigationRuntime {
public:
    NativeSystemSettingSaveNavigation(HMODULE module, NativeCeguiUiSession& ui,
        const std::filesystem::path& game_root, const NativeGameDatabase& database,
        const DialogManagerState& globals, NativeEventHandlerArray& activity,
        std::function<std::uint32_t()> context_menu);
    ~NativeSystemSettingSaveNavigation();
    NativeSystemSettingSaveNavigation(const NativeSystemSettingSaveNavigation&) = delete;
    NativeSystemSettingSaveNavigation& operator=(const NativeSystemSettingSaveNavigation&) = delete;
    [[nodiscard]] std::uint32_t InputAddress() const;
    void Activate();
    [[nodiscard]] bool OnPageClick(std::uint32_t event);
    [[nodiscard]] bool OnListClick(std::uint32_t event);
    [[nodiscard]] bool HandleKey(std::uint32_t event);
    [[nodiscard]] std::uint32_t InvokeVirtual(std::uint32_t member, std::uint32_t slot);
    [[nodiscard]] std::uint8_t InvokePredicate(std::uint32_t member, std::uint32_t slot, std::uint32_t event);
private:
    std::uint32_t Frame() const;
    void CheckList(std::uint32_t list) const;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void PopulateSaveList(std::uint32_t frame, std::int32_t page, std::uint32_t mode) override;
    std::uint32_t HoverItem(std::uint32_t list) override;
    std::uint32_t RowCount(std::uint32_t list) override;
    void UpHoverItem(std::uint32_t list) override;
    void DownHoverItem(std::uint32_t list) override;
    std::uint32_t FrameManager() override;
    void RemoveActive(std::uint32_t collection, std::uint32_t member) override;
    void ShowSystemFrame(std::uint32_t frame) override;
    std::uint32_t ShowSaveContextMenu(std::uint32_t frame) override;
    HMODULE module_;
    NativeCeguiUiSession& ui_;
    std::filesystem::path game_root_;
    const NativeGameDatabase& database_;
    const DialogManagerState& globals_;
    NativeEventHandlerArray& activity_;
    std::function<std::uint32_t()> context_menu_;
    std::uint32_t current_event_ = 0u;
};
}  // namespace pal4
