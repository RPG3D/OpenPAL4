#include "pal4/help_window.h"
#include "pal4/bitset_bounds_error.h"

#include <array>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IHelpWindowRuntime {
    struct Subscription {
        std::uint32_t window;
        pal4::HelpWindowEvent event;
        std::uint32_t callback;
    };
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x1024u, 0x5001u}, {0x102cu, 0x5002u},
        {0x1034u, 0x5003u}, {0x1038u, 0x5004u},
        {0x103cu, 0x5005u}, {0xa000u, 1u}, {0xa004u, 7u},
        {0x8000u, 34u}, {0x8004u, 12u}, {0x801cu, 0x8100u},
        {0x9138u, 0x8000u}, {0xe138u, 7u},
        {0x7054u, 0u}, {0x7058u, 0u},
        {0x54c3u, 3u}, {0x54b7u, 0x6000u},
        {0x90dec0u, 5u}, {0x7070u, 0u},
        {0x1048u, 0x1000u}, {0xf010u, 1u}};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> removed;
    std::vector<std::uint32_t> inserted;
    std::vector<std::string> window_names;
    std::vector<Subscription> subscriptions;
    std::vector<std::uint32_t> released_connections;
    std::unordered_map<std::uint32_t, std::uint32_t> columns{
        {0x5003u, 2u}, {0x5004u, 1u}};
    std::unordered_map<std::uint32_t, std::uint32_t> rows{
        {0x5003u, 0u}, {0x5004u, 0u}};
    std::unordered_map<std::uint32_t, std::string> texts{
        {0x8100u, "description"}};
    std::vector<pal4::HelpTopicEntry> topics;
    std::vector<pal4::HelpContentEntry> contents;
    std::vector<std::u32string> item_titles;
    std::vector<bool> item_unread;
    std::vector<std::uint32_t> item_rows;
    std::vector<std::uint32_t> freed_items;
    std::vector<std::uint32_t> scrolled_rows;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::uint32_t allocations = 0u;
    std::uint32_t item_allocations = 0u;
    int null_callback_at = -1;
    int null_connection_at = -1;
    std::array<std::uint32_t, 2> frame_event{};
    std::uint32_t frame_event_name = 0u;
    std::uint32_t topic_hovered = 0u;
    std::uint32_t hovered = 0u;
    std::u32string last_text;
    std::uint32_t read_text_address = 0u;
    std::uint32_t saved_global = 0u;
    std::uint32_t image_item = 0u;
    std::uint32_t redrawn = 0u;
    std::uint32_t restored_row_list = 0u;
    std::int32_t restored_row = -1;
    std::uint32_t restored_scrollbar = 0u;
    float restored_position = 0.0f;
    std::uint32_t invoked_callback = 0u;
    std::uint32_t mouse_window = 0u;
    std::uint32_t moved_list = 0u;
    bool moved_up = false;
    std::uint32_t processed_collection = 0u;
    std::uint32_t processed_key = 0u;
    std::uint32_t processed_handler = 0u;
    std::uint32_t classified_time = 0u;
    std::uint32_t parent = 0u;
    bool parent_visible = false;
    bool fail_lookup = false;
    bool fail_text = false;
    bool content_visible = false;
    bool fail_item_constructor = false;

    void LoadLayout(std::uint32_t object, std::uint32_t name) override {
        calls.emplace_back("layout");
        Check(object == 0x1000u && name == 0x9000u, "layout arguments");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    std::uint32_t DestroyBase(std::uint32_t object) override {
        calls.emplace_back("destroy-base");
        Check(object == 0x1000u, "destroyed base");
        return object;
    }
    std::uint32_t GetWindow(std::string_view name) override {
        calls.emplace_back("get-window");
        if (fail_lookup) throw std::runtime_error("lookup");
        window_names.emplace_back(name);
        if (name == "HelpWindow/EditInfo") return 0x5002u;
        if (name == "HelpWindow/BtnClose") return 0x5006u;
        if (name == "HelpWindow/ImgDisusedMask") return 0x5005u;
        if (name == "HelpWindow/List1") return 0x5003u;
        if (name == "HelpWindow/List2") return 0x5004u;
        throw std::runtime_error("unexpected window");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        if (bytes == 0x214u)
            return 0xc000u + item_allocations++ * 0x300u;
        Check(bytes == 0x18u, "callback allocation size");
        const auto index = allocations++;
        if (static_cast<int>(index) == null_callback_at) return 0u;
        return 0x9000u + index * 0x100u;
    }
    std::uint32_t Subscribe(std::uint32_t window,
                            pal4::HelpWindowEvent event,
                            std::uint32_t callback) override {
        calls.emplace_back("subscribe");
        subscriptions.push_back({window, event, callback});
        if (static_cast<int>(subscriptions.size() - 1u) ==
            null_connection_at) return 0u;
        return 0xb000u + static_cast<std::uint32_t>(subscriptions.size());
    }
    void ReleaseConnection(std::uint32_t connection) override {
        calls.emplace_back("release-connection");
        released_connections.push_back(connection);
    }
    std::uint32_t ColumnCount(std::uint32_t list) override {
        calls.emplace_back("column-count");
        return columns.at(list);
    }
    void RemoveColumn(std::uint32_t list,
                      std::uint32_t index) override {
        calls.emplace_back("remove-column");
        Check(index == 0u && columns.at(list) != 0u, "remove first column");
        --columns[list];
    }
    void SetSortDirection(std::uint32_t list,
                          std::uint32_t direction) override {
        calls.emplace_back("sort-direction");
        Check(columns.at(list) == 0u && direction == 0u, "sort direction");
    }
    void SetUserColumnSizing(std::uint32_t list, bool enabled) override {
        calls.emplace_back("column-sizing");
        Check(columns.contains(list) && !enabled, "disable sizing");
    }
    void SetUserColumnDragging(std::uint32_t list, bool enabled) override {
        calls.emplace_back("column-dragging");
        Check(columns.contains(list) && !enabled, "disable dragging");
    }
    void SetUserSortControl(std::uint32_t list, bool enabled) override {
        calls.emplace_back("sort-control");
        Check(columns.contains(list) && !enabled, "disable user sorting");
    }
    void AddColumn(std::uint32_t list, std::u32string_view name,
                   std::uint32_t id, float width) override {
        calls.emplace_back("add-column");
        Check(name.empty() && id == 0u && width == 1.0f &&
                  columns.at(list) == 0u,
              "single empty full-width column");
        ++columns[list];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.emplace_back("write-byte");
        bytes[address] = value;
    }
    void SetAlwaysOnTop(std::uint32_t window, bool value) override {
        calls.emplace_back(value ? "top-true" : "top-false");
        Check(window == 0x5001u || window == 0x5003u, "top window");
    }
    std::uint32_t TickCount() override {
        calls.emplace_back("tick");
        return 0x12345678u;
    }
    void ClassifyTimeValue(std::uint32_t value) override {
        calls.emplace_back("classify-time");
        classified_time = value;
    }
    void SetParent(std::uint32_t object, bool visible,
                   std::uint32_t value) override {
        calls.emplace_back("set-parent");
        Check(object == 0x1000u, "parent object");
        parent = value;
        parent_visible = visible;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x3000u;
    }
    void RemoveCallback(std::uint32_t collection,
                        std::uint32_t callback) override {
        calls.emplace_back("remove-callback");
        Check(collection == 0x3134u, "remove collection");
        removed.push_back(callback);
    }
    void InsertCallback(std::uint32_t collection,
                        std::uint32_t callback) override {
        calls.emplace_back("insert-callback");
        Check(collection == 0x3134u, "insert collection");
        inserted.push_back(callback);
    }
    void SetVisible(std::uint32_t window, bool value) override {
        calls.emplace_back(value ? "visible-true" : "visible-false");
        Check(window == 0x5005u, "visible window");
    }
    void ResetList(std::uint32_t list) override {
        calls.emplace_back("reset-list");
        Check(list == 0x5003u || list == 0x5004u, "reset list");
        rows[list] = 0u;
    }
    void SetText(std::uint32_t window,
                 std::u32string_view text) override {
        calls.emplace_back("set-text");
        Check(window == 0x5002u, "description window");
        last_text = text;
        if (fail_text) throw std::runtime_error("text");
    }
    void SetModal(std::uint32_t window, bool value) override {
        calls.emplace_back("modal");
        Check(window == 0x5001u && value, "modal window");
    }
    void HandleFrameEvent(std::uint32_t name,
                          const std::array<std::uint32_t, 2>& event) override {
        calls.emplace_back("handle-frame-event");
        frame_event_name = name;
        frame_event = event;
    }
    std::uint32_t FindFrame(std::uint32_t manager,
                            std::uint32_t name) override {
        calls.emplace_back("find-frame");
        Check(manager == 0x3000u && name == 0x8b13acu, "find HelpWindow");
        return 0x1000u;
    }
    std::vector<pal4::HelpTopicEntry> TopicEntries() override {
        calls.emplace_back("topic-entries");
        return topics;
    }
    std::vector<pal4::HelpContentEntry> ContentEntries() override {
        calls.emplace_back("content-entries");
        return contents;
    }
    std::string ReadText(std::uint32_t address) override {
        calls.emplace_back("read-text");
        read_text_address = address;
        return texts.at(address);
    }
    std::string ConvertToUtf8(std::string_view text) override {
        calls.emplace_back("utf8");
        return std::string(text);
    }
    void AddRow(std::uint32_t list, std::uint32_t index) override {
        calls.emplace_back("add-row");
        Check((list == 0x5003u || list == 0x5004u) && index == 0u,
              "help row insertion");
        ++rows[list];
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        calls.emplace_back("row-count");
        return rows.at(list);
    }
    std::uint32_t ConstructListItem(std::uint32_t storage,
                                    std::uint32_t list,
                                    std::u32string_view text,
                                    bool unread) override {
        calls.emplace_back("construct-item");
        Check((list == 0x5003u || list == 0x5004u) &&
                  storage >= 0xc000u,
              "help item constructor arguments");
        if (fail_item_constructor) throw std::runtime_error("item");
        item_titles.emplace_back(text);
        item_unread.push_back(unread);
        return storage;
    }
    void FreeItem(std::uint32_t item) override {
        calls.emplace_back("free-item");
        freed_items.push_back(item);
    }
    void SetItem(std::uint32_t list, std::uint32_t item,
                 std::uint32_t column, std::uint32_t row) override {
        calls.emplace_back("set-item");
        Check((list == 0x5003u || list == 0x5004u) && item >= 0xc000u &&
                  column == 0u && row < rows.at(list),
              "help item placement");
        item_rows.push_back(row);
    }
    void ScrollRowToVisible(std::uint32_t list,
                            std::uint32_t row) override {
        calls.emplace_back("scroll-row");
        Check(list == 0x5003u, "topic scroll list");
        scrolled_rows.push_back(row);
    }
    void HandleMouseEvent(std::uint32_t window) override {
        calls.emplace_back("mouse-event");
        mouse_window = window;
    }
    void MoveHoverItem(std::uint32_t list, bool up) override {
        calls.emplace_back("move-hover");
        moved_list = list;
        moved_up = up;
    }
    std::uint32_t HoverItem(std::uint32_t list) override {
        calls.emplace_back("hover-item");
        Check(list == 0x5003u || list == 0x5004u, "help list");
        return list == 0x5003u ? topic_hovered : hovered;
    }
    std::uint32_t GlobalData() override {
        calls.emplace_back("global-data");
        return 0x7000u;
    }
    void SaveSettings(std::uint32_t global) override {
        calls.emplace_back("save-settings");
        saved_global = global;
    }
    void SetListItemImage(std::uint32_t item,
                          std::uint32_t image) override {
        calls.emplace_back("set-image");
        Check(image == 0u, "read item image");
        image_item = item;
    }
    void RequestRedraw(std::uint32_t window) override {
        calls.emplace_back("redraw");
        redrawn = window;
    }
    float ReadFloat(std::uint32_t address) override {
        calls.emplace_back("read-float");
        Check(address == 0x64c0u, "scroll position offset");
        return 0.5f;
    }
    void SetHoverRow(std::uint32_t list, std::int32_t row) override {
        calls.emplace_back("set-hover-row");
        restored_row_list = list;
        restored_row = row;
    }
    void SetScrollPosition(std::uint32_t scrollbar,
                           float position) override {
        calls.emplace_back("set-scroll");
        restored_scrollbar = scrollbar;
        restored_position = position;
    }
    bool IsWindowVisible(std::uint32_t window,
                         bool recursive) override {
        calls.emplace_back("is-window-visible");
        Check(window == 0x5005u && !recursive, "content visibility");
        return content_visible;
    }
    void InvokeCallback(std::uint32_t callback) override {
        calls.emplace_back("invoke-callback");
        invoked_callback = callback;
    }
    std::uint8_t ProcessUiEvents(std::uint32_t collection,
                                 std::uint32_t key,
                                 std::uint32_t handler) override {
        calls.emplace_back("process-events");
        processed_collection = collection;
        processed_key = key;
        processed_handler = handler;
        return 7u;
    }
};

