#include "pal4/smith_window.h"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowConfirmTradeRuntime,
                       pal4::ISmithWindowConfirmClickRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::int32_t prescription_id = 77;
    std::int32_t inventory_count = 80;
    std::uint32_t mode = 0;
    bool active = false;
    bool requirements_met = true;
    bool has_elements = false;
    bool consume_callback = false;
    bool throw_dialog = false;
    std::uint8_t selection_flag = 9u;
    std::uint32_t template_id = 0;
    std::uint32_t style = 0;
    std::uint32_t destroyed_callback = 0;
    std::string message;
    std::vector<std::string> calls;
    std::vector<pal4::SmithWindowTradeCallback> callbacks;

    std::uint8_t OnConfirmTrade(std::uint32_t owner) override {
        return pal4::SmithWindow::OnConfirmTrade(owner, *this);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == object + 0xacu, "selection flag offset");
        selection_flag = value;
        calls.emplace_back(value == 0u ? "clear" : "select");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == object + 0xb4u, "mode offset");
        return mode;
    }
    std::int32_t PrescriptionHoverItemId(std::uint32_t owner) override {
        Check(owner == object, "hover owner");
        calls.emplace_back("hover");
        return prescription_id;
    }
    bool SmithActive() override { return active; }
    std::int32_t InventoryCount() override { return inventory_count; }
    bool CraftingRequirementsMet(std::int32_t id) override {
        Check(id == prescription_id, "crafting ID");
        calls.emplace_back("requirements");
        return requirements_met;
    }
    bool HasElements(std::int32_t id) override {
        Check(id == prescription_id, "element ID");
        calls.emplace_back("elements");
        return has_elements;
    }
    void CollectMatchingInventoryItemIds(std::uint32_t owner,
                                         std::int32_t id) override {
        Check(owner == object && id == prescription_id,
              "inventory collection arguments");
        calls.emplace_back("collect");
    }
    void UpdateAvailableEquipmentCategories(std::uint32_t owner,
                                            std::int32_t id) override {
        Check(owner == object && id == prescription_id,
              "category update arguments");
        calls.emplace_back("categories");
    }
    void ShowSelectionPanel(std::uint32_t owner) override {
        Check(owner == object, "selection panel owner");
        calls.emplace_back("panel");
    }
    void RefreshCollectedEquipmentList(std::uint32_t owner) override {
        Check(owner == object, "collected list owner");
        calls.emplace_back("refresh");
    }
    std::string_view TradeDataName(std::int32_t id) override {
        Check(id == prescription_id, "trade-data ID");
        return "trade-data";
    }
    std::string_view ElementPropertyName(std::int32_t id) override {
        Check(id == prescription_id, "element-property ID");
        return "element";
    }
    std::string_view DialogTemplate(std::uint32_t id) override {
        template_id = id;
        return "template";
    }
    std::string FormatMessage(std::string_view format,
                              std::string_view name) override {
        return std::string(format) + ":" + std::string(name);
    }
    void CreateCommonDialog() override { calls.emplace_back("dialog"); }
    std::uint32_t CreateCallback(
        std::uint32_t owner,
        pal4::SmithWindowTradeCallback kind) override {
        Check(owner == object, "callback owner");
        callbacks.push_back(kind);
        return 0x4000u;
    }
    void ShowDialog(std::string_view text,
                    const std::array<float, 2>& size,
                    std::uint32_t dialog_style,
                    std::uint32_t& callback) override {
        Check(std::bit_cast<std::uint32_t>(size[0]) == 0x3e99999au &&
                  std::bit_cast<std::uint32_t>(size[1]) == 0x3e99999au,
              "original dialog dimensions");
        message = text;
        style = dialog_style;
        calls.emplace_back("show-dialog");
        if (consume_callback) callback = 0u;
        if (throw_dialog) throw std::runtime_error("dialog failure");
    }
    void DestroyCallback(std::uint32_t callback,
                         std::uint32_t deleting) override {
        Check(callback == 0x4000u && deleting == 1u,
              "callback destruction");
        destroyed_callback = callback;
    }
};

void CheckDialog(Runtime& runtime, std::uint32_t id, std::uint32_t style,
                 std::string_view message, bool callback = false) {
    Check(pal4::SmithWindow::OnConfirmTrade(Runtime::object, runtime) == 1u,
          "confirm return");
    Check(runtime.selection_flag == 0u && runtime.template_id == id &&
              runtime.style == style && runtime.message == message,
          "confirm dialog decision");
    Check(runtime.callbacks.size() == (callback ? 1u : 0u) &&
              runtime.destroyed_callback == (callback ? 0x4000u : 0u),
          "confirm callback cleanup");
    if (callback) {
        Check(runtime.callbacks.front() ==
                  pal4::SmithWindowTradeCallback::SelectionConfirm,
              "selection confirmation callback");
    }
}

void CheckGatesAndDialogs() {
    Runtime no_hover;
    no_hover.prescription_id = -1;
    Check(pal4::SmithWindow::OnConfirmTrade(Runtime::object, no_hover) == 1u &&
              no_hover.selection_flag == 0u &&
              no_hover.calls == std::vector<std::string>{"clear", "hover"},
          "missing hover clears selection and returns");

    Runtime capacity;
    capacity.inventory_count = 79;
    CheckDialog(capacity, 0x2du, 4u, "template");
    Runtime threshold;
    threshold.inventory_count = 80;
    CheckDialog(threshold, 0x5bu, 3u, "template", true);

    for (const auto mode : {0u, 1u, 2u}) {
        Runtime requirements;
        requirements.mode = mode;
        requirements.requirements_met = false;
        CheckDialog(requirements, 0x31u, 4u,
                    "template:trade-data");
    }

    Runtime no_elements;
    CheckDialog(no_elements, 0x5bu, 3u, "template", true);
    Runtime active_no_elements;
    active_no_elements.active = true;
    CheckDialog(active_no_elements, 0x32u, 3u,
                "template:element", true);
}

void CheckSelectionPaths() {
    Runtime collected;
    collected.has_elements = true;
    Check(pal4::SmithWindow::OnConfirmTrade(Runtime::object, collected) == 1u &&
              collected.selection_flag == 1u &&
              collected.calls == (std::vector<std::string>{
                  "clear", "hover", "requirements", "elements",
                  "collect", "panel", "refresh", "select"}),
          "mode-zero element selection order");

    for (const auto mode : {1u, 2u}) {
        Runtime available;
        available.mode = mode;
        Check(pal4::SmithWindow::OnConfirmTrade(Runtime::object,
                                               available) == 1u &&
                  available.selection_flag == 1u &&
                  available.calls == (std::vector<std::string>{
                      "clear", "hover", "requirements", "categories",
                      "panel", "select"}),
              "nonzero-mode category selection order");
    }
}

void CheckOwnershipAndThunk() {
    Runtime transferred;
    transferred.consume_callback = true;
    Check(pal4::SmithWindow::OnConfirmClickThunk(
              Runtime::object, 0x1234u, transferred) == 1u &&
              transferred.template_id == 0x5bu &&
              transferred.destroyed_callback == 0u,
          "thunk composition and callback transfer");
    Runtime throwing;
    throwing.throw_dialog = true;
    bool caught = false;
    try {
        (void)pal4::SmithWindow::OnConfirmTrade(Runtime::object, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.destroyed_callback == 0x4000u,
          "callback released after dialog failure");
}

}  // namespace

int main() {
    try {
        CheckGatesAndDialogs();
        CheckSelectionPaths();
        CheckOwnershipAndThunk();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
