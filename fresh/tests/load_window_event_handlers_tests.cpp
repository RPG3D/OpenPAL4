#include "pal4/load_window_event_handlers.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::LoadWindowEventRuntimeStub {
    std::uint32_t save_reads{};
    std::uint32_t hovered_list{};
    std::uint32_t item = 0x1100u;
    std::uint32_t item_word = 1u;
    std::uint32_t allocated = 0x3300u;
    std::uint32_t text = 0x4400u;
    std::uint32_t released{};
    std::uint32_t release_flag{};
    bool throw_in_dialog{};
    bool replace_callback{};
    std::vector<std::string> calls;

    std::uint32_t EventWindow(std::uint32_t member) override {
        Check(member == 0x7008u,
              "page event reads clicked window at +8");
        calls.emplace_back("event-window");
        return 0x2200u;
    }
    std::uint32_t WindowUserId(std::uint32_t window) override {
        Check(window == 0x2200u,
              "page index comes from clicked window +0x118");
        calls.emplace_back("window-id");
        return 2u;
    }
    bool LoadSaveFile(std::int32_t slot, std::uint32_t mode,
                      pal4::SaveSlotData&,
                      pal4::SaveSlotExtras& extras) override {
        Check(slot == 20 + static_cast<std::int32_t>(save_reads) &&
                  mode == 1u && extras.flags == 0u,
              "page click populates ten slots without refresh");
        ++save_reads;
        calls.emplace_back("load");
        return false;
    }
    std::uint32_t HoverItem(std::uint32_t list) override {
        hovered_list = list;
        calls.emplace_back("hover-item");
        return item;
    }
    std::uint32_t ItemWord(std::uint32_t candidate,
                           std::uint32_t offset) override {
        Check(candidate == item && offset == 0x1ecu,
              "click checks save item payload at +0x1ec");
        calls.emplace_back("item-word");
        return item_word;
    }
    std::uint32_t AllocateCallback(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "save-load callback allocation is 24 bytes");
        calls.emplace_back("allocate-callback");
        return allocated;
    }
    void InitializeSaveLoadCallback(std::uint32_t callback,
                                     pal4::LoadWindowState&) override {
        Check(callback == allocated,
              "save-load callback receives allocated storage");
        calls.emplace_back("initialize-callback");
    }
    std::uint32_t TriggerText(std::uint32_t id) override {
        Check(id == 0xe5u, "confirmation text uses trigger E5");
        calls.emplace_back("trigger-text");
        return text;
    }
    std::uint32_t EmptyText() override {
        calls.emplace_back("empty-text");
        return 0x4500u;
    }
    std::uint32_t CreateCommonDialog() override {
        calls.emplace_back("create-dialog");
        return 0x5500u;
    }
    void ShowDialog(std::uint32_t dialog, std::uint32_t message,
                    float width, float height, std::uint32_t mode,
                    std::uint32_t& callback) override {
        Check(dialog == 0x5500u &&
                  message == (text == 0u ? 0x4500u : text) &&
                  width == 0.3f && height == 0.4f && mode == 3u &&
                  callback == allocated,
              "dialog receives message, geometry, mode and callback");
        calls.emplace_back("show-dialog");
        if (replace_callback) callback = 0x6600u;
        if (throw_in_dialog) throw std::runtime_error("show-dialog");
    }
    void ReleaseCallback(std::uint32_t callback,
                         std::uint32_t deleting) override {
        released = callback;
        release_flag = deleting;
        calls.emplace_back("release-callback");
    }
};

struct ListRuntime final : pal4::LoadWindowEventRuntimeStub {
    std::uint32_t code{};
    std::uint32_t rows = 10u;
    std::uint32_t item = 1u;
    std::int32_t row = 0;
    std::int32_t page = 4;
    std::uint32_t base_events{};
    std::uint32_t up_moves{};
    std::uint32_t down_moves{};
    std::uint32_t dialogs{};
    std::vector<std::int32_t> slots;

    std::uint32_t EventCode(std::uint32_t member) override {
        Check(member == 0x7010u, "list event code is at +0x10");
        return code;
    }
    void DispatchBaseEvent(pal4::LoadWindowState&) override {
        ++base_events;
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        Check(list == 0x5400u, "list event reads +0x54 list");
        return rows;
    }
    std::uint32_t HoverItem(std::uint32_t list) override {
        Check(list == 0x5400u, "hover item belongs to +0x54 list");
        return item;
    }
    std::int32_t HoverRowIndex(std::uint32_t list) override {
        Check(list == 0x5400u, "hover row belongs to +0x54 list");
        return row;
    }
    void UpHoverItem(std::uint32_t list) override {
        Check(list == 0x5400u, "up targets save list");
        ++up_moves;
    }
    void DownHoverItem(std::uint32_t list) override {
        Check(list == 0x5400u, "down targets save list");
        ++down_moves;
    }
    std::uint32_t WindowUserId(std::uint32_t window) override {
        Check(window == 0x5c00u ||
                  (window >= 0x2000u && window <= 0x2009u),
              "page ID comes from mask or target button");
        return window == 0x5c00u ? static_cast<std::uint32_t>(page)
                                 : window - 0x2000u;
    }
    bool LoadSaveFile(std::int32_t slot, std::uint32_t mode,
                      pal4::SaveSlotData&, pal4::SaveSlotExtras& extras) override {
        Check(mode == 1u && extras.flags == 0u,
              "keyboard page change reads saves without refresh");
        slots.push_back(slot);
        return false;
    }
    void ShowDialog(std::uint32_t, std::uint32_t, float, float,
                    std::uint32_t, std::uint32_t&) override {
        ++dialogs;
    }
    std::uint32_t ItemWord(std::uint32_t, std::uint32_t offset) override {
        Check(offset == 0x1ecu, "activate checks save payload");
        return 1u;
    }
};