void CheckConstructor() {
    using Window = pal4::HelpWindow;
    Runtime runtime;
    Check(Window::Construct(0x1000u, 0x9000u, runtime) == 0x1000u &&
              runtime.words.at(0x1000u) == Window::kVtable &&
              runtime.words.at(0x1028u) == Window::kSecondaryVtable &&
              runtime.words.at(0x1040u) == 0u &&
              runtime.words.at(0x1044u) == 0x843fb0u &&
              runtime.words.at(0x104cu) == 0x843f98u &&
              runtime.words.at(0x1048u) == 0x1000u &&
              runtime.words.at(0x1050u) == 0x1000u &&
              runtime.classified_time == 0x12345678u,
          "constructor fields, callback objects and time calls");
    Check(runtime.calls.at(0) == "layout" &&
              runtime.calls.at(7) == "get-window" &&
              runtime.calls.at(runtime.calls.size() - 6) == "read" &&
              runtime.calls.at(runtime.calls.size() - 5) == "top-true" &&
              runtime.calls.at(runtime.calls.size() - 4) == "tick" &&
              runtime.calls.at(runtime.calls.size() - 3) == "classify-time" &&
              runtime.subscriptions.size() == 5u,
          "constructor sequence");
    Runtime failure;
    failure.fail_lookup = true;
    bool caught = false;
    try { (void)Window::Construct(0x1000u, 0x9000u, failure); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && failure.calls.back() == "destroy-base",
          "control lookup failure destroys base dialog");
}

