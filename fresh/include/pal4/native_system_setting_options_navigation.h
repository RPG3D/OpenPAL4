#pragma once

#include "pal4/system_setting_navigation.h"
#include "pal4/system_setting_interface.h"
#include <array>
#include <functional>
#include <windows.h>

namespace pal4 {
class NativeCeguiUiSession;
class NativeEventHandlerArray;

// Owns stable navigation record pointers; borrows actual session records/windows
// and activity. Value changes/button activation require real host endpoints.
class NativeSystemSettingOptionsNavigation final : private ISystemSettingNavigationRuntime,
    private ISystemSettingNavigationLayoutRuntime {
public:
    NativeSystemSettingOptionsNavigation(HMODULE module, NativeCeguiUiSession& ui,
        NativeEventHandlerArray& active, std::uint32_t manager,
        std::function<void(std::uint32_t, float)> slider = {},
        std::function<void(std::uint32_t)> click = {});
    ~NativeSystemSettingOptionsNavigation();
    NativeSystemSettingOptionsNavigation(const NativeSystemSettingOptionsNavigation&) = delete;
    NativeSystemSettingOptionsNavigation& operator=(const NativeSystemSettingOptionsNavigation&) = delete;
    void Show();
    std::uint32_t InputAddress() const;
    std::uint32_t InvokeVirtual(std::uint32_t member, std::uint32_t slot);
    std::uint8_t InvokePredicate(std::uint32_t member, std::uint32_t slot, std::uint32_t event);
private:
    std::uint32_t Frame() const;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    float ReadFloat(std::uint32_t address) override;
    void RequestRedraw(std::uint32_t window) override;
    void SetSliderValue(std::uint32_t slider, float value) override;
    std::uint32_t FrameManager() override;
    void RemoveActive(std::uint32_t collection, std::uint32_t member) override;
    void ShowSystemFrame(std::uint32_t frame) override;
    void HandleMouseEvent(std::uint32_t window) override;
    void AppendRecordPointer(std::uint32_t collection, std::uint32_t record) override;
    HMODULE module_;
    NativeCeguiUiSession& ui_;
    NativeEventHandlerArray& active_;
    std::uint32_t manager_;
    std::function<void(std::uint32_t, float)> slider_;
    std::function<void(std::uint32_t)> click_;
    std::array<std::uint32_t, 10> records_{};
    std::array<std::uint32_t, 8> saved_input_{};
    std::array<std::uint32_t, 50> saved_records_{};
    std::size_t count_ = 0u;
};
}  // namespace pal4
