#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ISystemSettingListControlsRuntime {
public:
    virtual ~ISystemSettingListControlsRuntime() = default;
    virtual void ResetList(std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t ColumnCount(std::uint32_t window) = 0;
    virtual void RemoveColumn(std::uint32_t window, std::uint32_t index) = 0;
    virtual void SetSortDirection(std::uint32_t window, std::uint32_t direction) = 0;
    virtual void SetUserColumnSizing(std::uint32_t window, bool enabled) = 0;
    virtual void SetUserColumnDragging(std::uint32_t window, bool enabled) = 0;
    virtual void SetUserSortControl(std::uint32_t window, bool enabled) = 0;
};

class ISystemSettingNavigationLayoutRuntime {
public:
    virtual ~ISystemSettingNavigationLayoutRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void AppendRecordPointer(std::uint32_t collection, std::uint32_t record) = 0;
};

class ISystemSettingInterfaceRuntime : public ISystemSettingListControlsRuntime,
                                      public ISystemSettingNavigationLayoutRuntime {
public:
    [[nodiscard]] virtual std::uint32_t BeginName(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t ResetName(
        std::uint32_t buffer, std::uint32_t frame,
        std::string_view suffix) = 0;
    [[nodiscard]] virtual std::uint32_t EndName(std::uint32_t buffer) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, std::uint32_t event,
        std::uint32_t callback) = 0;
    virtual void ReleaseConnection(std::uint32_t connection) = 0;
    [[nodiscard]] virtual std::uint32_t TriggerText(
        std::uint32_t trigger_id) = 0;
    [[nodiscard]] virtual std::string_view ToUtf8(std::uint32_t text) = 0;
    virtual void SetText(std::uint32_t window, std::string_view text) = 0;
    virtual void CopyText(std::uint32_t target, std::uint32_t source) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window, bool enabled) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    virtual void SetHorizontalFormatting(std::uint32_t window,
                                         std::uint32_t format) = 0;
    virtual void SetSliderMaximum(std::uint32_t window, float maximum) = 0;
    virtual void SetupMouseHandlers(std::uint32_t member,
                                    std::uint32_t window) = 0;
};

class SystemSettingInterface final {
public:
    static void ConfigureSaveList(std::uint32_t list,
                                  ISystemSettingListControlsRuntime& runtime);
    static void LinkNavigationRecords(std::uint32_t frame,
        ISystemSettingNavigationLayoutRuntime& runtime);
    static constexpr std::uint32_t kCallbackVtable = 0x843c00u;
    static constexpr std::uint32_t kClickedEvent = 0x8411b8u;
    static constexpr std::uint32_t kHoverEvent = 0x841184u;
    static constexpr std::uint32_t kMouseClickEvent = 0x841180u;
    static constexpr std::uint32_t kMouseButtonUpEvent = 0x841108u;
    static constexpr std::uint32_t kValueChangedEvent = 0x841024u;

    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t frame, ISystemSettingInterfaceRuntime& runtime);
};

}  // namespace pal4