void CheckControls() {
    using Window = pal4::HelpWindow;
    Runtime runtime;
    Window::InitializeControls(0x1000u, runtime);
    Check(runtime.window_names == (std::vector<std::string>{
              "HelpWindow/EditInfo", "HelpWindow/BtnClose",
              "HelpWindow/ImgDisusedMask", "HelpWindow/List1",
              "HelpWindow/List2"}) &&
              runtime.words.at(0x102cu) == 0x5002u &&
              runtime.words.at(0x1030u) == 0x5006u &&
              runtime.words.at(0x103cu) == 0x5005u &&
              runtime.words.at(0x1034u) == 0x5003u &&
              runtime.words.at(0x1038u) == 0x5004u,
          "five control names and object offsets");
    const std::array<std::uint32_t, 5> callbacks{
        0x4b6890u, 0x4b7ff0u, 0x4a7820u, 0x4a7880u, 0x4a79a0u};
    const std::array<std::uint32_t, 5> windows{
        0x5006u, 0x5003u, 0x5003u, 0x5004u, 0x5001u};
    const std::array<pal4::HelpWindowEvent, 5> events{
        pal4::HelpWindowEvent::Clicked,
        pal4::HelpWindowEvent::HoverRowChanged,
        pal4::HelpWindowEvent::MouseClick,
        pal4::HelpWindowEvent::HoverRowChanged,
        pal4::HelpWindowEvent::KeyDown};
    Check(runtime.subscriptions.size() == 5u &&
              runtime.released_connections.size() == 5u,
          "five subscribed callbacks and released connections");
    for (std::uint32_t i = 0; i < 5u; ++i) {
        const auto address = 0x9000u + i * 0x100u;
        Check(runtime.words.at(address) == Window::kCallbackVtable &&
                  runtime.words.at(address + 8u) == callbacks[i] &&
                  runtime.words.at(address + 0xcu) == 0u &&
                  runtime.words.at(address + 0x10u) == 0x1000u &&
                  runtime.subscriptions[i].window == windows[i] &&
                  runtime.subscriptions[i].event == events[i] &&
                  runtime.subscriptions[i].callback == address &&
                  runtime.released_connections[i] == 0xb001u + i,
              "callback record, event and connection order");
    }
    Check(runtime.columns.at(0x5003u) == 1u &&
              runtime.columns.at(0x5004u) == 1u &&
              runtime.bytes.at(0x54dau) == 0u &&
              runtime.bytes.at(0x54dbu) == 0u &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "remove-column") == 3 &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "add-column") == 2,
          "both lists reset to one column and internal flag cleared");

    Runtime null_values;
    null_values.null_callback_at = 0;
    null_values.null_connection_at = 1;
    Window::InitializeControls(0x1000u, null_values);
    Check(null_values.subscriptions.at(0).callback == 0u &&
              !null_values.words.contains(0x9000u) &&
              null_values.released_connections.size() == 4u &&
              null_values.released_connections.at(0) == 0xb001u &&
              null_values.released_connections.at(1) == 0xb003u,
          "null callback still subscribes and null connection is not released");
}

