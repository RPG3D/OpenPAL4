#include "pal4/smith_window.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowEquipmentSelectionRuntime,
                       pal4::ISmithWindowExtendedCraftRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {object + 0x40u, 0x2000u},
        {object + 0x48u, 0x3000u},
        {object + 0x4cu, 0x3004u},
        {0x5000u + 0x9cu, 0x7000u},
        {0x5000u + 0xacu, 0x7004u},
        {0x5000u + 0xbcu, 0u},
        {0x5000u + 0x184u, 9u},
        {0x9000u + 0x138u, 55u},
    };
    std::vector<std::string> calls;
    std::vector<std::uint32_t> templates;
    std::vector<std::string> dialogs;
    std::map<std::uint32_t, std::string> text;
    std::vector<std::uint32_t> inventory_ids;
    std::vector<std::uint32_t> read_addresses;
    std::int32_t effect_result = 0;
    std::int32_t extended_result = 0;
    std::uint32_t hover = 0x9000u;
    std::uint32_t destroyed_callback = 0u;
    bool consume_callback = false;
    bool throw_dialog = false;
    std::uint32_t rows = 2u;
    std::uint32_t row_index = 0u;
    std::vector<std::uint32_t> selected_rows;

    std::uint32_t ReadWord(std::uint32_t address) override {
        read_addresses.push_back(address);
        calls.emplace_back("read");
        return words.at(address);
    }
    std::int32_t PrescriptionHoverItemId(
        std::uint32_t owner) override {
        Check(owner == object, "prescription owner");
        calls.emplace_back("prescription-id");
        return 77;
    }
    std::int32_t EquipmentHoverItemId(
        std::uint32_t owner) override {
        Check(owner == object, "equipment owner");
        calls.emplace_back("equipment-id");
        return 88;
    }
    void RegisterSmithSystem() override {
        calls.emplace_back("register");
    }
    std::int32_t ApplyItemEffects(std::int32_t prescription_id,
                                  std::int32_t equipment_id) override {
        Check(prescription_id == 77 && equipment_id == 88,
              "item use IDs");
        calls.emplace_back("apply");
        return effect_result;
    }
    std::int32_t ProcessExtendedCraft(
        std::int32_t prescription_id,
        std::int32_t equipment_id) override {
        Check(prescription_id == 77 && equipment_id == 88,
              "extended craft IDs");
        calls.emplace_back("extended-craft");
        return extended_result;
    }
    std::string ItemDataName(std::int32_t equipment_id) override {
        Check(equipment_id == 88, "item data equipment ID");
        calls.emplace_back("item-data-name");
        return "raw gem";
    }
    std::uint32_t UIFrameManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    void CreateCommonDialog(std::uint32_t manager) override {
        Check(manager == 0x4000u, "dialog manager");
        calls.emplace_back("create-dialog");
    }
    std::string_view DialogTemplate(std::uint32_t id) override {
        Check(id == 0x39u || id == 0x38u || id == 0x3fu,
              "dialog template ID");
        templates.push_back(id);
        calls.emplace_back("template");
        return id == 0x39u ? "used %s"
            : id == 0x3fu ? "crafted %s" : "cannot use";
    }
    std::uint32_t FindInventoryItem(std::uint32_t id,
                                    std::uint32_t category) override {
        Check(category == 7u && (id == 88u || id == 55u),
              "inventory lookup arguments");
        inventory_ids.push_back(id);
        calls.emplace_back("find");
        return 0x5000u;
    }
    std::string FormatMessage(std::string_view format,
                               std::string_view item_name) override {
        Check((format == "used %s" && item_name == "raw sword") ||
                  (format == "crafted %s" && item_name == "raw gem"),
              "success message arguments");
        calls.emplace_back("format");
        return format == "used %s" ? "used raw sword"
            : "crafted raw gem";
    }
    std::uint32_t CreateCloseCallback(
        std::uint32_t owner) override {
        Check(owner == object, "callback owner");
        calls.emplace_back("create-callback");
        return 0x6000u;
    }
    void ShowDialog(std::string_view message,
                    const std::array<float, 2>& size,
                    std::uint32_t style,
                    std::uint32_t& callback) override {
        Check(style == 4u &&
                  std::bit_cast<std::uint32_t>(size[0]) ==
                      0x3e99999au &&
                  std::bit_cast<std::uint32_t>(size[1]) ==
                      0x3e99999au,
              "dialog dimensions and style");
        dialogs.emplace_back(message);
        calls.emplace_back("show-dialog");
        if (consume_callback) callback = 0u;
        if (throw_dialog) throw std::runtime_error("dialog failure");
    }
    void DestroyCallback(std::uint32_t callback,
                         std::uint32_t deleting) override {
        Check(callback == 0x6000u && deleting == 1u,
              "callback destructor arguments");
        destroyed_callback = callback;
        calls.emplace_back("destroy-callback");
    }
    std::uint32_t HoverItem(std::uint32_t list) override {
        Check(list == 0x2000u, "hover list");
        calls.emplace_back("hover-item");
        return hover;
    }
    std::uint32_t ItemRowIndex(std::uint32_t list,
                               std::uint32_t item) override {
        Check(list == 0x2000u && item == hover,
              "item row lookup arguments");
        calls.emplace_back("row-index");
        return row_index;
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        Check(list == 0x2000u, "row count list");
        calls.emplace_back("row-count");
        return rows;
    }
    void SetHoverRow(std::uint32_t list,
                     std::uint32_t row) override {
        Check(list == 0x2000u, "set hover list");
        selected_rows.push_back(row);
        calls.emplace_back("set-hover-row");
    }
    std::string_view Utf8Text(std::uint32_t source) override {
        Check(source == 0x7004u, "selected item name source");
        calls.emplace_back("utf8");
        return "selected sword";
    }
    void SetText(std::uint32_t window,
                 std::string_view value) override {
        text[window] = value;
        calls.emplace_back("text");
    }
    std::string_view DescribeItem(std::uint32_t item) override {
        Check(item == 0x5000u, "description item");
        calls.emplace_back("describe");
        return "item description";
    }
    std::string_view ReadText(std::uint32_t source) override {
        Check(source == 0x7000u || source == 0u,
              "raw text source");
        calls.emplace_back("read-text");
        return source == 0u ? "" : "raw sword";
    }
    std::string Utf8Buffer(std::string_view value) override {
        calls.emplace_back("utf8-buffer");
        return std::string(value);
    }
    void Call49F080(std::uint32_t owner) override {
        Check(owner == object, "49F080 owner");
        calls.emplace_back("49F080");
    }
    void Call49EF80(std::uint32_t owner) override {
        Check(owner == object, "49EF80 owner");
        calls.emplace_back("49EF80");
    }
    void HideSelectionPanel(std::uint32_t owner) override {
        Check(owner == object, "hide owner");
        calls.emplace_back("hide");
    }
};

