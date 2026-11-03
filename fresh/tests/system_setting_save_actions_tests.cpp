#include "pal4/system_setting_save_actions.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemSettingSaveActionsRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::pair<std::string, std::int32_t>> attributes;
    std::uint32_t hover_item = 0x8000u;
    std::uint32_t saved = 0u;
    std::uint32_t helper_slot = 0u;
    std::int32_t populated_page = 999;
    std::uint32_t created_node = 0u;
    std::uint32_t destroyed_node = 0u;
    std::uint32_t event_count = 0u;
    std::uint32_t initialized_game = 0u;
    bool fail_attribute = false;
    bool null_callback = false;
    bool fail_dialog = false;
    std::uint32_t trigger_id = 0u;
    std::uint32_t dialog_callback = 0u;
    std::uint32_t released_callback = 0u;
    std::uint32_t constructed_function = 0u;
    std::uint32_t rows = 10u;
    std::uint32_t up_count = 0u;
    std::uint32_t down_count = 0u;
    std::uint32_t removed_member = 0u;
    std::uint32_t shown_frame = 0u;

    Runtime() {
        words[0x4000u + 104u] = 0x5000u;
        words[0x4040u + 4u] = 0x4000u;
        words[0x5000u + 1216u] = 4u;
        words[0x4000u + 176u] = 0xe000u;
        words[0xe000u + 280u] = 4u;
        words[0x5000u + 1204u] = 0x6000u;
        words[0x6000u + 1216u] = 0x3f400000u;
        words[0x8000u + 440u] = 27u;
        words[0x8000u + 492u] = 1u;
        words[0x12000u + 8u] = 0x13000u;
        words[0x13000u + 280u] = 4u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t HoverItem(std::uint32_t list) override {
        calls.emplace_back("hover");
        Check(list == 0x5000u, "hover list");
        return hover_item;
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        calls.emplace_back("rows");
        Check(list == 0x5000u, "save list row count");
        return rows;
    }
    void UpHoverItem(std::uint32_t list) override {
        calls.emplace_back("up");
        Check(list == 0x5000u, "up hover list");
        ++up_count;
    }
    void DownHoverItem(std::uint32_t list) override {
        calls.emplace_back("down");
        Check(list == 0x5000u, "down hover list");
        ++down_count;
    }
    std::uint32_t GlobalData() override {
        calls.emplace_back("global");
        return 0x7000u;
    }
    void SaveSettings(std::uint32_t data) override {
        calls.emplace_back("save");
        Check(data == 0x7000u && words.at(0x702cu) == 27u &&
              words.at(0x7030u) == 0x3f400000u,
              "selection saved before settings write");
        ++saved;
    }
    void InvokeSlotHelper(std::int32_t slot) override {
        calls.emplace_back("helper");
        helper_slot = static_cast<std::uint32_t>(slot);
    }
    void PopulateSaveList(std::uint32_t frame, std::int32_t page,
                          std::uint32_t mode) override {
        calls.emplace_back("populate");
        Check(frame == 0x4000u && (mode == 0u || mode == 1u),
              "populate args");
        populated_page = page;
    }
    std::uint32_t CreateXmlNode() override {
        calls.emplace_back("create-xml");
        ++created_node;
        return 0x9000u;
    }
    void SetIntAttribute(std::uint32_t node, std::string_view name,
                         std::int32_t value) override {
        calls.emplace_back("attribute");
        Check(node == 0x9000u, "XML node identity");
        if (fail_attribute) throw std::runtime_error("attribute");
        attributes.emplace_back(name, value);
    }
    void DestroyXmlNode(std::uint32_t node) override {
        calls.emplace_back("destroy-xml");
        Check(node == 0x9000u, "XML destroy identity");
        ++destroyed_node;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0xa000u;
    }
    void RemoveActive(std::uint32_t collection,
                      std::uint32_t member) override {
        calls.emplace_back("remove-active");
        Check(collection == 0xa000u + 308u,
              "active save-list collection");
        removed_member = member;
    }
    void ShowSystemFrame(std::uint32_t frame) override {
        calls.emplace_back("show-system");
        shown_frame = frame;
    }
    void ProcessUiEvent(std::uint32_t dispatcher,
                        std::uint32_t event,
                        std::uint32_t first,
                        std::uint32_t second) override {
        calls.emplace_back("ui-event");
        Check(dispatcher == 0xa000u + 260u && event == 0x8a80b4u &&
              first == 0u && second == 0u, "load UI event");
        ++event_count;
    }
    std::uint32_t Game() override {
        calls.emplace_back("game");
        return 0xb000u;
    }
    void InitializeGameState(std::uint32_t game,
                             std::uint32_t state,
                             std::uint32_t argument) override {
        calls.emplace_back("initialize-game");
        Check(game == 0xb000u && state == 6u && argument == 0x9000u,
              "archive load state and XML argument");
        ++initialized_game;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        Check(bytes == 0x18u, "context callback size");
        return null_callback ? 0u : 0xc000u;
    }
    std::uint32_t ConstructCallback(std::uint32_t allocation,
                                    std::uint32_t function,
                                    std::uint32_t frame) override {
        calls.emplace_back("construct-callback");
        Check(allocation == 0xc000u && frame == 0x4000u,
              "context callback constructor arguments");
        constructed_function = function;
        return allocation;
    }
    std::uint32_t TriggerText(std::uint32_t id) override {
        calls.emplace_back("trigger");
        trigger_id = id;
        return 0xd000u;
    }
    void CreateCommonDialog(std::uint32_t manager) override {
        calls.emplace_back("create-dialog");
        Check(manager == 0xa000u, "context dialog manager");
    }
    std::uint32_t ShowDialog(std::uint32_t text,
                             std::uint32_t x_bits,
                             std::uint32_t y_bits,
                             std::uint32_t mode,
                             std::uint32_t callback) override {
        calls.emplace_back("show-dialog");
        Check(text == 0xd000u && x_bits == 1050253722u &&
              y_bits == 1053609165u && mode == 3u,
              "context dialog arguments");
        dialog_callback = callback;
        if (fail_dialog) throw std::runtime_error("dialog");
        return 0x77u;
    }
    std::uint32_t ReleaseCallback(std::uint32_t callback) override {
        calls.emplace_back("release");
        released_callback = callback;
        return 0x88u;
    }
};

}  // namespace

