#pragma once
#ifdef FormatMessage
#undef FormatMessage
#endif

#include "pal4/smith_window_item_quantity.h"
#include "pal4/smith_window_word_array.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pal4 {

enum class SmithWindowEvent {
    Clicked,
    HoverRowChanged,
    MouseClick,
};

class ISmithWindowRuntime {
public:
    virtual ~ISmithWindowRuntime() = default;
    virtual void LoadLayout(std::uint32_t object,
                            std::uint32_t layout_name) = 0;
    virtual void InitializeListAdapter(std::uint32_t adapter) = 0;
    virtual void InitializeNameNode(std::uint32_t node) = 0;
    virtual void ResetTextFlag(std::uint32_t state,
                               std::uint8_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t GetWindowByName(
        std::uint32_t object, std::string_view name) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window, bool enabled) = 0;
    virtual void SetReadOnly(std::uint32_t window, bool enabled) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, SmithWindowEvent event,
        std::uint32_t callback) = 0;
    virtual void ReleaseSubscription(std::uint32_t subscription) = 0;
    virtual void SetShowVertScrollbar(std::uint32_t list,
                                      bool visible) = 0;
    virtual void ResetState(std::uint32_t object) = 0;
    virtual void SetupMouseEventHandlers(std::uint32_t adapter,
                                         std::uint32_t window) = 0;
};

