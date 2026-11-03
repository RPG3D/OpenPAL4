#pragma once

#include "pal4/system_setting_visibility.h"
#include "pal4/system_setting_actions.h"
#include "pal4/system_setting_key_handler.h"
#include "pal4/ui_navigation_list.h"
#include <array>
#include <filesystem>
#include <functional>
#include <windows.h>

namespace pal4 {
class NativeCeguiUiSession;
class NativeGameDatabase;
class DialogManagerState;
class NativeEventHandlerArray;
class ISystemSettingStatsDataRuntime;

struct NativeSystemSettingFrameServices {
    std::function<void(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t)> event;
    std::function<void()> save_settings;
    std::function<void(std::uint32_t)> tutorial;
    std::function<void()> show_options;
    std::function<void()> confirm_quit;
};

// Borrows DLL windows/data and activity. Hide and remove subordinate input
// owners before destruction. Save/options input and quit confirmation belong
// to separate controllers; unsupported requests remain explicit failures.
class NativeSystemSettingFrame final : private ISystemSettingVisibilityRuntime,
    private ISystemSettingSelectionRuntime, private IUiNavigationInputRuntime,
    private ISystemSettingKeyRuntime {
public:
    NativeSystemSettingFrame(HMODULE module, NativeCeguiUiSession& ui,
        const std::filesystem::path& root, const NativeGameDatabase& database,
        const DialogManagerState& globals, ISystemSettingStatsDataRuntime& stats,
        NativeEventHandlerArray& active, std::uint32_t manager, std::uint32_t arena,
        NativeSystemSettingFrameServices services);
    ~NativeSystemSettingFrame();
    NativeSystemSettingFrame(const NativeSystemSettingFrame&) = delete;
    NativeSystemSettingFrame& operator=(const NativeSystemSettingFrame&) = delete;
    void SetVisible(bool visible, std::uint32_t parent = 0u);
    std::uint32_t FrameAddress() const;
    std::uint32_t InputAddress() const { return FrameAddress() + 40u; }
    std::uint32_t InvokeVirtual(std::uint32_t member, std::uint32_t slot);
    std::uint8_t InvokePredicate(std::uint32_t member, std::uint32_t slot, std::uint32_t event);
    bool OnMainClick(std::uint32_t event);
    // Borrowed by subordinate action controllers; valid only for this lifetime.
    ISystemSettingSelectionRuntime& SelectionRuntime() { return *this; }
private:
    void CheckFrame(std::uint32_t frame) const;
    void CheckMember(std::uint32_t collection, std::uint32_t member) const;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void SetButtonSelected(std::uint32_t window, bool selected) override;
    bool IsDisabled(std::uint32_t window) override;
    bool IsVisible(std::uint32_t window) override;
    std::uint32_t InvokeCycle(std::uint32_t list, std::uint32_t slot, std::uint32_t previous) override;
    void MouseAtCenter(std::uint32_t window) override;
    void SetParent(std::uint32_t frame, std::uint8_t visible, std::uint32_t parent) override;
    std::uint32_t FrameManager() override;
    void ProcessUiEvent(std::uint32_t tree, std::uint32_t key, std::uint32_t first, std::uint32_t second) override;
    void ShowSystemFrame(std::uint32_t frame) override;
    void UpdateStats(std::uint32_t frame) override;
    void InsertActive(std::uint32_t collection, std::uint32_t member) override;
    void RemoveActive(std::uint32_t collection, std::uint32_t member) override;
    void SetEnabled(std::uint32_t window, bool enabled) override;
    std::uint32_t ArenaManager() override;
    std::uint32_t CreateEventArgs() override;
    void DestroyEventArgs(std::uint32_t event) override;
    void InvokeLoadData(std::uint32_t frame, std::uint32_t event) override;
    void InvokeStartNewGame(std::uint32_t frame, std::uint32_t event) override;
    void UpdateSystemDialog(std::uint32_t manager, std::uint32_t key) override;
    std::uint32_t InitializeGlobalData() override;
    void SaveSettings(std::uint32_t data) override;
    void ShowDataMode(std::uint32_t frame) override;
    void ShowSave(std::uint32_t frame) override;
    void SetFocusedButton(std::uint32_t member, std::uint32_t button) override;
    void SetSelected(std::uint32_t button, bool selected) override;
    void RequestRedraw(std::uint32_t window) override;
    void SetVisible(std::uint32_t window, bool visible) override;
    void SetImage(std::uint32_t window, std::string_view imageset, std::string_view image) override;
    void HideFrame(std::uint32_t frame, std::uint32_t first, std::uint32_t second) override;
    std::uint8_t ForwardKey(std::uint32_t member, std::uint32_t event) override;
    void HandleMouseEvent(std::uint32_t window) override;
    void SelectLoadData(std::uint32_t frame) override;
    void ShowSystemSettings(std::uint32_t frame) override;
    void ConfirmQuit(std::uint32_t frame) override;
    HMODULE module_;
    NativeCeguiUiSession& ui_;
    std::filesystem::path root_;
    const NativeGameDatabase& database_;
    const DialogManagerState& globals_;
    ISystemSettingStatsDataRuntime& stats_;
    NativeEventHandlerArray& active_;
    std::uint32_t manager_, arena_;
    NativeSystemSettingFrameServices services_;
    std::array<std::uint32_t, 4> navigation_{};
    std::array<std::uint32_t, 3> saved_navigation_{};
    alignas(4) std::array<std::byte, 8> event_{};
};
}  // namespace pal4
