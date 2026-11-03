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

struct Runtime final : pal4::ISmithWindowTradeRuntime,
                       pal4::ISmithWindowResetClickRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    bool visible = true;
    bool selected = true;
    bool active = false;
    bool below = false;
    bool requirements_met = true;
    bool sufficient = true;
    bool nonzero = true;
    bool matches = false;
    bool consume_callback = false;
    bool throw_dialog = false;
    std::int32_t equipment_id = 55;
    std::int32_t inventory_count = 80;
    std::int32_t equipment_property = 1;
    std::uint32_t mode = 0;
    std::uint32_t rows = 1;
    std::uint32_t template_id = 0;
    std::uint32_t style = 0;
    std::uint32_t assigned_destination = 0;
    std::uint32_t destroyed_callback = 0;
    std::string message;
    std::vector<pal4::SmithWindowTradeCallback> callbacks;
    std::vector<std::string> calls;

    std::uint8_t OnResetClick(std::uint32_t owner) override {
        return pal4::SmithWindow::OnResetTrade(owner, *this);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read-byte");
        if (address == object + 0x81u) return visible;
        Check(address == object + 0xacu, "selection offset");
        return selected;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == object + 0xb4u) return mode;
        Check(address == object + 0x40u, "equipment list offset");
        return 0x2000u;
    }
    std::int32_t EquipmentHoverItemId(std::uint32_t owner) override {
        Check(owner == object, "equipment owner");
        calls.emplace_back("equipment-hover");
        return equipment_id;
    }
    std::int32_t PrescriptionHoverItemId(std::uint32_t owner) override {
        Check(owner == object, "prescription owner");
        calls.emplace_back("prescription-hover");
        return 77;
    }
    bool SmithActive() override {
        calls.emplace_back("smith-active");
        return active;
    }
    std::int32_t InventoryCount() override {
        calls.emplace_back("inventory-count");
        return inventory_count;
    }
    std::uint32_t EquipmentRowCount(std::uint32_t list) override {
        Check(list == 0x2000u, "equipment list");
        return rows;
    }
    std::int32_t EquipmentProperty(std::int32_t id) override {
        Check(id == equipment_id, "equipment property id");
        return equipment_property;
    }
    bool QuantityBelowRequirement(std::int32_t prescription,
                                  std::int32_t equipment) override {
        Check(prescription == 77 && equipment == equipment_id,
              "quantity ids");
        return below;
    }
    bool CraftingRequirementsMet(std::int32_t prescription) override {
        Check(prescription == 77, "crafting prescription");
        return requirements_met;
    }
    bool EquipmentPropertySufficient(std::int32_t prescription,
                                     std::int32_t equipment) override {
        Check(prescription == 77 && equipment == equipment_id,
              "sufficiency ids");
        return sufficient;
    }
    bool EquipmentPropertyNonZero(std::int32_t equipment) override {
        Check(equipment == equipment_id, "nonzero id");
        return nonzero;
    }
    bool EquipmentPropertyMatches(std::int32_t equipment,
                                  std::int32_t prescription) override {
        Check(equipment == equipment_id && prescription == 77,
              "match ids");
        return matches;
    }
    std::string_view ElementPropertyName(std::int32_t prescription) override {
        Check(prescription == 77, "element name id");
        return "element";
    }
    std::string_view InventoryTextName(std::int32_t equipment) override {
        Check(equipment == equipment_id, "inventory text id");
        return "text-name";
    }
    void AssignInventoryText(std::uint32_t destination,
                             std::int32_t equipment) override {
        Check(equipment == equipment_id, "assigned equipment");
        assigned_destination = destination;
    }
    std::string_view InventoryDisplayName(std::int32_t equipment) override {
        Check(equipment == equipment_id, "display name id");
        return "display-name";
    }
    std::string_view ItemDataName(std::int32_t equipment) override {
        Check(equipment == equipment_id, "item-data id");
        return "item-data";
    }
    std::string_view TradeDataName(std::int32_t prescription) override {
        Check(prescription == 77, "trade-data id");
        return "trade-data";
    }
    std::string_view DialogTemplate(std::uint32_t id) override {
        template_id = id;
        return "template";
    }
    std::string FormatMessage(std::string_view format,
                              std::string_view name) override {
        return std::string(format) + ":" + std::string(name);
    }
    void CreateCommonDialog() override { calls.emplace_back("create-dialog"); }
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
                 std::string_view message,
                 std::vector<pal4::SmithWindowTradeCallback> callbacks = {}) {
    Check(pal4::SmithWindow::OnResetTrade(Runtime::object, runtime) == 1u,
          "trade action return");
    Check(runtime.template_id == id && runtime.style == style &&
              runtime.message == message && runtime.callbacks == callbacks,
          "dialog decision");
    Check(runtime.destroyed_callback ==
              (callbacks.empty() ? 0u : 0x4000u),
          "callback ownership");
}