void CheckVisibility() {
    using Window = pal4::HelpWindow;
    Runtime runtime;
    Window::SetVisible(0x1000u, false, 0x6000u, runtime);
    Check(runtime.parent == 0x6000u && !runtime.parent_visible &&
              runtime.removed == (std::vector<std::uint32_t>{0x1028u}) &&
              runtime.inserted.empty() &&
              runtime.calls == (std::vector<std::string>{
                  "set-parent", "manager", "remove-callback", "read",
                  "top-true", "read", "visible-false", "read",
                  "reset-list", "read", "set-text", "read", "modal"}),
          "hide resets list and removes callback before modal update");
    runtime.calls.clear();
    Window::SetVisible(0x1000u, true, 0x6000u, runtime);
    Check(runtime.parent_visible && runtime.inserted ==
              (std::vector<std::uint32_t>{0x1028u}) &&
              runtime.calls.at(0) == "set-parent" &&
              runtime.calls.at(runtime.calls.size() - 2) == "manager" &&
              runtime.calls.back() == "insert-callback",
          "show inserts callback after UI reset");
    Runtime failure;
    failure.fail_text = true;
    bool caught = false;
    try { Window::SetVisible(0x1000u, true, 0x6000u, failure); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && failure.inserted.empty() &&
              failure.calls.back() == "set-text",
          "text failure prevents callback insertion");
}