class ISmithWindowDestructionRuntime {
public:
    virtual ~ISmithWindowDestructionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void ResetEffectHandle(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestructBase(
        std::uint32_t object) = 0;
};

class ISmithWindowVisibilityRuntime {
public:
    virtual ~ISmithWindowVisibilityRuntime() = default;
    virtual void SetParent(std::uint32_t object, std::uint8_t visible,
                           std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void ProcessUIEvent(std::uint32_t tree,
                                std::uint32_t key,
                                std::uint32_t first,
                                std::uint32_t second) = 0;
    virtual void HandleEffect(std::uint32_t handle,
                              std::uint32_t event) = 0;
    [[nodiscard]] virtual bool SmithActive() = 0;
    virtual void SetSmithActive(bool active) = 0;
    virtual void SetSize(std::uint32_t window,
                         std::uint32_t width_bits,
                         std::uint32_t height_bits) = 0;
    virtual void UpdateDisplay(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::int32_t GameDataEntry(
        std::uint32_t index) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window, bool enabled) = 0;
    [[nodiscard]] virtual std::uint8_t RemoveActiveFrame(
        std::uint32_t list, std::uint32_t adapter) = 0;
    virtual void InsertActiveFrame(std::uint32_t list,
                                   std::uint32_t adapter) = 0;
    [[nodiscard]] virtual std::uint8_t OnTabChange(
        std::uint32_t object, std::uint32_t tab) = 0;
    [[nodiscard]] virtual bool IsDialogVisible(
        std::uint32_t object) = 0;
    virtual void ResetState(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::uint32_t name) = 0;
    virtual void RemoveChildWindow(std::uint32_t parent,
                                   std::uint32_t child) = 0;
    virtual void ClearModelPreview(std::uint32_t object) = 0;
};

class ISmithWindowDisplayRuntime {
public:
    virtual ~ISmithWindowDisplayRuntime() = default;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetImage(std::uint32_t window,
                          std::string_view imageset,
                          std::string_view image) = 0;
    virtual void HandleEffect(std::uint32_t handle,
                              std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateModelPreview(
        std::uint32_t object, std::uint32_t first,
        std::uint32_t second, std::uint32_t third) = 0;
};

class ISmithWindowNameRuntime {
public:
    virtual ~ISmithWindowNameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view Utf8Text(
        std::uint32_t source) = 0;
    virtual void SetText(std::uint32_t window,
                         std::string_view text) = 0;
};

class ISmithWindowItemTextRuntime
    : public ISmithWindowNameRuntime {
public:
    virtual ~ISmithWindowItemTextRuntime() = default;
    [[nodiscard]] virtual std::string_view DescribeItem(
        std::uint32_t item) = 0;
    [[nodiscard]] virtual std::string_view ReadText(
        std::uint32_t source) = 0;
    [[nodiscard]] virtual std::string Utf8Buffer(
        std::string_view text) = 0;
};

class ISmithWindowEquipmentListRuntime
    : public ISmithWindowItemTextRuntime,
      public ISmithWindowQuantityRuntime {
public:
    virtual ~ISmithWindowEquipmentListRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) override = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void SetSortDirection(std::uint32_t list,
                                  std::uint32_t direction) = 0;
    virtual void ResetList(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::int32_t RightHoverItemId(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t CreateEmptyText() = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseText(
        std::uint32_t text) = 0;
    [[nodiscard]] virtual std::int32_t InventorySize(
        std::uint32_t category) = 0;
    [[nodiscard]] virtual std::uint32_t InventoryAt(
        std::uint32_t category, std::uint32_t index) = 0;
    virtual void AddRow(std::uint32_t list,
                        std::uint32_t row_id) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(
        std::uint32_t list) = 0;
    [[nodiscard]] virtual bool CheckPlayerCondition(
        std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t CreateNameItem(
        std::uint32_t list, std::uint32_t empty_text,
        std::string_view name, bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t CreateNumberItem(
        std::uint32_t list, std::string_view number) = 0;
    virtual void SetItem(std::uint32_t list,
                         std::uint32_t item, std::uint32_t column,
                         std::uint32_t row) = 0;
    virtual void SetImage(std::uint32_t window,
                          std::string_view imageset,
                          std::string_view image) = 0;
    virtual void SetHoverRow(std::uint32_t list,
                             std::uint32_t row) = 0;
    [[nodiscard]] virtual std::uint32_t HoverItem(
        std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t FindInventoryItem(
        std::uint32_t id, std::uint32_t category) = 0;
    virtual void Refresh(std::uint32_t object) = 0;
};

class ISmithWindowResetRuntime {
public:
    virtual ~ISmithWindowResetRuntime() = default;
    virtual void ClearSelection(std::uint32_t object) = 0;
    virtual void LoadData(std::uint32_t object,
                          std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(
        std::uint32_t list) = 0;
    virtual void SetHoverRow(std::uint32_t list,
                             std::uint32_t row) = 0;
    virtual void SetSelected(std::uint32_t button,
                             bool selected) = 0;
};

class ISmithWindowPrescriptionDescriptionRuntime
    : public ISmithWindowNameRuntime {
public:
    virtual ~ISmithWindowPrescriptionDescriptionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t EquipmentForId(
        std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string_view DescribeEquipment(
        std::uint32_t equipment) = 0;
    [[nodiscard]] virtual std::string Utf8Buffer(
        std::string_view text) = 0;
};

class ISmithWindowPrescriptionRuntime
    : public ISmithWindowPrescriptionDescriptionRuntime,
      public ISmithWindowQuantityRuntime {
public:
    virtual ~ISmithWindowPrescriptionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) override = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void SetSortDirection(std::uint32_t list,
                                  std::uint32_t direction) = 0;
    virtual void ResetList(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::int32_t ColumnCount(
        std::uint32_t list) = 0;
    virtual void RemoveColumn(std::uint32_t list,
                              std::uint32_t column) = 0;
    virtual void SetUserColumnSizing(std::uint32_t list,
                                     bool enabled) = 0;
    virtual void SetUserColumnDragging(std::uint32_t list,
                                       bool enabled) = 0;
    virtual void SetUserSortControl(std::uint32_t list,
                                    bool enabled) = 0;
    virtual void AddColumn(std::uint32_t list,
                           std::uint32_t column, float width) = 0;
    virtual void SetColumnLabelImage(std::uint32_t list,
                                     std::uint32_t column,
                                     std::string_view imageset,
                                     std::string_view image) = 0;
    [[nodiscard]] virtual std::int32_t InventoryCount(
        std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t InventoryIdAt(
        std::uint32_t mode, std::uint32_t index) = 0;
    [[nodiscard]] virtual bool HasRecipe(std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t RecipeForId(
        std::uint32_t id) = 0;
    virtual void AddRow(std::uint32_t list,
                        std::uint32_t row_id) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(
        std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t CreateCell(
        std::uint32_t list, std::string_view text) = 0;
    virtual void SetItem(std::uint32_t list,
                         std::uint32_t cell, std::uint32_t column,
                         std::uint32_t row) = 0;
    [[nodiscard]] virtual std::optional<std::string_view> EffectName(
        std::uint32_t equipment) = 0;
    [[nodiscard]] virtual std::uint32_t HoverItem(
        std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(
        std::uint32_t object) = 0;
};

class ISmithWindowHoverRuntime {
public:
    virtual ~ISmithWindowHoverRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t HoverItem(
        std::uint32_t list) = 0;
};

class ISmithWindowAvailableItemsRuntime
    : public ISmithWindowWordArrayRuntime {
public:
    virtual ~ISmithWindowAvailableItemsRuntime() = default;
    [[nodiscard]] virtual bool HasRecipe(std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t RecipeForId(
        std::uint32_t id) = 0;
    [[nodiscard]] virtual std::int32_t InventorySize(
        std::uint32_t category) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateEquipmentList(
        std::uint32_t object, std::uint32_t category) = 0;
};

class ISmithWindowInventoryCollectionRuntime
    : public ISmithWindowWordArrayRuntime {
public:
    virtual ~ISmithWindowInventoryCollectionRuntime() = default;
    [[nodiscard]] virtual std::int32_t ResolveSmithItemId(
        std::uint32_t input) = 0;
    [[nodiscard]] virtual bool HasDatabaseItem(
        std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t DatabaseItem(
        std::uint32_t id) = 0;
    [[nodiscard]] virtual std::int32_t InventorySize(
        std::uint32_t category) = 0;
    [[nodiscard]] virtual std::uint32_t InventoryAt(
        std::uint32_t category, std::uint32_t index) = 0;
};

class ISmithWindowCollectedListRuntime
    : public ISmithWindowItemTextRuntime,
      public ISmithWindowQuantityRuntime {
public:
    virtual ~ISmithWindowCollectedListRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) override = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void SetSortDirection(std::uint32_t list,
                                  std::uint32_t direction) = 0;
    virtual void ResetList(std::uint32_t list) = 0;
    virtual void AddRow(std::uint32_t list,
                        std::uint32_t row_id) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(
        std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t FindInventoryItem(
        std::uint32_t id, std::uint32_t category) = 0;
    [[nodiscard]] virtual bool CheckPlayerCondition(
        std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t CreateNameItem(
        std::uint32_t list, std::string_view name,
        bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t CreateNumberItem(
        std::uint32_t list, std::string_view number) = 0;
    virtual void SetItem(std::uint32_t list,
                         std::uint32_t item, std::uint32_t column,
                         std::uint32_t row) = 0;
    [[nodiscard]] virtual std::optional<std::string_view> CategoryName(
        std::uint32_t category) = 0;
    virtual void SetImage(std::uint32_t window,
                          std::string_view imageset,
                          std::string_view image) = 0;
    virtual void SetHoverRow(std::uint32_t list,
                             std::uint32_t row) = 0;
    [[nodiscard]] virtual std::uint32_t HoverItem(
        std::uint32_t list) = 0;
    virtual void Call49F080(std::uint32_t object) = 0;
    virtual void Call49EF80(std::uint32_t object) = 0;
    virtual void SetVisible(std::uint32_t window,
                            bool visible) = 0;
};

class ISmithWindowInventoryRefreshRuntime {
public:
    virtual ~ISmithWindowInventoryRefreshRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::int32_t PrescriptionHoverItemId(
        std::uint32_t object) = 0;
    virtual void TouchSmithRecord(std::int32_t id) = 0;
    [[nodiscard]] virtual bool HasInventoryItem(
        std::uint32_t id, std::uint32_t category) = 0;
    virtual void CollectMatchingInventoryItemIds(
        std::uint32_t object, std::int32_t id) = 0;
    virtual void RefreshCollectedEquipmentList(
        std::uint32_t object) = 0;
};

class ISmithWindowSelectionClearRuntime
    : public ISmithWindowItemTextRuntime {
public:
    virtual ~ISmithWindowSelectionClearRuntime() = default;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void SetSortDirection(std::uint32_t list,
                                  std::uint32_t direction) = 0;
    virtual void ResetList(std::uint32_t list) = 0;
    [[nodiscard]] virtual bool ColumnsInitialized() = 0;
    virtual void SetColumnsInitialized(bool initialized) = 0;
    [[nodiscard]] virtual std::int32_t ColumnCount(
        std::uint32_t list) = 0;
    virtual void RemoveColumn(std::uint32_t list,
                              std::uint32_t column) = 0;
    virtual void SetUserColumnSizing(std::uint32_t list,
                                     bool enabled) = 0;
    virtual void SetUserColumnDragging(std::uint32_t list,
                                       bool enabled) = 0;
    virtual void SetUserSortControl(std::uint32_t list,
                                    bool enabled) = 0;
    virtual void AddColumn(std::uint32_t list,
                           std::uint32_t column, float width) = 0;
    virtual void SetColumnLabelImage(std::uint32_t list,
                                     std::uint32_t column,
                                     std::string_view imageset,
                                     std::string_view image) = 0;
};

class ISmithWindowCategoryPageRuntime {
public:
    virtual ~ISmithWindowCategoryPageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void UpdateEquipmentList(std::uint32_t object,
                                     std::uint32_t category) = 0;
    virtual void SetVisible(std::uint32_t window,
                            bool visible) = 0;
};

class ISmithWindowSelectionPanelRuntime {
public:
    virtual ~ISmithWindowSelectionPanelRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void SetVisible(std::uint32_t window,
                            bool visible) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window,
                                bool enabled) = 0;
    virtual void HandleMouseEvent(std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    [[nodiscard]] virtual std::uint8_t InsertFrameRecord(
        std::uint32_t array, std::uint32_t record) = 0;
};

class ISmithWindowEffectPanelRuntime {
public:
    virtual ~ISmithWindowEffectPanelRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void SetVisible(std::uint32_t window,
                            bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    virtual void InsertFrameRecord(std::uint32_t array,
                                   std::uint32_t record) = 0;
    virtual void HandleEffectEvent(std::uint32_t handle,
                                   std::uint32_t event) = 0;
    [[nodiscard]] virtual std::array<std::uint32_t, 2> CreateEffect(
        std::uint32_t manager, std::string_view name,
        const std::array<float, 3>& position,
        std::uint32_t sentinel, std::uint32_t flag) = 0;
    virtual void ResetTemporaryEffect() = 0;
    [[nodiscard]] virtual std::uint8_t UpdateEffectProperties(
        std::uint32_t handle, const std::array<float, 3>& scale,
        std::uint8_t flag) = 0;
};

class ISmithWindowSelectionConfirmRuntime {
public:
    virtual ~ISmithWindowSelectionConfirmRuntime() = default;
    [[nodiscard]] virtual std::int32_t PrescriptionHoverItemId(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::int32_t EquipmentHoverItemId(
        std::uint32_t object) = 0;
    virtual void RegisterSmithSystem() = 0;
    [[nodiscard]] virtual std::uint32_t ElementProperty(
        std::int32_t prescription_id) = 0;
    [[nodiscard]] virtual bool CheckItemQuantity(
        std::uint32_t property) = 0;
    [[nodiscard]] virtual std::int32_t ProcessCrafting(
        std::int32_t prescription_id,
        std::int32_t equipment_id) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    virtual void CreateCommonDialog(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::string_view DialogTemplate(
        std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string_view EquipmentName(
        std::uint32_t property) = 0;
    [[nodiscard]] virtual std::string FormatMessage(
        std::string_view format, std::string_view item_name) = 0;
    [[nodiscard]] virtual std::uint32_t CreateCloseCallback(
        std::uint32_t object) = 0;
    virtual void ShowDialog(std::string_view message,
                            const std::array<float, 2>& size,
                            std::uint32_t style,
                            std::uint32_t& callback) = 0;
    virtual void DestroyCallback(std::uint32_t callback,
                                 std::uint32_t deleting) = 0;
    virtual void Call49F080(std::uint32_t object) = 0;
    virtual void Call49EF80(std::uint32_t object) = 0;
    virtual void HideSelectionPanel(std::uint32_t object) = 0;
};

class ISmithWindowItemUseRuntime
    : public ISmithWindowItemTextRuntime {
public:
    virtual ~ISmithWindowItemUseRuntime() = default;
    [[nodiscard]] virtual std::int32_t PrescriptionHoverItemId(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::int32_t EquipmentHoverItemId(
        std::uint32_t object) = 0;
    virtual void RegisterSmithSystem() = 0;
    [[nodiscard]] virtual std::int32_t ApplyItemEffects(
        std::int32_t prescription_id,
        std::int32_t equipment_id) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    virtual void CreateCommonDialog(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::string_view DialogTemplate(
        std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t FindInventoryItem(
        std::uint32_t id, std::uint32_t category) = 0;
    [[nodiscard]] virtual std::string FormatMessage(
        std::string_view format, std::string_view item_name) = 0;
    [[nodiscard]] virtual std::uint32_t CreateCloseCallback(
        std::uint32_t object) = 0;
    virtual void ShowDialog(std::string_view message,
                            const std::array<float, 2>& size,
                            std::uint32_t style,
                            std::uint32_t& callback) = 0;
    virtual void DestroyCallback(std::uint32_t callback,
                                 std::uint32_t deleting) = 0;
    [[nodiscard]] virtual std::uint32_t HoverItem(
        std::uint32_t list) = 0;
    virtual void Call49F080(std::uint32_t object) = 0;
    virtual void Call49EF80(std::uint32_t object) = 0;
    virtual void HideSelectionPanel(std::uint32_t object) = 0;
};

class ISmithWindowEquipmentPromptRuntime {
public:
    virtual ~ISmithWindowEquipmentPromptRuntime() = default;
    [[nodiscard]] virtual std::int32_t PrescriptionHoverItemId(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::int32_t EquipmentHoverItemId(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    virtual void CreateCommonDialog(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t CreateSelectCallback(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t CreatePageChangeCallback(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t FindInventoryItem(
        std::uint32_t id, std::uint32_t category) = 0;
    virtual void AssignText(std::uint32_t destination,
                            std::uint32_t source) = 0;
    [[nodiscard]] virtual std::string_view DialogTemplate(
        std::uint32_t id) = 0;
    virtual void ShowDialog(std::string_view message,
                            const std::array<float, 2>& size,
                            std::uint32_t style,
                            std::uint32_t& callback) = 0;
    virtual void DestroyCallback(std::uint32_t callback,
                                 std::uint32_t deleting) = 0;
};

class ISmithWindowEquipmentSelectionRuntime
    : public virtual ISmithWindowItemUseRuntime {
public:
    virtual ~ISmithWindowEquipmentSelectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ItemRowIndex(
        std::uint32_t list, std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(
        std::uint32_t list) = 0;
    virtual void SetHoverRow(std::uint32_t list,
                             std::uint32_t row) = 0;
};

class ISmithWindowExtendedCraftRuntime
    : public virtual ISmithWindowItemUseRuntime {
public:
    virtual ~ISmithWindowExtendedCraftRuntime() = default;
    [[nodiscard]] virtual std::int32_t ProcessExtendedCraft(
        std::int32_t prescription_id,
        std::int32_t equipment_id) = 0;
    [[nodiscard]] virtual std::string ItemDataName(
        std::int32_t equipment_id) = 0;
};

class ISmithWindowDialogCloseRuntime {
public:
    virtual ~ISmithWindowDialogCloseRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    virtual void InsertFrameRecord(std::uint32_t array,
                                   std::uint32_t record) = 0;
};

class ISmithWindowEquipmentClickRuntime
    : public ISmithWindowHoverRuntime,
      public ISmithWindowItemTextRuntime {
public:
    virtual ~ISmithWindowEquipmentClickRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) override = 0;
    [[nodiscard]] virtual std::uint32_t FindInventoryItem(
        std::uint32_t id, std::uint32_t category) = 0;
};

class ISmithWindowPrescriptionClickRuntime
    : public ISmithWindowPrescriptionDescriptionRuntime {
public:
    virtual ~ISmithWindowPrescriptionClickRuntime() = default;
    virtual void ClearEquipmentSelection(std::uint32_t object) = 0;
    virtual void HideSelectionPanel(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t HoverItem(
        std::uint32_t list) = 0;
    [[nodiscard]] virtual bool HasRecipe(std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t RecipeForId(
        std::uint32_t id) = 0;
};

class ISmithWindowResetClickRuntime {
public:
    virtual ~ISmithWindowResetClickRuntime() = default;
    [[nodiscard]] virtual std::uint8_t OnResetClick(
        std::uint32_t object) = 0;
};

class ISmithWindowConfirmClickRuntime {
public:
    virtual ~ISmithWindowConfirmClickRuntime() = default;
    [[nodiscard]] virtual std::uint8_t OnConfirmTrade(
        std::uint32_t object) = 0;
};

enum class SmithWindowTradeCallback {
    SelectionConfirm,
    EquipmentSelectionConfirm,
    EquipmentSelectionPrompt,
    ItemUseConfirm,
    ExtendedCraftConfirm,
    PageChangePrompt,
    PageChangeConfirm,
};

class ISmithWindowDialogRuntime {
public:
    virtual ~ISmithWindowDialogRuntime() = default;
    [[nodiscard]] virtual std::string_view DialogTemplate(std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string FormatMessage(
        std::string_view format, std::string_view name) = 0;
    virtual void CreateCommonDialog() = 0;
    [[nodiscard]] virtual std::uint32_t CreateCallback(
        std::uint32_t object, SmithWindowTradeCallback kind) = 0;
    virtual void ShowDialog(std::string_view message,
                            const std::array<float, 2>& size,
                            std::uint32_t style,
                            std::uint32_t& callback) = 0;
    virtual void DestroyCallback(std::uint32_t callback,
                                 std::uint32_t deleting) = 0;
};

class ISmithWindowTradeRuntime : public ISmithWindowDialogRuntime {
public:
    virtual ~ISmithWindowTradeRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t EquipmentHoverItemId(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::int32_t PrescriptionHoverItemId(std::uint32_t object) = 0;
    [[nodiscard]] virtual bool SmithActive() = 0;
    [[nodiscard]] virtual std::int32_t InventoryCount() = 0;
    [[nodiscard]] virtual std::uint32_t EquipmentRowCount(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::int32_t EquipmentProperty(std::int32_t equipment_id) = 0;
    [[nodiscard]] virtual bool QuantityBelowRequirement(
        std::int32_t prescription_id, std::int32_t equipment_id) = 0;
    [[nodiscard]] virtual bool CraftingRequirementsMet(std::int32_t prescription_id) = 0;
    [[nodiscard]] virtual bool EquipmentPropertySufficient(
        std::int32_t prescription_id, std::int32_t equipment_id) = 0;
    [[nodiscard]] virtual bool EquipmentPropertyNonZero(std::int32_t equipment_id) = 0;
    [[nodiscard]] virtual bool EquipmentPropertyMatches(
        std::int32_t equipment_id, std::int32_t prescription_id) = 0;
    [[nodiscard]] virtual std::string_view ElementPropertyName(
        std::int32_t prescription_id) = 0;
    [[nodiscard]] virtual std::string_view InventoryTextName(
        std::int32_t equipment_id) = 0;
    virtual void AssignInventoryText(std::uint32_t destination,
                                     std::int32_t equipment_id) = 0;
    [[nodiscard]] virtual std::string_view InventoryDisplayName(
        std::int32_t equipment_id) = 0;
    [[nodiscard]] virtual std::string_view ItemDataName(
        std::int32_t equipment_id) = 0;
    [[nodiscard]] virtual std::string_view TradeDataName(
        std::int32_t prescription_id) = 0;
};

class ISmithWindowConfirmTradeRuntime : public ISmithWindowDialogRuntime {
public:
    virtual ~ISmithWindowConfirmTradeRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t PrescriptionHoverItemId(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual bool SmithActive() = 0;
    [[nodiscard]] virtual std::int32_t InventoryCount() = 0;
    [[nodiscard]] virtual bool CraftingRequirementsMet(
        std::int32_t prescription_id) = 0;
    [[nodiscard]] virtual bool HasElements(std::int32_t prescription_id) = 0;
    virtual void CollectMatchingInventoryItemIds(
        std::uint32_t object, std::int32_t prescription_id) = 0;
    virtual void UpdateAvailableEquipmentCategories(
        std::uint32_t object, std::int32_t prescription_id) = 0;
    virtual void ShowSelectionPanel(std::uint32_t object) = 0;
    virtual void RefreshCollectedEquipmentList(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::string_view TradeDataName(
        std::int32_t prescription_id) = 0;
    [[nodiscard]] virtual std::string_view ElementPropertyName(
        std::int32_t prescription_id) = 0;
};

class ISmithWindowCategoryNavigationRuntime {
public:
    virtual ~ISmithWindowCategoryNavigationRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void RefreshAfterInventoryChange(std::uint32_t object) = 0;
    virtual void UpdateCategoryPageButtons(std::uint32_t object) = 0;
};

class ISmithWindowModeClickRuntime {
public:
    virtual ~ISmithWindowModeClickRuntime() = default;
    virtual void ClearEquipmentSelection(std::uint32_t object) = 0;
    virtual void LoadData(std::uint32_t object,
                          std::uint32_t requested_mode) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(std::uint32_t list) = 0;
    virtual void SetHoverRow(std::uint32_t list,
                             std::uint32_t row) = 0;
    virtual void SetImage(std::uint32_t image,
                          std::string_view imageset,
                          std::string_view name) = 0;
    virtual void SetSelected(std::uint32_t button,
                             bool selected) = 0;
    virtual void UpdateModelPreview(std::string_view model,
                               std::string_view key,
                               std::string_view path) = 0;
};

class ISmithWindowCraftTabRuntime {
public:
    virtual ~ISmithWindowCraftTabRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void SetImage(std::uint32_t image,
                          std::string_view imageset,
                          std::string_view name) = 0;
    virtual void ProcessUIEvent(std::uint32_t key,
                                std::uint32_t value) = 0;
    virtual void UpdateSystemDialogVisibility(std::uint32_t key) = 0;
    virtual void ShowEffectPanel(std::uint32_t object) = 0;
    virtual void HideSelectionPanel(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint8_t OnCraftModeClick(
        std::uint32_t object, std::uint32_t mode) = 0;
};

class ISmithWindowCloseConfirmRuntime {
public:
    virtual ~ISmithWindowCloseConfirmRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t HandleBaseKeyEvent(
        std::uint32_t object, std::uint32_t event) = 0;
    [[nodiscard]] virtual bool SmithActive() = 0;
    virtual void ClearModelPreview(std::uint32_t trade_object) = 0;
    virtual void ProcessUIEvent(std::uint32_t key,
                                std::uint32_t first,
                                std::uint32_t second) = 0;
};

class ISmithWindowPrescriptionAvailabilityRuntime {
public:
    virtual ~ISmithWindowPrescriptionAvailabilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::int32_t RowCount(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::int32_t ColumnCount(
        std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t ItemAt(
        std::uint32_t list, std::int32_t row,
        std::int32_t column) = 0;
    virtual void RegisterSmithSystem() = 0;
    [[nodiscard]] virtual bool CraftingRequirementsMet(
        std::uint32_t recipe_id) = 0;
};

class ISmithWindowEquipmentAvailabilityRuntime {
public:
    virtual ~ISmithWindowEquipmentAvailabilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    [[nodiscard]] virtual std::int32_t RowCount(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::int32_t ColumnCount(
        std::uint32_t list) = 0;
    [[nodiscard]] virtual std::int32_t PrescriptionHoverItemId(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ItemAt(
        std::uint32_t list, std::int32_t row,
        std::int32_t column) = 0;
    virtual void RegisterSmithSystem() = 0;
    [[nodiscard]] virtual std::int32_t EquipmentProperty(
        std::uint32_t equipment_id) = 0;
    [[nodiscard]] virtual bool EquipmentPropertySufficient(
        std::int32_t prescription_id,
        std::uint32_t equipment_id) = 0;
};

class SmithWindow final {
public:
    // IDA 0x496c20.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint32_t layout_name,
        ISmithWindowRuntime& runtime);
    // IDA 0x497330.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object,
        ISmithWindowDestructionRuntime& runtime);
    // IDA 0x497420.
    [[nodiscard]] static std::uint8_t SetVisible(
        std::uint32_t object, std::uint8_t visible,
        std::uint32_t parent,
        ISmithWindowVisibilityRuntime& runtime);
    // IDA 0x4976e0.
    [[nodiscard]] static std::uint32_t UpdateDisplay(
        std::uint32_t object,
        ISmithWindowDisplayRuntime& runtime);
    // IDA 0x497900.
    [[nodiscard]] static std::uint32_t UpdateEquipmentList(
        std::uint32_t object, std::uint32_t category,
        ISmithWindowEquipmentListRuntime& runtime);
    // IDA 0x498350.
    static void UpdateEquipmentItemName(
        std::uint32_t object, std::uint32_t item,
        ISmithWindowNameRuntime& runtime);
    // IDA 0x498590.
    static void UpdateEquipmentItemDescription(
        std::uint32_t object, std::uint32_t item,
        ISmithWindowItemTextRuntime& runtime);
    // IDA 0x4988a0.
    static void ResetState(std::uint32_t object,
                           ISmithWindowResetRuntime& runtime);
    // IDA 0x4988f0.
    [[nodiscard]] static std::uint32_t LoadData(
        std::uint32_t object, std::uint32_t mode,
        ISmithWindowPrescriptionRuntime& runtime);
    // IDA 0x499a60.
    static void UpdatePrescriptionItemName(
        std::uint32_t object, std::uint32_t recipe,
        ISmithWindowNameRuntime& runtime);
    // IDA 0x499ca0.
    static void UpdatePrescriptionItemDescription(
        std::uint32_t object, std::uint32_t recipe,
        ISmithWindowPrescriptionDescriptionRuntime& runtime);
    // IDA 0x499f10.
    [[nodiscard]] static std::int32_t GetEquipmentHoverItemId(
        std::uint32_t object, ISmithWindowHoverRuntime& runtime);
    // IDA 0x499f30.
    [[nodiscard]] static std::int32_t GetPrescriptionHoverItemId(
        std::uint32_t object, ISmithWindowHoverRuntime& runtime);
    // IDA 0x499f50.
    [[nodiscard]] static std::uint8_t UpdateAvailableEquipmentCategories(
        std::uint32_t object, std::uint32_t source,
        ISmithWindowAvailableItemsRuntime& runtime);
    // IDA 0x49a070.
    [[nodiscard]] static std::uint8_t CollectMatchingInventoryItemIds(
        std::uint32_t object, std::uint32_t input,
        ISmithWindowInventoryCollectionRuntime& runtime);
    // IDA 0x49a2c0.
    static void RefreshCollectedEquipmentList(
        std::uint32_t object,
        ISmithWindowCollectedListRuntime& runtime);
    // IDA 0x49ae00.
    static void RefreshAfterInventoryChange(
        std::uint32_t object,
        ISmithWindowInventoryRefreshRuntime& runtime);
    // IDA 0x49aea0.
    static void ClearEquipmentSelection(
        std::uint32_t object,
        ISmithWindowSelectionClearRuntime& runtime);
    // IDA 0x49b260.
    static void UpdateCategoryPageButtons(
        std::uint32_t object,
        ISmithWindowCategoryPageRuntime& runtime);
    // IDA 0x49b2e0.
    [[nodiscard]] static std::uint8_t ShowSelectionPanel(
        std::uint32_t object,
        ISmithWindowSelectionPanelRuntime& runtime);
    // IDA 0x49b450.
    static void HideSelectionPanel(
        std::uint32_t object,
        ISmithWindowSelectionPanelRuntime& runtime);
    // IDA 0x49b360.
    [[nodiscard]] static std::uint8_t ShowEffectPanel(
        std::uint32_t object,
        ISmithWindowEffectPanelRuntime& runtime);
    // IDA 0x49b4a0.
    [[nodiscard]] static std::uint8_t OnSelectionConfirm(
        std::uint32_t object, std::uint32_t event,
        ISmithWindowSelectionConfirmRuntime& runtime);
    // IDA 0x49b6c0.
    [[nodiscard]] static std::uint8_t OnItemUseConfirm(
        std::uint32_t object, std::uint32_t event,
        ISmithWindowItemUseRuntime& runtime);
    // IDA 0x49b9e0.
    [[nodiscard]] static std::uint8_t OnEquipmentSelectionPrompt(
        std::uint32_t object, std::uint32_t event,
        ISmithWindowEquipmentPromptRuntime& runtime);
    // IDA 0x49c0e0.
    [[nodiscard]] static std::uint8_t OnPageChangePrompt(
        std::uint32_t object, std::uint32_t event,
        ISmithWindowEquipmentPromptRuntime& runtime);
    // IDA 0x49bb70.
    [[nodiscard]] static std::uint8_t OnEquipmentSelectionConfirm(
        std::uint32_t object, std::uint32_t event,
        ISmithWindowEquipmentSelectionRuntime& runtime);
    // IDA 0x49bef0.
    [[nodiscard]] static std::uint8_t OnExtendedCraftConfirm(
        std::uint32_t object, std::uint32_t event,
        ISmithWindowExtendedCraftRuntime& runtime);
    // IDA 0x49c250.
    [[nodiscard]] static std::uint8_t OnPageChangeConfirm(
        std::uint32_t object, std::uint32_t event,
        ISmithWindowExtendedCraftRuntime& runtime);
    // IDA 0x49c440.
    [[nodiscard]] static std::uint8_t OnDialogClose(
        std::uint32_t object, std::uint32_t callback_argument,
        ISmithWindowDialogCloseRuntime& runtime);
    // IDA 0x49c460.
    [[nodiscard]] static std::uint8_t OnEquipmentListClick(
        std::uint32_t object, std::uint32_t event,
        ISmithWindowEquipmentClickRuntime& runtime);
    // IDA 0x49c4a0.
    [[nodiscard]] static std::uint8_t OnPrescriptionListClick(
        std::uint32_t object, std::uint32_t event,
        ISmithWindowPrescriptionClickRuntime& runtime);
    // IDA 0x49c520.
    [[nodiscard]] static std::uint8_t OnResetClickThunk(
        std::uint32_t object, std::uint32_t ignored_argument,
        ISmithWindowResetClickRuntime& runtime);
    // IDA 0x49c530.
    [[nodiscard]] static std::uint8_t OnResetTrade(
        std::uint32_t object, ISmithWindowTradeRuntime& runtime);
    // IDA 0x49d5c0.
    [[nodiscard]] static std::uint8_t OnConfirmClickThunk(
        std::uint32_t object, std::uint32_t ignored_argument,
        ISmithWindowConfirmClickRuntime& runtime);
    // IDA 0x49d5d0.
    [[nodiscard]] static std::uint8_t OnConfirmTrade(
        std::uint32_t object, ISmithWindowConfirmTradeRuntime& runtime);
    // IDA 0x49dd00.
    [[nodiscard]] static std::uint8_t OnCategoryPreviousClick(
        std::uint32_t object, std::uint32_t ignored_event,
        ISmithWindowCategoryNavigationRuntime& runtime);
    // IDA 0x49dd80.
    [[nodiscard]] static std::uint8_t OnCategoryNextClick(
        std::uint32_t object, std::uint32_t ignored_event,
        ISmithWindowCategoryNavigationRuntime& runtime);
    // IDA 0x49de00.
    [[nodiscard]] static std::uint8_t OnCraftModeClick(
        std::uint32_t object, std::uint32_t requested_mode,
        ISmithWindowModeClickRuntime& runtime);
    // IDA 0x49e200/0x49e360/0x49e4c0.
    [[nodiscard]] static std::uint8_t OnRongzhuTabClick(
        std::uint32_t object, std::uint32_t ignored_event,
        ISmithWindowCraftTabRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnDuanyeTabClick(
        std::uint32_t object, std::uint32_t ignored_event,
        ISmithWindowCraftTabRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnZhulingTabClick(
        std::uint32_t object, std::uint32_t ignored_event,
        ISmithWindowCraftTabRuntime& runtime);
    // IDA 0x49e620.
    [[nodiscard]] static std::uint8_t OnCloseConfirm(
        std::uint32_t object, std::uint32_t event,
        ISmithWindowCloseConfirmRuntime& runtime);
    // IDA 0x49e6c0.
    [[nodiscard]] static std::uint8_t DispatchCraftTabEvent(
        std::uint32_t adapter, std::uint32_t event,
        ISmithWindowCraftTabRuntime& runtime);
    // IDA 0x49ef80.
    [[nodiscard]] static std::int32_t RefreshPrescriptionAvailability(
        std::uint32_t object,
        ISmithWindowPrescriptionAvailabilityRuntime& runtime);
    // IDA 0x49f080.
    static void RefreshEquipmentAvailability(
        std::uint32_t object,
        ISmithWindowEquipmentAvailabilityRuntime& runtime);
};

}  // namespace pal4