void CheckGates() {
    Runtime invisible;
    invisible.visible = false;
    Check(pal4::SmithWindow::OnResetTrade(Runtime::object, invisible) == 1u &&
              invisible.calls.size() == 1u,
          "visibility gate avoids later reads");
    Runtime unselected;
    unselected.selected = false;
    Check(pal4::SmithWindow::OnResetTrade(Runtime::object, unselected) == 1u &&
              unselected.calls.size() == 2u,
          "selection gate avoids hover");
    Runtime no_equipment;
    no_equipment.equipment_id = -1;
    (void)pal4::SmithWindow::OnResetTrade(Runtime::object, no_equipment);
    Check(no_equipment.template_id == 0u &&
              no_equipment.calls.back() == "equipment-hover",
          "missing equipment exits before prescription lookup");
    Runtime insufficient_slots;
    insufficient_slots.inventory_count = 79;
    CheckDialog(insufficient_slots, 0x2du, 4u, "template");
    Runtime threshold;
    threshold.inventory_count = 80;
    CheckDialog(threshold, 0x5bu, 3u, "template",
                {pal4::SmithWindowTradeCallback::SelectionConfirm});
    Runtime empty_list;
    empty_list.mode = 1;
    empty_list.rows = 0;
    (void)pal4::SmithWindow::OnResetTrade(Runtime::object, empty_list);
    Check(empty_list.template_id == 0u, "empty equipment list exits");
}

void CheckModes() {
    using Callback = pal4::SmithWindowTradeCallback;
    Runtime base;
    CheckDialog(base, 0x5bu, 3u, "template", {Callback::SelectionConfirm});
    Runtime element;
    element.active = true;
    CheckDialog(element, 0x32u, 3u, "template:element",
                {Callback::SelectionConfirm});

    Runtime invalid_property;
    invalid_property.mode = 1;
    invalid_property.equipment_property = 0;
    CheckDialog(invalid_property, 0xbeu, 4u, "template");
    Runtime missing_quantity;
    missing_quantity.mode = 1;
    CheckDialog(missing_quantity, 0x5cu, 3u, "template",
                {Callback::EquipmentSelectionConfirm});
    Runtime choose_equipment;
    choose_equipment.mode = 1;
    choose_equipment.below = true;
    CheckDialog(choose_equipment, 0x37u, 3u, "template:text-name",
                {Callback::EquipmentSelectionPrompt});
    Check(choose_equipment.assigned_destination == Runtime::object + 0xd8u,
          "equipment text assignment");
    Runtime active_quantity;
    active_quantity.mode = 1;
    active_quantity.active = true;
    CheckDialog(active_quantity, 0x36u, 3u, "template:display-name",
                {Callback::EquipmentSelectionConfirm});
    Runtime use_item;
    use_item.mode = 1;
    use_item.active = true;
    use_item.below = true;
    CheckDialog(use_item, 0x37u, 3u, "template:text-name",
                {Callback::ItemUseConfirm});

    Runtime requirements;
    requirements.mode = 2;
    requirements.requirements_met = false;
    CheckDialog(requirements, 0x31u, 4u, "template:trade-data");
    Runtime insufficient_property;
    insufficient_property.mode = 2;
    insufficient_property.sufficient = false;
    CheckDialog(insufficient_property, 0x3du, 4u,
                "template:display-name");
    Runtime page_change;
    page_change.mode = 2;
    page_change.nonzero = false;
    CheckDialog(page_change, 0x5eu, 3u, "template",
                {Callback::PageChangeConfirm});
    Runtime active_page_change;
    active_page_change.mode = 3;
    active_page_change.active = true;
    active_page_change.nonzero = false;
    CheckDialog(active_page_change, 0x3cu, 3u,
                "template:display-name", {Callback::PageChangeConfirm});
    Runtime matching;
    matching.mode = 2;
    matching.matches = true;
    CheckDialog(matching, 0xccu, 4u, "template:item-data");
    Runtime active_matching;
    active_matching.mode = 3;
    active_matching.active = true;
    active_matching.matches = true;
    CheckDialog(active_matching, 0xccu, 4u, "template:item-data");
    Runtime page_prompt;
    page_prompt.mode = 2;
    CheckDialog(page_prompt, 0x3eu, 3u, "template:item-data",
                {Callback::PageChangePrompt});
    Runtime extended;
    extended.mode = 2;
    extended.active = true;
    CheckDialog(extended, 0x3eu, 3u, "template:item-data",
                {Callback::ExtendedCraftConfirm});
}

void CheckOwnershipAndThunk() {
    Runtime transferred;
    transferred.consume_callback = true;
    (void)pal4::SmithWindow::OnResetClickThunk(Runtime::object, 0x1234u,
                                               transferred);
    Check(transferred.template_id == 0x5bu &&
              transferred.destroyed_callback == 0u,
          "thunk composes with trade action and transfers callback");
    Runtime throwing;
    throwing.throw_dialog = true;
    bool caught = false;
    try {
        (void)pal4::SmithWindow::OnResetTrade(Runtime::object, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.destroyed_callback == 0x4000u,
          "callback released after dialog exception");
}

}  // namespace

int main() {
    try {
        CheckGates();
        CheckModes();
        CheckOwnershipAndThunk();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