void CheckEvents() {
    using Window = pal4::HelpWindow;
    Runtime runtime;
    Check(Window::HandleShow(0xa000u, runtime) == 1u &&
              runtime.frame_event_name == Window::kNamePointer &&
              runtime.frame_event ==
                  (std::array<std::uint32_t, 2>{1u, 0u}) &&
              runtime.words.at(0x1040u) == 7u &&
              runtime.calls == (std::vector<std::string>{
                  "read", "handle-frame-event", "manager", "find-frame",
                  "read", "write", "read", "reset-list",
                  "topic-entries", "row-count"}),
          "show event opens frame and selects topic");
    runtime.calls.clear();
    runtime.words[0xa000u] = 0u;
    Check(Window::HandleShow(0xa000u, runtime) == 1u &&
              runtime.frame_event ==
                  (std::array<std::uint32_t, 2>{0u, 0u}) &&
              runtime.calls == (std::vector<std::string>{
                  "read", "handle-frame-event"}),
          "hide event only closes frame");
    runtime.calls.clear();
    Check(Window::OnShow(0x1000u, runtime) == 1u &&
              runtime.rows.at(0x5004u) == 0u &&
              runtime.mouse_window == 0x5005u &&
              runtime.inserted ==
                  (std::vector<std::uint32_t>{0x1044u}) &&
              runtime.calls == (std::vector<std::string>{
                  "read", "reset-list", "global-data", "read",
                  "hover-item",
                  "content-entries", "row-count", "read", "top-false",
                  "read", "visible-true", "read", "mouse-event",
                  "manager", "insert-callback"}),
          "selection opens content pane and inserts close callback");
    runtime.calls.clear();
    Check(Window::RefreshContentList(0x1000u, runtime) == 1u &&
              runtime.calls == (std::vector<std::string>{
                  "read", "reset-list", "global-data", "read",
                  "hover-item",
                  "content-entries", "row-count"}),
          "content refresh forwarding");
    runtime.calls.clear();
    Check(Window::OnListChanged(0x1000u, runtime) == 1u &&
              runtime.calls == (std::vector<std::string>{
                  "read", "hover-item", "read", "hover-item"}),
          "list-change forwarding");
}