void CheckEventGateAndNonDialogResult() {
    Runtime ignored;
    Check(pal4::SmithWindow::OnItemUseConfirm(
              Runtime::object, 2u, ignored) == 1u &&
              ignored.calls.empty(),
          "other events have no side effects");

    Runtime unchanged;
    unchanged.effect_result = 2;
    Check(pal4::SmithWindow::OnItemUseConfirm(
              Runtime::object, 1u, unchanged) == 1u &&
              unchanged.dialogs.empty() &&
              unchanged.text.empty() &&
              unchanged.calls.at(unchanged.calls.size() - 3u) ==
                  "49F080" &&
              unchanged.calls.back() == "hide",
          "other nonzero result refreshes without dialog");
}

void CheckSuccessAndFailureDialogs() {
    Runtime success;
    (void)pal4::SmithWindow::OnItemUseConfirm(
        Runtime::object, 1u, success);
    Check(success.templates == std::vector<std::uint32_t>{0x39u} &&
              success.dialogs ==
                  std::vector<std::string>{"used raw sword"} &&
              success.inventory_ids ==
                  std::vector<std::uint32_t>{88u, 88u} &&
              success.text.at(0x3000u) == "selected sword" &&
              success.text.at(0x3004u) == "item description" &&
              success.destroyed_callback == 0x6000u &&
              success.calls.back() == "hide",
          "success formats raw name then updates equipment detail");
    auto shown = false;
    auto looked_up_after_dialog = false;
    for (const auto& call : success.calls) {
        if (call == "show-dialog") shown = true;
        if (shown && call == "equipment-id") {
            looked_up_after_dialog = true;
            break;
        }
    }
    Check(looked_up_after_dialog,
          "dialog precedes selected equipment lookup");

    Runtime failure;
    failure.effect_result = -1;
    (void)pal4::SmithWindow::OnItemUseConfirm(
        Runtime::object, 1u, failure);
    Check(failure.templates == std::vector<std::uint32_t>{0x38u} &&
              failure.dialogs ==
                  std::vector<std::string>{"cannot use"} &&
              failure.inventory_ids ==
                  std::vector<std::uint32_t>{55u} &&
              failure.text.at(0x3000u) == "selected sword" &&
              failure.destroyed_callback == 0x6000u &&
              failure.calls.back() == "hide",
          "failure dialog uses hovered list item");

    Runtime no_hover;
    no_hover.effect_result = -1;
    no_hover.hover = 0u;
    (void)pal4::SmithWindow::OnItemUseConfirm(
        Runtime::object, 1u, no_hover);
    Check(no_hover.inventory_ids.empty() &&
              no_hover.text.at(0x3000u).empty() &&
              no_hover.text.at(0x3004u).empty(),
          "failure with no hover clears equipment detail");
}

