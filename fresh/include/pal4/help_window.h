#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {

enum class HelpWindowEvent {
    Clicked,
    HoverRowChanged,
    MouseClick,
    KeyDown,
};

struct HelpTopicEntry {
    std::uint32_t id{};
    std::string title;
};

struct HelpContentEntry {
    std::uint32_t address{};
    std::uint32_t id{};
    std::uint32_t topic_id{};
    std::string title;
    std::int32_t unlock_progress{};
};

class IHelpWindowRuntime {
public:
    virtual ~IHelpWindowRuntime() = default;
    virtual void LoadLayout(std::uint32_t object, std::uint32_t name) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t GetWindow(
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, HelpWindowEvent event,
        std::uint32_t callback) = 0;
    virtual void ReleaseConnection(std::uint32_t connection) = 0;
    [[nodiscard]] virtual std::uint32_t ColumnCount(
        std::uint32_t list) = 0;
    virtual void RemoveColumn(std::uint32_t list,
                              std::uint32_t index) = 0;
    virtual void SetSortDirection(std::uint32_t list,
                                  std::uint32_t direction) = 0;
    virtual void SetUserColumnSizing(std::uint32_t list, bool enabled) = 0;
    virtual void SetUserColumnDragging(std::uint32_t list, bool enabled) = 0;
    virtual void SetUserSortControl(std::uint32_t list, bool enabled) = 0;
    virtual void AddColumn(std::uint32_t list, std::u32string_view name,
                           std::uint32_t id, float width) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window, bool value) = 0;
    [[nodiscard]] virtual std::uint32_t TickCount() = 0;
    virtual void ClassifyTimeValue(std::uint32_t value) = 0;
    virtual void SetParent(std::uint32_t object, bool visible,
                           std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void RemoveCallback(std::uint32_t collection,
                                std::uint32_t callback) = 0;
    virtual void InsertCallback(std::uint32_t collection,
                                std::uint32_t callback) = 0;
    virtual void SetVisible(std::uint32_t window, bool value) = 0;
    virtual void ResetList(std::uint32_t list) = 0;
    virtual void SetText(std::uint32_t window, std::u32string_view text) = 0;
    virtual void SetModal(std::uint32_t window, bool value) = 0;
    virtual void HandleFrameEvent(std::uint32_t name_pointer,
                                  const std::array<std::uint32_t, 2>& event) = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::uint32_t name_pointer) = 0;
    [[nodiscard]] virtual std::vector<HelpTopicEntry> TopicEntries() = 0;
    [[nodiscard]] virtual std::vector<HelpContentEntry> ContentEntries() = 0;
    [[nodiscard]] virtual std::string ReadText(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    virtual void AddRow(std::uint32_t list, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructListItem(
        std::uint32_t storage, std::uint32_t list,
        std::u32string_view text, bool unread) = 0;
    virtual void FreeItem(std::uint32_t item) = 0;
    virtual void SetItem(std::uint32_t list, std::uint32_t item,
                         std::uint32_t column, std::uint32_t row) = 0;
    virtual void ScrollRowToVisible(std::uint32_t list,
                                    std::uint32_t row) = 0;
    virtual void HandleMouseEvent(std::uint32_t window) = 0;
    virtual void MoveHoverItem(std::uint32_t list, bool up) = 0;
    [[nodiscard]] virtual std::uint32_t HoverItem(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t GlobalData() = 0;
    virtual void SaveSettings(std::uint32_t global_data) = 0;
    virtual void SetListItemImage(std::uint32_t item,
                                  std::uint32_t image) = 0;
    virtual void RequestRedraw(std::uint32_t window) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void SetHoverRow(std::uint32_t list, std::int32_t row) = 0;
    virtual void SetScrollPosition(std::uint32_t scrollbar,
                                   float position) = 0;
    [[nodiscard]] virtual bool IsWindowVisible(
        std::uint32_t window, bool recursive) = 0;
    virtual void InvokeCallback(std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint8_t ProcessUiEvents(
        std::uint32_t collection, std::uint32_t key,
        std::uint32_t handler) = 0;
};

class HelpWindow final {
public:
    static constexpr std::uint32_t kVtable = 0x843f80u;
    static constexpr std::uint32_t kSecondaryVtable = 0x843f68u;
    static constexpr std::uint32_t kCallbackVtable = 0x843fc8u;
    static constexpr std::uint32_t kNamePointer = 0x8b13acu;

    // IDA 0x4a7590-0x4a8fb0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint32_t layout_name,
        IHelpWindowRuntime& runtime);
    static void SetVisible(std::uint32_t object, bool visible,
                           std::uint32_t parent, IHelpWindowRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandleShow(
        std::uint32_t event, IHelpWindowRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnShow(
        std::uint32_t object, IHelpWindowRuntime& runtime);
    [[nodiscard]] static std::uint8_t RefreshContentList(
        std::uint32_t object, IHelpWindowRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnListChanged(
        std::uint32_t object, IHelpWindowRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnItemClick(
        std::uint32_t object, IHelpWindowRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnClose(
        std::uint32_t object, IHelpWindowRuntime& runtime);
    static void InitializeControls(std::uint32_t object,
                                   IHelpWindowRuntime& runtime);
    static void PopulateTopicList(std::uint32_t object,
                                  std::uint32_t selected_topic,
                                  IHelpWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t PopulateContentList(
        std::uint32_t object, IHelpWindowRuntime& runtime);
    static void SetDescription(std::uint32_t object,
                               std::uint32_t content,
                               IHelpWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t SelectedTopicId(
        std::uint32_t object, IHelpWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t SelectedContentData(
        std::uint32_t object, IHelpWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t HelpDataCount(
        std::uint32_t topic, IHelpWindowRuntime& runtime);
    [[nodiscard]] static bool HasUnreadHelp(
        std::uint32_t topic, IHelpWindowRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnKeyDown(
        std::uint32_t callback, std::uint32_t event,
        IHelpWindowRuntime& runtime);
    [[nodiscard]] static std::int32_t HelpDataSize(
        std::uint32_t collection, IHelpWindowRuntime& runtime);
    [[nodiscard]] static std::uint8_t ProcessEvents(
        std::uint32_t handler, IHelpWindowRuntime& runtime);
};

}  // namespace pal4