void CheckListEvents() {
    pal4::LoadWindowState state{};
    state.Write(0x54u, 0x5400u);
    state.Write(0x5cu, 0x5c00u);
    for (std::uint32_t page = 0; page < 10; ++page)
        state.Write(0x2cu + page * 4u, 0x2000u + page);
    ListRuntime runtime;
    const auto handle = [&](std::uint32_t code) {
        runtime.code = code;
        return pal4::LoadWindowEventHandlers::HandleListEvent(
            state, 0x7000u, runtime);
    };
    runtime.rows = 0u;
    Check(handle(1u) == 1u && runtime.base_events == 1u,
          "base event bypasses empty-list gate");
    Check(handle(200u) == 0u && runtime.up_moves == 0u,
          "empty list rejects navigation");
    runtime.rows = 10u;
    Check(handle(999u) == 0u, "unknown event returns zero");
    runtime.item = 0u;
    Check(handle(200u) == 1u && handle(208u) == 1u &&
              runtime.up_moves == 0u && runtime.down_moves == 0u,
          "navigation consumes event without hover item");
    runtime.item = 1u;
    runtime.row = 0;
    Check(handle(200u) == 1u && runtime.up_moves == 0u,
          "up at first row is bounded");
    runtime.row = 1;
    Check(handle(200u) == 1u && runtime.up_moves == 1u,
          "up moves a selected row");
    runtime.row = 9;
    Check(handle(208u) == 1u && runtime.down_moves == 0u,
          "down at final row is bounded");
    runtime.row = 8;
    Check(handle(208u) == 1u && runtime.down_moves == 1u,
          "down moves a selected row");
    Check(handle(203u) == 1u && runtime.slots.size() == 10u &&
              runtime.slots.front() == 30 && runtime.slots.back() == 39,
          "previous page loads ten slots");
    runtime.slots.clear();
    Check(handle(205u) == 1u && runtime.slots.size() == 10u &&
              runtime.slots.front() == 50 && runtime.slots.back() == 59,
          "next page loads ten slots");
    runtime.slots.clear();
    runtime.page = 0;
    Check(handle(203u) == 1u && runtime.slots.empty(),
          "previous page stops at zero");
    runtime.page = 9;
    Check(handle(205u) == 1u && runtime.slots.empty(),
          "next page stops at nine");
    Check(handle(28u) == 1u && runtime.dialogs == 1u,
          "activation delegates to item confirmation");
}

}  // namespace

int main() {
    CheckListEvents();
    pal4::LoadWindowState state{};
    state.Write(0x2cu + 2u * 4u, 0x2200u);
    state.Write(0x54u, 0x5400u);
    Runtime runtime;
    Check(pal4::LoadWindowEventHandlers::OnPageButton(
              state, 0x7000u, runtime) == 1u &&
              runtime.save_reads == 10u &&
              runtime.calls.front() == "event-window" &&
              runtime.calls[1] == "window-id",
          "page event forwards selected ID and returns one");
    Check(pal4::LoadWindowEventHandlers::OnMouseClick(
              state, runtime) == 1u &&
              runtime.released == 0x3300u && runtime.release_flag == 1u &&
              runtime.calls.back() == "release-callback",
          "mouse event shows confirmation and releases callback");
    Check(pal4::LoadWindowEventHandlers::OnHoverRowChanged(
              state, runtime) == 1u &&
              runtime.hovered_list == 0x5400u &&
              runtime.calls.back() == "hover-item",
          "hover event queries list at +0x54 and returns one");

    Runtime no_item;
    no_item.item = 0u;
    Check(pal4::LoadWindowEventHandlers::OnItemClick(
              state, no_item) == 1u && no_item.calls.size() == 1u,
          "missing hover item returns before reading payload");
    Runtime empty_slot;
    empty_slot.item_word = 0u;
    Check(pal4::LoadWindowEventHandlers::OnItemClick(
              state, empty_slot) == 1u && empty_slot.calls.size() == 2u,
          "empty save slot returns before allocating callback");
    Runtime no_text;
    no_text.text = 0u;
    no_text.allocated = 0u;
    Check(pal4::LoadWindowEventHandlers::OnItemClick(
              state, no_text) == 1u && no_text.released == 0u &&
              no_text.calls[3] == "trigger-text" &&
              no_text.calls[4] == "empty-text",
          "null allocation still shows dialog with empty text fallback");
    Runtime replacement;
    replacement.replace_callback = true;
    Check(pal4::LoadWindowEventHandlers::OnItemClick(
              state, replacement) == 1u &&
              replacement.released == 0x6600u,
          "dialog replacement of callback is released");
    Runtime throwing;
    throwing.throw_in_dialog = true;
    try {
        (void)pal4::LoadWindowEventHandlers::OnItemClick(state, throwing);
        Check(false, "dialog exception propagates");
    } catch (const std::runtime_error&) {
        Check(throwing.released == 0x3300u &&
                  throwing.calls.back() == "release-callback",
              "dialog exception releases callback");
    }
    std::cout << "Load window event handler tests passed\n";
}