void CheckCallbackOwnership() {
    Runtime transferred;
    transferred.consume_callback = true;
    (void)pal4::SmithWindow::OnItemUseConfirm(
        Runtime::object, 1u, transferred);
    Check(transferred.destroyed_callback == 0u,
          "dialog may consume callback");

    Runtime throwing;
    throwing.throw_dialog = true;
    bool caught = false;
    try {
        (void)pal4::SmithWindow::OnItemUseConfirm(
            Runtime::object, 1u, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.destroyed_callback == 0x6000u,
          "callback released if dialog throws");
}

void CheckEquipmentSelectionSuccess() {
    Runtime runtime;
    Check(pal4::SmithWindow::OnEquipmentSelectionConfirm(
              Runtime::object, 1u, runtime) == 1u &&
              runtime.templates == std::vector<std::uint32_t>{0x39u} &&
              runtime.dialogs ==
                  std::vector<std::string>{"used raw sword"} &&
              runtime.inventory_ids ==
                  std::vector<std::uint32_t>{88u, 88u} &&
              runtime.text.at(0x3000u) == "selected sword" &&
              runtime.destroyed_callback == 0x6000u &&
              runtime.calls.back() == "hide",
          "equipment selection success dialog and detail update");
    Check(std::find(runtime.read_addresses.begin(),
                    runtime.read_addresses.end(),
                    0x5000u + 0x184u) !=
              runtime.read_addresses.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "row-index") != runtime.calls.end(),
          "success reads the signed quantity after locating hover row");
}

void CheckEquipmentSelectionFailure() {
    Runtime runtime;
    runtime.effect_result = -1;
    runtime.row_index = 9u;
    Check(pal4::SmithWindow::OnEquipmentSelectionConfirm(
              Runtime::object, 1u, runtime) == 1u &&
              runtime.templates == std::vector<std::uint32_t>{0x38u} &&
              runtime.selected_rows == std::vector<std::uint32_t>{1u} &&
              runtime.inventory_ids == std::vector<std::uint32_t>{55u} &&
              runtime.calls.back() == "hide",
          "failed selection clamps hover row before updating details");

    Runtime in_range;
    in_range.effect_result = -1;
    in_range.row_index = 0u;
    (void)pal4::SmithWindow::OnEquipmentSelectionConfirm(
        Runtime::object, 1u, in_range);
    Check(in_range.selected_rows == std::vector<std::uint32_t>{0u},
          "in-range hover row is retained");

    Runtime empty;
    empty.effect_result = -1;
    empty.rows = 0u;
    empty.hover = 0u;
    (void)pal4::SmithWindow::OnEquipmentSelectionConfirm(
        Runtime::object, 1u, empty);
    Check(empty.selected_rows.empty() &&
              empty.inventory_ids.empty() &&
              empty.text.at(0x3000u).empty() &&
              empty.text.at(0x3004u).empty(),
          "empty row set skips hover reset and clears details");

    Runtime other;
    other.effect_result = 2;
    (void)pal4::SmithWindow::OnEquipmentSelectionConfirm(
        Runtime::object, 1u, other);
    Check(other.dialogs.empty() && other.calls.back() == "hide",
          "other nonzero result only refreshes and hides");
}