void CheckItemClick() {
    using Window = pal4::HelpWindow;
    Runtime runtime;
    Check(Window::OnItemClick(0x1000u, runtime) == 0u &&
              runtime.calls == (std::vector<std::string>{
                  "read", "hover-item", "read", "hover-item"}),
          "no hovered item returns zero");

    runtime.calls.clear();
    runtime.hovered = 0x9000u;
    Check(Window::OnItemClick(0x1000u, runtime) == 1u &&
              runtime.read_text_address == 0x8100u &&
              runtime.last_text == U"description" &&
              runtime.words.at(0x7058u) == 4u &&
              runtime.saved_global == 0x7000u &&
              runtime.image_item == 0x9000u &&
              runtime.redrawn == 0x5004u &&
              runtime.words.at(0x1040u) == 0u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "topic-entries") != runtime.calls.end() &&
              runtime.restored_row_list == 0x5003u &&
              runtime.restored_row == 3 &&
              runtime.restored_scrollbar == 0x6000u &&
              runtime.restored_position == 0.5f &&
              runtime.calls.at(0) == "read" &&
              runtime.calls.at(1) == "hover-item" &&
              runtime.calls.back() == "set-scroll",
          "first read saves bit and restores topic scroll state");

    runtime.calls.clear();
    runtime.saved_global = 0u;
    runtime.image_item = 0u;
    Check(Window::OnItemClick(0x1000u, runtime) == 1u &&
              runtime.saved_global == 0u &&
              runtime.image_item == 0u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "topic-entries") == runtime.calls.end(),
          "already read item has no save or list rebuild");

    runtime.calls.clear();
    runtime.words[0x7058u] = 0u;
    runtime.contents = {{0xd000u, 35u, 12u, "more", 0}};
    Check(Window::OnItemClick(0x1000u, runtime) == 1u &&
              runtime.saved_global == 0x7000u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "topic-entries") == runtime.calls.end(),
          "remaining unread help skips topic rebuild");

    runtime.calls.clear();
    runtime.words[0x8000u] = 256u;
    runtime.saved_global = 0u;
    Check(Window::OnItemClick(0x1000u, runtime) == 1u &&
              runtime.saved_global == 0u &&
              runtime.words.at(0x7058u) == 4u,
          "out of bitset range skips mark");
}

void CheckTopicPopulation() {
    using Window = pal4::HelpWindow;
    Runtime runtime;
    runtime.topics = {{1u, "one"}, {2u, "skip"},
                      {3u, "\xe4\xb8\xad"}};
    runtime.contents = {{0xd000u, 1u, 1u, "a", 0},
                        {0xd02cu, 2u, 1u, "b", 5},
                        {0xd058u, 3u, 2u, "locked", 6},
                        {0xd084u, 4u, 3u, "c", 0}};
    Window::PopulateTopicList(0x1000u, 3u, runtime);
    Check(runtime.words.at(0x1040u) == 3u &&
              runtime.rows.at(0x5003u) == 2u &&
              runtime.item_titles ==
                  (std::vector<std::u32string>{U"one", U"\u4e2d"}) &&
              runtime.item_rows ==
                  (std::vector<std::uint32_t>{0u, 1u}) &&
              runtime.words.at(0xc138u) == 1u &&
              runtime.words.at(0xc438u) == 3u &&
              runtime.restored_row == 1 &&
              runtime.scrolled_rows ==
                  (std::vector<std::uint32_t>{1u}) &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "content-entries") == 5,
          "topic filtering, UTF-8 titles, row placement and selected scroll");

    Runtime empty;
    Window::PopulateTopicList(0x1000u, 9u, empty);
    Check(empty.words.at(0x1040u) == 9u &&
              empty.rows.at(0x5003u) == 0u &&
              empty.scrolled_rows.empty(),
          "empty category collection resets without selection");

    Runtime failure;
    failure.topics = {{1u, "one"}};
    failure.contents = {{0xd000u, 1u, 1u, "a", 0}};
    failure.fail_item_constructor = true;
    bool caught = false;
    try { Window::PopulateTopicList(0x1000u, 1u, failure); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && failure.freed_items ==
              (std::vector<std::uint32_t>{0xc000u}) &&
              failure.calls.back() == "free-item",
          "item construction failure frees storage");
}