int main() {
    Runtime click;
    Check(pal4::SystemSettingSaveActions::OnClick(
          0x4000u, 1u, click) == 1u &&
          click.saved == 1u && click.helper_slot == 27u &&
          click.populated_page == 2 && click.created_node == 0u,
          "click persists slot and refreshes page");
    Check(click.calls == std::vector<std::string>{
          "read", "hover", "read", "global", "write", "read",
          "read", "read", "global", "write", "global", "save",
          "helper", "populate"}, "click call order");

    Runtime ignored;
    Check(pal4::SystemSettingSaveActions::OnClick(
          0x4000u, 2u, ignored) == 1u && ignored.calls.empty(),
          "non-primary event ignored");
    ignored.hover_item = 0u;
    Check(pal4::SystemSettingSaveActions::OnClick(
          0x4000u, 1u, ignored) == 1u && ignored.saved == 0u,
          "missing hover item ignored");

    Runtime dbl;
    Check(pal4::SystemSettingSaveActions::OnDoubleClick(
          0x4000u, 1u, dbl) == 1u &&
          dbl.saved == 1u && dbl.created_node == 1u &&
          dbl.destroyed_node == 1u && dbl.event_count == 1u &&
          dbl.initialized_game == 1u &&
          dbl.attributes ==
              std::vector<std::pair<std::string, std::int32_t>>{
                  {"PALGAMEIV_LOADING_TYPE", 1},
                  {"PALGAMEIV_LOADING_ARCHIVE", 27}},
          "double click loads archive with two XML attributes");
    Check(dbl.calls[dbl.calls.size() - 7u] == "attribute" &&
          dbl.calls[dbl.calls.size() - 6u] == "attribute" &&
          dbl.calls[dbl.calls.size() - 5u] == "manager" &&
          dbl.calls[dbl.calls.size() - 4u] == "ui-event" &&
          dbl.calls[dbl.calls.size() - 3u] == "game" &&
          dbl.calls[dbl.calls.size() - 2u] == "initialize-game" &&
          dbl.calls.back() == "destroy-xml",
          "double click dispatch and cleanup order");

    Runtime empty_slot;
    empty_slot.words[0x8000u + 492u] = 0u;
    Check(pal4::SystemSettingSaveActions::OnDoubleClick(
          0x4000u, 1u, empty_slot) == 1u &&
          empty_slot.saved == 0u && empty_slot.created_node == 0u,
          "unpopulated archive ignored");

    Runtime fail;
    fail.fail_attribute = true;
    bool caught = false;
    try {
        (void)pal4::SystemSettingSaveActions::OnDoubleClick(
            0x4000u, 1u, fail);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && fail.created_node == 1u && fail.destroyed_node == 1u,
          "XML node released on attribute exception");

    Runtime page;
    Check(pal4::SystemSettingSaveActions::OnChangePage(
          0x4000u, 0x12000u, page) == 1u &&
          page.populated_page == 4 &&
          page.calls == std::vector<std::string>{
              "read", "read", "populate"},
          "page callback reads source index and uses mode zero");

    Runtime save_menu;
    save_menu.words[0x4000u + 504u] = 1u;
    Check(pal4::SystemSettingSaveActions::ShowContextMenu(
          0x4000u, save_menu) == 0x88u &&
          save_menu.trigger_id == 109u &&
          save_menu.constructed_function == 0x4816b0u &&
          save_menu.dialog_callback == 0xc000u &&
          save_menu.released_callback == 0xc000u,
          "populated save mode dialog");
    save_menu.words[0x8000u + 492u] = 0u;
    Check(pal4::SystemSettingSaveActions::OnContextMenuEvent(
          0x4000u, 0xabcdefu, save_menu) == 1u &&
          save_menu.trigger_id == 231u,
          "empty save mode dialog wrapper");

    Runtime load_menu;
    load_menu.words[0x4000u + 504u] = 2u;
    Check(pal4::SystemSettingSaveActions::ShowContextMenu(
          0x4000u, load_menu) == 0x88u &&
          load_menu.trigger_id == 229u &&
          load_menu.constructed_function == 0u &&
          load_menu.words.at(0xc000u) == 0x843c08u &&
          load_menu.words.at(0xc008u) == 0x481740u &&
          load_menu.words.at(0xc00cu) == 0u &&
          load_menu.words.at(0xc010u) == 0x4000u,
          "load mode dialog callback layout");

    Runtime no_dialog;
    no_dialog.words[0x4000u + 504u] = 2u;
    no_dialog.words[0x8000u + 492u] = 0u;
    Check(pal4::SystemSettingSaveActions::ShowContextMenu(
          0x4000u, no_dialog) == 0x8000u &&
          no_dialog.trigger_id == 0u,
          "empty load mode returns hovered item");
    no_dialog.hover_item = 0u;
    Check(pal4::SystemSettingSaveActions::ShowContextMenu(
          0x4000u, no_dialog) == 0u,
          "no hover item returns null");

    Runtime null_menu;
    null_menu.words[0x4000u + 504u] = 1u;
    null_menu.null_callback = true;
    Check(pal4::SystemSettingSaveActions::ShowContextMenu(
          0x4000u, null_menu) == 0x77u &&
          null_menu.dialog_callback == 0u &&
          null_menu.released_callback == 0u,
          "null callback returns dialog result");

    Runtime failed_dialog;
    failed_dialog.words[0x4000u + 504u] = 2u;
    failed_dialog.fail_dialog = true;
    caught = false;
    try {
        (void)pal4::SystemSettingSaveActions::ShowContextMenu(
            0x4000u, failed_dialog);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failed_dialog.released_callback == 0xc000u,
          "context callback released on dialog failure");

    Runtime hover;
    Check(pal4::SystemSettingSaveActions::OnHover(
          0x4000u, 0xdeadu, hover) == 1u &&
          hover.calls == std::vector<std::string>{"read", "hover"},
          "hover callback only queries current list item");

    Runtime back;
    back.words[0x12000u + 16u] = 1u;
    Check(pal4::SystemSettingSaveActions::OnKey(
          0x4040u, 0x12000u, back) == 1u &&
          back.removed_member == 0x4040u &&
          back.shown_frame == 0x4000u &&
          back.calls == std::vector<std::string>{
              "read", "manager", "remove-active", "read", "show-system"},
          "back key exits before querying list rows");

    Runtime empty_list;
    empty_list.words[0x12000u + 16u] = 200u;
    empty_list.rows = 0u;
    Check(pal4::SystemSettingSaveActions::OnKey(
          0x4040u, 0x12000u, empty_list) == 0u &&
          empty_list.up_count == 0u,
          "empty list rejects non-back keys");

    Runtime up;
    up.words[0x12000u + 16u] = 200u;
    Check(pal4::SystemSettingSaveActions::OnKey(
          0x4040u, 0x12000u, up) == 1u && up.up_count == 1u,
          "up key advances hover while index positive");
    up.words[0x5000u + 1216u] = 0u;
    (void)pal4::SystemSettingSaveActions::OnKey(0x4040u, 0x12000u, up);
    Check(up.up_count == 1u, "up key stops at first row");

    Runtime down;
    down.words[0x12000u + 16u] = 208u;
    Check(pal4::SystemSettingSaveActions::OnKey(
          0x4040u, 0x12000u, down) == 1u && down.down_count == 1u,
          "down key advances hover before last row");
    down.words[0x5000u + 1216u] = 9u;
    (void)pal4::SystemSettingSaveActions::OnKey(0x4040u, 0x12000u, down);
    Check(down.down_count == 1u, "down key stops at last row");

    Runtime left;
    left.words[0x12000u + 16u] = 203u;
    Check(pal4::SystemSettingSaveActions::OnKey(
          0x4040u, 0x12000u, left) == 1u &&
          left.populated_page == 3,
          "left key repopulates previous page");
    left.words[0xe000u + 280u] = 0u;
    left.populated_page = 999;
    (void)pal4::SystemSettingSaveActions::OnKey(0x4040u, 0x12000u, left);
    Check(left.populated_page == 999, "left key stops at page zero");

    Runtime right;
    right.words[0x12000u + 16u] = 205u;
    Check(pal4::SystemSettingSaveActions::OnKey(
          0x4040u, 0x12000u, right) == 1u &&
          right.populated_page == 5,
          "right key repopulates next page");
    right.words[0xe000u + 280u] = 9u;
    right.populated_page = 999;
    (void)pal4::SystemSettingSaveActions::OnKey(0x4040u, 0x12000u, right);
    Check(right.populated_page == 999, "right key stops at page nine");

    Runtime confirm_key;
    confirm_key.words[0x12000u + 16u] = 28u;
    confirm_key.words[0x4000u + 504u] = 2u;
    Check(pal4::SystemSettingSaveActions::OnKey(
          0x4040u, 0x12000u, confirm_key) == 1u &&
          confirm_key.trigger_id == 229u,
          "confirm key opens context menu for current list mode");

    Runtime unknown_key;
    unknown_key.words[0x12000u + 16u] = 999u;
    Check(pal4::SystemSettingSaveActions::OnKey(
          0x4040u, 0x12000u, unknown_key) == 1u &&
          unknown_key.trigger_id == 0u,
          "unknown key on nonempty list returns true");
}
