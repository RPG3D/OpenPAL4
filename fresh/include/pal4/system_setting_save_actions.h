#pragma once

#include <cstdint>
#include <string_view>
#include "pal4/system_setting_dialog_callback.h"

namespace pal4 {

class ISystemSettingSavePageRuntime {
public:
    virtual ~ISystemSettingSavePageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void PopulateSaveList(std::uint32_t frame, std::int32_t page, std::uint32_t mode) = 0;
};

class ISystemSettingSaveContextEventRuntime {
public:
    virtual ~ISystemSettingSaveContextEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ShowSaveContextMenu(std::uint32_t frame) = 0;
};

class ISystemSettingSaveNavigationRuntime : public ISystemSettingSavePageRuntime,
                                           public ISystemSettingSaveContextEventRuntime {
public:
    [[nodiscard]] virtual std::uint32_t HoverItem(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(std::uint32_t list) = 0;
    virtual void UpHoverItem(std::uint32_t list) = 0;
    virtual void DownHoverItem(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void RemoveActive(std::uint32_t collection, std::uint32_t member) = 0;
    virtual void ShowSystemFrame(std::uint32_t frame) = 0;
};

class ISystemSettingSaveContextRuntime : public ISystemSettingDialogCallbackRuntime {
public:
    [[nodiscard]] virtual std::uint32_t HoverItem(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t ConstructCallback(std::uint32_t allocation,
        std::uint32_t function, std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t TriggerText(std::uint32_t trigger_id) = 0;
    virtual void CreateCommonDialog(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t ShowDialog(std::uint32_t text,
        std::uint32_t x_bits, std::uint32_t y_bits, std::uint32_t mode, std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseCallback(std::uint32_t callback) = 0;
};

// Only the boundaries used after a dialog selection. This keeps native result
// handling independent of page navigation and prompt allocation.
class ISystemSettingSaveResultRuntime {
public:
    virtual ~ISystemSettingSaveResultRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void PopulateSaveList(std::uint32_t frame, std::int32_t page, std::uint32_t mode) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t HoverItem(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t GlobalData() = 0;
    virtual void SaveSettings(std::uint32_t data) = 0;
    virtual void InvokeSlotHelper(std::int32_t slot) = 0;
    [[nodiscard]] virtual std::uint32_t CreateXmlNode() = 0;
    virtual void SetIntAttribute(std::uint32_t node,
                                 std::string_view name,
                                 std::int32_t value) = 0;
    virtual void DestroyXmlNode(std::uint32_t node) = 0;
    virtual void ProcessUiEvent(std::uint32_t dispatcher,
                                std::uint32_t event,
                                std::uint32_t first,
                                std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t Game() = 0;
    virtual void InitializeGameState(std::uint32_t game,
                                     std::uint32_t state,
                                     std::uint32_t argument) = 0;
};

class ISystemSettingSaveActionsRuntime : public ISystemSettingSaveNavigationRuntime,
                                        public ISystemSettingSaveContextRuntime,
                                        public ISystemSettingSaveResultRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) override = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) override = 0;
    [[nodiscard]] virtual std::uint32_t HoverItem(std::uint32_t list) override = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() override = 0;
    virtual void PopulateSaveList(std::uint32_t frame, std::int32_t page, std::uint32_t mode) override = 0;
    [[nodiscard]] std::uint32_t ShowSaveContextMenu(std::uint32_t frame) override;
};

class SystemSettingSaveActions final {
public:
    [[nodiscard]] static std::uint8_t OnClick(
        std::uint32_t frame, std::uint32_t event,
        ISystemSettingSaveResultRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnDoubleClick(
        std::uint32_t frame, std::uint32_t event,
        ISystemSettingSaveResultRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnChangePage(
        std::uint32_t frame, std::uint32_t event,
        ISystemSettingSavePageRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnContextMenuEvent(
        std::uint32_t frame, std::uint32_t event,
        ISystemSettingSaveContextEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t ShowContextMenu(
        std::uint32_t frame, ISystemSettingSaveContextRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnHover(
        std::uint32_t frame, std::uint32_t event,
        ISystemSettingSaveActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnKey(
        std::uint32_t member, std::uint32_t event,
        ISystemSettingSaveNavigationRuntime& runtime);
};

}  // namespace pal4