void CheckContentPopulation() {
    using Window = pal4::HelpWindow;
    Runtime runtime;
    runtime.topic_hovered = 0xe000u;
    runtime.contents = {
        {0xd000u, 34u, 7u, "one", 5},
        {0xd02cu, 35u, 7u, "locked", 6},
        {0xd058u, 36u, 8u, "other", 0},
        {0xd084u, 255u, 7u, "\xe4\xb8\xad", 4},
        {0xd0b0u, 256u, 7u, "oversize", 0},
    };
    Check(Window::PopulateContentList(0x1000u, runtime) == 0u &&
              runtime.rows.at(0x5004u) == 2u &&
              runtime.item_titles ==
                  (std::vector<std::u32string>{U"one", U"\u4e2d"}) &&
              runtime.item_unread == (std::vector<bool>{true, true}) &&
              runtime.words.at(0xc138u) == 0xd000u &&
              runtime.words.at(0xc438u) == 0xd084u &&
              runtime.item_rows ==
                  (std::vector<std::uint32_t>{0u, 1u}) &&
              runtime.restored_row_list == 0x5004u &&
              runtime.restored_row == 0,
          "topic, progress and bitset gates preserve content order");

    Runtime already_read;
    already_read.topic_hovered = 0xe000u;
    already_read.contents = {{0xd000u, 34u, 7u, "seen", 0}};
    already_read.words[0x7058u] = 4u;
    (void)Window::PopulateContentList(0x1000u, already_read);
    Check(already_read.item_unread == (std::vector<bool>{false}),
          "seen bit clears unread image");

    Runtime empty;
    empty.topic_hovered = 0xe000u;
    empty.contents = {{0xd000u, 34u, 8u, "other", 0}};
    (void)Window::PopulateContentList(0x1000u, empty);
    Check(empty.rows.at(0x5004u) == 0u &&
              empty.item_titles.empty() &&
              empty.restored_row == -1,
          "no matching topic leaves list empty");

    Runtime failure;
    failure.topic_hovered = 0xe000u;
    failure.contents = {{0xd000u, 34u, 7u, "one", 0}};
    failure.fail_item_constructor = true;
    bool caught = false;
    try { (void)Window::PopulateContentList(0x1000u, failure); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && failure.freed_items ==
              (std::vector<std::uint32_t>{0xc000u}) &&
              failure.calls.back() == "free-item",
          "content item constructor failure releases storage");
}

void CheckSelectionAndDescription() {
    using Window = pal4::HelpWindow;
    Runtime runtime;
    Check(Window::SelectedTopicId(0x1000u, runtime) == 0xffffffffu &&
              Window::SelectedContentData(0x1000u, runtime) == 0u,
          "missing hover items have distinct sentinels");
    runtime.topic_hovered = 0xe000u;
    runtime.hovered = 0x9000u;
    Check(Window::SelectedTopicId(0x1000u, runtime) == 7u &&
              Window::SelectedContentData(0x1000u, runtime) == 0x8000u,
          "both selectors read hovered item data at +0x138");

    runtime.calls.clear();
    Window::SetDescription(0x1000u, 0u, runtime);
    Check(runtime.calls.empty(), "null content leaves description unchanged");
    runtime.words[0x801cu] = 0x8100u;
    runtime.texts[0x8100u] = "\xe4\xb8\xad";
    Window::SetDescription(0x1000u, 0x8000u, runtime);
    Check(runtime.read_text_address == 0x8100u &&
              runtime.last_text == U"\u4e2d",
          "description reads record +0x1c and decodes UTF-8");
    runtime.words[0x801cu] = 0u;
    runtime.calls.clear();
    Window::SetDescription(0x1000u, 0x8000u, runtime);
    Check(runtime.last_text.empty() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "read-text") == runtime.calls.end(),
          "null text pointer writes empty description");
}

void CheckHelpDataCount() {
    using Window = pal4::HelpWindow;
    Runtime runtime;
    runtime.contents = {{0xd000u, 1u, 7u, "a", -2},
                        {0xd02cu, 256u, 7u, "b", 5},
                        {0xd058u, 2u, 7u, "c", 6},
                        {0xd084u, 3u, 8u, "d", 0}};
    Check(Window::HelpDataCount(7u, runtime) == 2u &&
              Window::HelpDataCount(8u, runtime) == 1u &&
              Window::HelpDataCount(9u, runtime) == 0u,
          "count filters topic and signed progress, not content ID");
    runtime.words[0x90dec0u] = static_cast<std::uint32_t>(-3);
    Check(Window::HelpDataCount(7u, runtime) == 0u,
          "negative progress retains signed comparison");
}

void CheckUnreadHelp() {
    using Window = pal4::HelpWindow;
    Runtime runtime;
    runtime.contents = {{0xd000u, 34u, 7u, "seen", 0},
                        {0xd02cu, 35u, 7u, "locked", 6},
                        {0xd058u, 256u, 7u, "large", 0},
                        {0xd084u, 255u, 8u, "other", 0}};
    runtime.words[0x7058u] = 4u;
    Check(!Window::HasUnreadHelp(7u, runtime),
          "seen, locked and out-of-range records do not count unread");
    runtime.words[0x90dec0u] = 6u;
    Check(Window::HasUnreadHelp(7u, runtime),
          "newly unlocked unseen record is unread");
    runtime.words[0x7058u] = 12u;
    Check(!Window::HasUnreadHelp(7u, runtime),
          "seen bit removes last unread record");
    Check(Window::HasUnreadHelp(8u, runtime) &&
              !Window::HasUnreadHelp(9u, runtime),
          "topic filter and empty collection");
}

