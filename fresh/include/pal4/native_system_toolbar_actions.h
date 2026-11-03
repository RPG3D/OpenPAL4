#pragma once

#include "pal4/system_toolbar_operations.h"
#include "pal4/ui_navigation_list.h"
#include <array>
#include <windows.h>

namespace pal4 {
class NativeCeguiUiSession;
struct NativeSystemToolbarVisibilityServices;
class NativeSystemToolbarVisibilityRuntime;
class NativeSystemToolbarCommandRuntime;
class NativeSystemToolbarFrameEventRuntime;

// Borrows the session's windows and game services. Reuses the shared toolbar
// algorithms and the shared navigation selection over real DLL windows.
// The game delegate owns page-event dispatch, smith and state services.
// Destroy or clear the session's click handler before destroying this adapter.
class NativeSystemToolbarActions final : private ISystemToolbarOperationsRuntime,
                                        private IUiNavigationInputRuntime {
public:
    NativeSystemToolbarActions(HMODULE module, NativeCeguiUiSession& ui,
                               ISystemToolbarGameRuntime& game);
    NativeSystemToolbarActions(const NativeSystemToolbarActions&) = delete;
    NativeSystemToolbarActions& operator=(const NativeSystemToolbarActions&) = delete;
    [[nodiscard]] bool OnButtonClick(std::uint32_t event);
    void ClearAllSelection();
    [[nodiscard]] bool HandleKeyInput(std::uint32_t event);
    [[nodiscard]] std::uint8_t Close();
    // Reviewed callback descriptors B0(reset), B4(close), B8(select page).
    // Page/state services still belong to the required game delegate.
    [[nodiscard]] std::uint8_t HandleSystemEvent(std::uint32_t descriptor,
        const std::array<std::uint32_t, 2>& payload);
    [[nodiscard]] std::uint32_t UpdateVisibility(std::uint8_t visible,
        std::uint32_t parent, NativeSystemToolbarVisibilityServices& services);
    // Generic frame-event normalization (4AA000) followed by original toolbar
    // visibility. Requires the session's preloaded toolbar frame.
    [[nodiscard]] bool HandleVisibilityEvent(std::uint32_t enabled,
        std::uint32_t parent, NativeSystemToolbarVisibilityServices& services);
    [[nodiscard]] std::uint32_t ActiveComponentAddress() const noexcept { return FrameAddress() + 40u; }
    [[nodiscard]] std::uint32_t DispatchActiveCallback(std::uint32_t component, std::uint32_t slot);
    [[nodiscard]] std::uint8_t DispatchActivePredicate(std::uint32_t component,
        std::uint32_t slot, std::uint32_t event);
    [[nodiscard]] std::uint32_t FrameAddress() const noexcept;
private:
    friend class NativeSystemToolbarVisibilityRuntime;
    friend class NativeSystemToolbarCommandRuntime;
    friend class NativeSystemToolbarFrameEventRuntime;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    bool CompareWindowName(std::uint32_t name, std::string_view text) override;
    std::uint32_t FrameManager() override;
    void DispatchEvent(std::uint32_t tree, std::uint32_t key,
                       std::uint32_t first, std::uint32_t second) override;
    void SetSelected(std::uint32_t button, bool selected) override;
    void SetCurrentSelected(std::uint32_t helper, std::uint32_t button) override;
    void RequestRedraw(std::uint32_t window) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void SetButtonSelected(std::uint32_t button, bool selected) override;
    bool IsDisabled(std::uint32_t window) override;
    bool IsVisible(std::uint32_t window) override;
    std::uint32_t InvokeCycle(std::uint32_t list, std::uint32_t slot, std::uint32_t previous) override;
    void MouseAtCenter(std::uint32_t window) override;
    std::uint32_t SmithSystem() override;
    void SetVisible(std::uint32_t frame, std::uint32_t visible, std::uint32_t parent) override;
    std::uint32_t Game() override;
    std::int32_t SceneMode(std::uint32_t scene) override;
    std::uint32_t TradeSystem() override;
    std::uint8_t ClearTradeItemInfo(std::uint32_t trade) override;
    std::uint8_t InitializeGameState(std::uint32_t game, std::uint32_t mode,
                                     std::uint32_t argument) override;
    void HandleMouseEvent(std::uint32_t window) override;
    HMODULE module_;
    NativeCeguiUiSession& ui_;
    ISystemToolbarGameRuntime& game_;
    // Reviewed frame fields used by SystemToolbarOperations. No EXE vtable is
    // installed or called. Navigation borrows the fixed owned window array.
    std::array<std::uint32_t, 23> frame_{};
    std::array<std::uint32_t, 7> navigation_windows_{};
};
}  // namespace pal4
