#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

// Construction and callback transport only; list population and game actions
// remain required by the full runtime below.
class ICombatSelectConstructionRuntime {
public:
    virtual ~ICombatSelectConstructionRuntime()=default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void LoadLayout(std::uint32_t window, std::uint32_t layout,
                            std::uint32_t group, std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::string_view ReadCString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetWindowByName(
        std::uint32_t window, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t GetWindow(
        std::uint32_t window, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t SubscribeMouseClick(
        std::uint32_t control, std::uint32_t callback) = 0;
    virtual void ReleaseSubscription(std::uint32_t subscription) = 0;
    virtual void SetSortColumn(std::uint32_t list, std::uint32_t column) = 0;
    virtual void SetSortDirection(std::uint32_t list,
                                  std::uint32_t direction) = 0;
    virtual void SetUserColumnSizingEnabled(std::uint32_t list,
                                            bool enabled) = 0;
    virtual void SetUserColumnDraggingEnabled(std::uint32_t list,
                                              bool enabled) = 0;
    virtual void SetUserSortControlEnabled(std::uint32_t list,
                                           bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTemporaryString() = 0;
    [[nodiscard]] virtual std::uint32_t SubscribeListHover(
        std::uint32_t list, std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t SubscribeButtonClick(
        std::uint32_t button, std::uint32_t callback) = 0;
    virtual void SetButtonEnabled(std::uint32_t button, bool enabled) = 0;
};

class ICombatSelectWindowRuntime : public ICombatSelectConstructionRuntime {
public:
    virtual ~ICombatSelectWindowRuntime() = default;
    virtual void InvokeVirtualSelection(std::uint32_t window,
                                        std::uint32_t vtable_offset) = 0;
    virtual void InvokeVisibility(std::uint32_t window,
                                  std::uint32_t visible,
                                  std::uint32_t parent) = 0;
    virtual void SetText(std::uint32_t control,
                         std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t InsertEventHandler(
        std::uint32_t array, std::uint32_t facet) = 0;
    virtual void ResetList(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t GetColumnCount(std::uint32_t list) = 0;
    virtual void RemoveColumn(std::uint32_t list, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::string_view GetGameString(std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(std::string_view text) = 0;
    virtual void AddColumn(std::uint32_t list, std::string_view title,
                           std::uint32_t id, float width) = 0;
    [[nodiscard]] virtual std::uint32_t GetSkillIterator(
        std::uint32_t collection, std::uint32_t index) = 0;
    [[nodiscard]] virtual bool IteratorDone(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorSkillId(
        std::uint32_t iterator) = 0;
    virtual void AdvanceIterator(std::uint32_t iterator) = 0;
    virtual void ReleaseIterator(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t FindMagic(std::uint32_t id) = 0;
    virtual void AddRow(std::uint32_t list, std::uint32_t index) = 0;
    virtual void ConstructNameItem(std::uint32_t item, std::uint32_t list,
                                   std::string_view name, bool selected) = 0;
    virtual void ConstructCostItem(std::uint32_t item, std::uint32_t list,
                                   std::string_view cost) = 0;
    [[nodiscard]] virtual std::uint32_t GetImage(
        std::string_view imageset, std::string_view image) = 0;
    [[nodiscard]] virtual std::uint32_t GetMagicKey(std::uint32_t magic) = 0;
    [[nodiscard]] virtual std::uint32_t GetRowCount(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t GetColumnWithId(
        std::uint32_t list, std::uint32_t id) = 0;
    virtual void SetListItem(std::uint32_t list, std::uint32_t item,
                             std::uint32_t row, std::uint32_t column) = 0;
    [[nodiscard]] virtual std::int32_t GetActorStat(
        std::uint32_t actor, std::uint32_t stat) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void SetHoverRowIndex(std::uint32_t list,
                                  std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t GetHoverItem(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::int32_t IteratorSize(
        std::uint32_t iterator) = 0;
    virtual void SetButtonSelected(std::uint32_t button, bool selected) = 0;
    virtual void SetButtonVisible(std::uint32_t button, bool visible) = 0;
    [[nodiscard]] virtual bool IsButtonDisabled(std::uint32_t button) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveEventHandler(
        std::uint32_t array, std::uint32_t facet) = 0;
    virtual void InvokeSelectionCallback(std::uint32_t callback,
                                         std::uint32_t owner,
                                         std::uint32_t selection) = 0;
    [[nodiscard]] virtual std::int32_t GetInventoryCount(
        std::uint32_t group) = 0;
    [[nodiscard]] virtual std::uint32_t GetInventoryElement(
        std::uint32_t group, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t FindProperty(
        std::uint32_t id) = 0;
};

class ICombatStuntSelectWindowRuntime : public ICombatSelectWindowRuntime {
public:
    [[nodiscard]] virtual std::uint32_t GetStuntIterator(
        std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t FindStunt(std::uint32_t id) = 0;
    [[nodiscard]] virtual bool IsStuntUsable(
        std::uint32_t actor, std::uint32_t stunt_record) = 0;
};

using ICombatMagicSelectWindowRuntime = ICombatSelectWindowRuntime;

// Delegate hover movement to the original CEGUI list implementation.
class ICombatSelectListNavigationRuntime {
public:
    virtual ~ICombatSelectListNavigationRuntime() = default;
    virtual void UpHoverItem(std::uint32_t list) = 0;
    virtual void DownHoverItem(std::uint32_t list) = 0;
};

class CombatSelectEventRecord final {
public:
    // IDA 0x555090 and 0x5550b0. Word +4 is left untouched.
    [[nodiscard]] static std::uint32_t ConstructMagic(
        std::uint32_t record, std::uint32_t function,
        std::uint32_t context, std::uint32_t owner,
        ICombatSelectWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructProperty(
        std::uint32_t record, std::uint32_t function,
        std::uint32_t context, std::uint32_t owner,
        ICombatSelectWindowRuntime& runtime);
};

class CombatMagicSelectBase {
public:
    static constexpr std::uint32_t kVtable = 0x8455b8u;
    static constexpr std::uint32_t kClickCallbackVtable = 0x8455d4u;
    static constexpr std::uint32_t kClickCallback = 0x552380u;

    // IDA 0x552190.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t window, std::uint32_t layout, std::uint32_t group,
        ICombatSelectConstructionRuntime& runtime);
    // IDA 0x5521f0.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t window, ICombatSelectConstructionRuntime& runtime);
    // IDA 0x552380.
    [[nodiscard]] static std::uint8_t HandleRootClick(
        std::uint32_t window, std::uint32_t event,
        ICombatMagicSelectWindowRuntime& runtime);
};

class CombatMagicSelectWindow final : public CombatMagicSelectBase {
public:
    static constexpr std::uint32_t kLayoutName = 0x8b755cu;
    static constexpr std::uint32_t kResourceGroup = 0x8b7574u;
    static constexpr std::uint32_t kVtable = 0x8455f4u;
    static constexpr std::uint32_t kFacetVtable = 0x8455dcu;
    static constexpr std::uint32_t kEventVtable = 0x845610u;
    static constexpr std::uint32_t kHoverCallback = 0x5530d0u;
    static constexpr std::uint32_t kListClickCallback = 0x552ff0u;
    static constexpr std::uint32_t kButtonClickCallback = 0x552fd0u;

    // IDA 0x5523a0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t window, ICombatSelectConstructionRuntime& runtime);
    // IDA 0x552560.
    [[nodiscard]] static std::uint32_t InitializeButtons(
        std::uint32_t window, ICombatSelectConstructionRuntime& runtime);
    // IDA 0x552410.
    [[nodiscard]] static std::uint32_t Setup(
        std::uint32_t window, std::uint32_t pair,
        std::uint32_t mode, std::uint32_t parent,
        ICombatMagicSelectWindowRuntime& runtime);
    // IDA 0x552830.
    static void UpdateMagicList(std::uint32_t window,
                                ICombatMagicSelectWindowRuntime& runtime);
    // IDA 0x552ed0.
    [[nodiscard]] static std::uint32_t GetHoveredMagic(
        std::uint32_t window, ICombatMagicSelectWindowRuntime& runtime);
    // IDA 0x552f10.
    static void SelectButton(std::uint32_t window, std::uint32_t selection,
                             ICombatMagicSelectWindowRuntime& runtime);
    // IDA 0x552fd0.
    [[nodiscard]] static std::uint8_t HandleButtonClick(
        std::uint32_t window, std::uint32_t event,
        ICombatMagicSelectWindowRuntime& runtime);
    // IDA 0x552ff0 and 0x553000.
    [[nodiscard]] static std::uint8_t HandleListClick(
        std::uint32_t window, ICombatMagicSelectWindowRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandleListClickInternal(
        std::uint32_t window, ICombatMagicSelectWindowRuntime& runtime);
    // IDA 0x553050 and 0x553090.
    [[nodiscard]] static std::int32_t FindNextEnabledButton(
        std::uint32_t window, std::int32_t current,
        ICombatMagicSelectWindowRuntime& runtime);
    [[nodiscard]] static std::int32_t FindPreviousEnabledButton(
        std::uint32_t window, std::int32_t current,
        ICombatMagicSelectWindowRuntime& runtime);
    // IDA 0x5530d0.
    [[nodiscard]] static std::uint8_t UpdateMagicDescription(
        std::uint32_t window, ICombatMagicSelectWindowRuntime& runtime);
    // IDA 0x553350.
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t window, ICombatMagicSelectWindowRuntime& runtime);
    // IDA 0x553380, facet at window+48; event+16 is a DIK scan code.
    [[nodiscard]] static std::uint8_t HandleKeyDown(
        std::uint32_t window, std::uint32_t event,
        ICombatMagicSelectWindowRuntime& runtime,
        ICombatSelectListNavigationRuntime& navigation);
};

}  // namespace pal4