void CheckKeyboard() {
    using Window = pal4::HelpWindow;
    Runtime runtime;
    runtime.rows[0x5004u] = 2u;
    runtime.last_text = U"old";
    Check(Window::OnKeyDown(0x1044u, 0xf000u, runtime) == 1u &&
              runtime.removed == (std::vector<std::uint32_t>{0x1044u}) &&
              runtime.inserted == (std::vector<std::uint32_t>{0x1028u}) &&
              runtime.rows.at(0x5004u) == 0u &&
              runtime.last_text.empty() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "top-true") != runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "visible-false") != runtime.calls.end(),
          "Escape leaves content page and restores main callback");

    runtime.calls.clear();
    runtime.words[0xf010u] = 200u;
    Check(Window::OnKeyDown(0x1044u, 0xf000u, runtime) == 1u &&
              runtime.moved_list == 0x5004u && runtime.moved_up &&
              runtime.calls.back() == "move-hover",
          "up key moves content hover upward");
    runtime.calls.clear();
    runtime.words[0xf010u] = 208u;
    Check(Window::OnKeyDown(0x1044u, 0xf000u, runtime) == 1u &&
              runtime.moved_list == 0x5004u && !runtime.moved_up,
          "down key moves content hover downward");
    runtime.calls.clear();
    runtime.words[0xf010u] = 28u;
    Check(Window::OnKeyDown(0x1044u, 0xf000u, runtime) == 1u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "hover-item") != runtime.calls.end(),
          "Enter forwards to content click even when nothing is hovered");
    runtime.calls.clear();
    runtime.words[0xf010u] = 99u;
    Check(Window::OnKeyDown(0x1044u, 0xf000u, runtime) == 0u &&
              runtime.calls == (std::vector<std::string>{"read"}),
          "unknown key does not touch the callback object");
}

void CheckHelpHelpers() {
    using Window = pal4::HelpWindow;
    Runtime runtime;
    runtime.words[0xf104u] = 0u;
    Check(Window::HelpDataSize(0xf100u, runtime) == 0 &&
              runtime.calls == (std::vector<std::string>{"read"}),
          "null data start yields empty collection without reading end");
    runtime.words[0xf104u] = 0x2000u;
    runtime.words[0xf108u] = 0x2058u;
    Check(Window::HelpDataSize(0xf100u, runtime) == 2,
          "44-byte record span yields two entries");
    runtime.words[0xf108u] = 0x1fd4u;
    Check(Window::HelpDataSize(0xf100u, runtime) == -1,
          "signed malformed span retains native division");
    Check(Window::ProcessEvents(0xe000u, runtime) == 7u &&
              runtime.processed_collection == 0x3104u &&
              runtime.processed_key == 0xe008u &&
              runtime.processed_handler == 0xe000u,
          "event wrapper forwards manager collection and handler offsets");

    bool caught = false;
    try { pal4::BitsetBoundsError::ThrowInvalidPosition(); }
    catch (const std::out_of_range& error) {
        caught = std::string_view(error.what()) ==
            "invalid bitset<N> position";
    }
    Check(caught, "help bitset throw reuses exact out-of-range path");
}

void CheckClose() {
    using Window = pal4::HelpWindow;
    Runtime runtime;
    Check(Window::OnClose(0x1000u, runtime) == 1u &&
              runtime.invoked_callback == 0x1028u &&
              runtime.calls == (std::vector<std::string>{
                  "read", "is-window-visible", "invoke-callback"}),
          "close main help view");
    runtime.calls.clear();
    runtime.content_visible = true;
    Check(Window::OnClose(0x1000u, runtime) == 1u &&
              runtime.invoked_callback == 0x1044u,
          "close content pane");
}

}  // namespace

int main() {
    try {
        CheckConstructor();
        CheckControls();
        CheckVisibility();
        CheckEvents();
        CheckItemClick();
        CheckTopicPopulation();
        CheckContentPopulation();
        CheckSelectionAndDescription();
        CheckHelpDataCount();
        CheckUnreadHelp();
        CheckKeyboard();
        CheckHelpHelpers();
        CheckClose();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