void CheckEquipmentSelectionCallback() {
    Runtime ignored;
    Check(pal4::SmithWindow::OnEquipmentSelectionConfirm(
              Runtime::object, 0u, ignored) == 1u &&
              ignored.calls.empty(),
          "selection ignores events other than one");

    Runtime transferred;
    transferred.consume_callback = true;
    (void)pal4::SmithWindow::OnEquipmentSelectionConfirm(
        Runtime::object, 1u, transferred);
    Check(transferred.destroyed_callback == 0u,
          "selection dialog may consume callback");

    Runtime throwing;
    throwing.throw_dialog = true;
    bool caught = false;
    try {
        (void)pal4::SmithWindow::OnEquipmentSelectionConfirm(
            Runtime::object, 1u, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.destroyed_callback == 0x6000u,
          "selection callback released on dialog exception");
}

void CheckExtendedCraft() {
    Runtime ignored;
    Check(pal4::SmithWindow::OnExtendedCraftConfirm(
              Runtime::object, 2u, ignored) == 1u &&
              ignored.calls.empty(),
          "extended craft ignores other events");

    Runtime failed;
    failed.extended_result = -1;
    Check(pal4::SmithWindow::OnExtendedCraftConfirm(
              Runtime::object, 1u, failed) == 1u &&
              failed.dialogs.empty() &&
              failed.calls.at(failed.calls.size() - 3u) ==
                  "49F080" &&
              failed.calls.back() == "hide",
          "nonzero extended result still refreshes and hides");

    Runtime success;
    Check(pal4::SmithWindow::OnExtendedCraftConfirm(
              Runtime::object, 1u, success) == 1u &&
              success.templates == std::vector<std::uint32_t>{0x3fu} &&
              success.dialogs ==
                  std::vector<std::string>{"crafted raw gem"} &&
              success.inventory_ids ==
                  std::vector<std::uint32_t>{88u} &&
              success.text.at(0x3000u) == "selected sword" &&
              success.destroyed_callback == 0x6000u &&
              success.calls.back() == "hide",
          "successful extended craft formats item-data name");
    Check(std::count(success.calls.begin(), success.calls.end(),
                     "register") == 2 &&
              std::find(success.calls.begin(), success.calls.end(),
                        "item-data-name") != success.calls.end(),
          "extended success re-registers Smith system for item data");

    Runtime transferred;
    transferred.consume_callback = true;
    (void)pal4::SmithWindow::OnExtendedCraftConfirm(
        Runtime::object, 1u, transferred);
    Check(transferred.destroyed_callback == 0u,
          "extended dialog may consume callback");

    Runtime throwing;
    throwing.throw_dialog = true;
    bool caught = false;
    try {
        (void)pal4::SmithWindow::OnExtendedCraftConfirm(
            Runtime::object, 1u, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.destroyed_callback == 0x6000u,
          "extended callback released on dialog exception");
}

void CheckPageChangeConfirm() {
    Runtime ignored;
    Check(pal4::SmithWindow::OnPageChangeConfirm(
              Runtime::object, 0u, ignored) == 1u &&
              ignored.calls.empty(),
          "page confirm ignores other events");

    Runtime failed;
    failed.extended_result = -1;
    (void)pal4::SmithWindow::OnPageChangeConfirm(
        Runtime::object, 1u, failed);
    Check(failed.dialogs.empty() && failed.calls.back() == "hide",
          "page confirm nonzero result uses shared refresh path");

    Runtime success;
    (void)pal4::SmithWindow::OnPageChangeConfirm(
        Runtime::object, 1u, success);
    Check(success.dialogs ==
              std::vector<std::string>{"crafted raw gem"} &&
              success.text.at(0x3000u) == "selected sword" &&
              success.destroyed_callback == 0x6000u &&
              success.calls.back() == "hide",
          "page confirm success uses shared craft path");
}

}  // namespace

int main() {
    try {
        CheckEventGateAndNonDialogResult();
        CheckSuccessAndFailureDialogs();
        CheckCallbackOwnership();
        CheckEquipmentSelectionSuccess();
        CheckEquipmentSelectionFailure();
        CheckEquipmentSelectionCallback();
        CheckExtendedCraft();
        CheckPageChangeConfirm();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
