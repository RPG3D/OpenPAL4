#include "pal4/smith_window.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowPrescriptionRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t list = 0x2000u;
    static constexpr std::uint32_t recipe = 0x5000u;
    static constexpr std::uint32_t equipment = 0x6000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {object + 0x44u, list},
        {object + 0x50u, 0x3000u},
        {object + 0x54u, 0x3400u},
        {recipe + 8u, 0x7000u},
        {recipe + 0x18u, 0x7100u},
        {recipe + 0x28u, 0x7200u},
        {recipe + 0x68u, 77u},
        {recipe + 0x6cu, static_cast<std::uint32_t>(-12)},
        {recipe + 0x70u, 15u},
        {equipment + 0x168u, static_cast<std::uint32_t>(-1)},
        {equipment + 0x16cu, 0u},
        {equipment + 0x170u, 3u},
        {equipment + 0x174u, 9u},
    };
    std::vector<std::string> calls;
    std::vector<std::tuple<std::uint32_t, float,
                           std::string>> columns;
    std::vector<std::string> cell_texts;
    std::vector<std::tuple<std::uint32_t, std::uint32_t,
                           std::uint32_t>> cells;
    std::vector<std::uint32_t> row_ids;
    std::uint32_t removed_columns = 0u;
    std::uint32_t hover = 0xa000u;
    std::string shown_name;
    std::string shown_description;
    std::int32_t inventory_count = 2;
    bool has_effect = true;
    bool fail_name = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write");
    }
    void SetSortDirection(std::uint32_t target,
                          std::uint32_t direction) override {
        Check(target == list && direction == 0u, "sort arguments");
        calls.emplace_back("sort");
    }
    void ResetList(std::uint32_t target) override {
        Check(target == list, "reset list");
        calls.emplace_back("reset");
    }
    std::int32_t ColumnCount(std::uint32_t target) override {
        Check(target == list, "column count list");
        calls.emplace_back("column-count");
        return 3;
    }
    void RemoveColumn(std::uint32_t target,
                      std::uint32_t column) override {
        Check(target == list && column == 0u,
              "remove first old column");
        ++removed_columns;
        calls.emplace_back("remove-column");
    }
    void SetUserColumnSizing(std::uint32_t target,
                             bool enabled) override {
        Check(target == list && !enabled, "column sizing disabled");
        calls.emplace_back("disable-sizing");
    }
    void SetUserColumnDragging(std::uint32_t target,
                               bool enabled) override {
        Check(target == list && !enabled, "column dragging disabled");
        calls.emplace_back("disable-dragging");
    }
    void SetUserSortControl(std::uint32_t target,
                            bool enabled) override {
        Check(target == list && !enabled, "user sorting disabled");
        calls.emplace_back("disable-sort");
    }
    void AddColumn(std::uint32_t target, std::uint32_t column,
                   float width) override {
        Check(target == list, "add column list");
        columns.emplace_back(column, width, "");
        calls.emplace_back("add-column");
    }
    void SetColumnLabelImage(std::uint32_t target,
                             std::uint32_t column,
                             std::string_view imageset,
                             std::string_view image) override {
        Check(target == list && imageset == "zi0" &&
                  column < columns.size(), "column label target");
        std::get<2>(columns[column]) = image;
        calls.emplace_back("column-image");
    }
    std::int32_t InventoryCount(std::uint32_t mode) override {
        Check(mode < 4u, "inventory group");
        calls.emplace_back("inventory-count");
        return inventory_count;
    }
    std::uint32_t InventoryIdAt(std::uint32_t mode,
                                std::uint32_t index) override {
        Check(mode < 4u && index < 2u, "inventory group index");
        calls.emplace_back("inventory-id");
        return index == 0u ? 42u : 43u;
    }
    bool HasRecipe(std::uint32_t id) override {
        calls.emplace_back("has-recipe");
        return id == 42u;
    }
    std::uint32_t RecipeForId(std::uint32_t id) override {
        Check(id == 42u, "recipe ID");
        calls.emplace_back("recipe");
        return recipe;
    }
    void AddRow(std::uint32_t target,
                std::uint32_t row_id) override {
        Check(target == list, "add row list");
        row_ids.push_back(row_id);
        calls.emplace_back("add-row");
    }
    std::uint32_t RowCount(std::uint32_t target) override {
        Check(target == list, "row count list");
        calls.emplace_back("row-count");
        return static_cast<std::uint32_t>(row_ids.size());
    }
    std::string_view Utf8Text(std::uint32_t source) override {
        Check(source == 0x7000u || source == 0x7100u ||
                  source == 0x7200u ||
                  source == 0u, "recipe name source");
        calls.emplace_back("utf8");
        if (source == 0u) return "";
        if (source == 0x7000u) return "formula";
        return source == 0x7100u ? "selected formula" : "plain desc";
    }
    void SetText(std::uint32_t window,
                 std::string_view text) override {
        Check(window == 0x3000u || window == 0x3400u,
              "prescription text target");
        if (window == 0x3000u)
            shown_name = text;
        else
            shown_description = text;
        calls.emplace_back("set-text");
    }
    std::uint32_t CreateCell(std::uint32_t target,
                             std::string_view text) override {
        Check(target == list, "cell list");
        cell_texts.emplace_back(text);
        calls.emplace_back("create-cell");
        if (fail_name && cell_texts.size() == 1u) return 0u;
        return 0xa000u + static_cast<std::uint32_t>(
            cell_texts.size() - 1u) * 0x1000u;
    }
    void SetItem(std::uint32_t target, std::uint32_t cell,
                 std::uint32_t column,
                 std::uint32_t row) override {
        Check(target == list, "set cell list");
        cells.emplace_back(cell, column, row);
        calls.emplace_back("set-item");
    }
    std::uint32_t EquipmentForId(std::uint32_t id) override {
        Check(id == 77u, "equipment ID");
        calls.emplace_back("equipment");
        return equipment;
    }
    std::string_view DescribeEquipment(
        std::uint32_t item) override {
        Check(item == equipment, "description equipment");
        calls.emplace_back("describe-equipment");
        return "equipment desc";
    }
    std::string Utf8Buffer(std::string_view text) override {
        calls.emplace_back("utf8-buffer");
        return std::string(text);
    }
    std::optional<std::string_view> EffectName(
        std::uint32_t item) override {
        Check(item == equipment, "effect owner");
        calls.emplace_back("effect-name");
        if (!has_effect) return std::nullopt;
        return "bonus";
    }
    std::uint32_t HoverItem(std::uint32_t target) override {
        Check(target == list, "hover list");
        calls.emplace_back("hover-item");
        return hover;
    }
    std::uint32_t Refresh(std::uint32_t owner) override {
        Check(owner == object, "refresh owner");
        calls.emplace_back("refresh");
        return 0xf00du;
    }
};

void CheckModeZero() {
    Runtime runtime;
    Check(pal4::SmithWindow::LoadData(Runtime::object, 0u,
                                      runtime) == 0xf00du,
          "refresh result forwarding");
    Check(runtime.words.at(Runtime::object + 0xb4u) == 0u &&
              runtime.removed_columns == 3u &&
              runtime.columns ==
                  (std::vector<std::tuple<std::uint32_t, float,
                                          std::string>>{
                      {0u, 0.5f, "mingcheng"},
                      {1u, 0.197f, "leibie"},
                      {2u, 0.197f, "dengji"}}),
          "mode zero configures three labeled columns");
    Check(runtime.row_ids == (std::vector<std::uint32_t>{0u}) &&
              runtime.cell_texts ==
                  (std::vector<std::string>{"formula", "bonus", "3"}) &&
              runtime.cells ==
                  (std::vector<std::tuple<std::uint32_t,
                                          std::uint32_t,
                                          std::uint32_t>>{
                      {0xa000u, 0u, 0u},
                      {0xb000u, 1u, 0u},
                      {0xc000u, 2u, 0u}}) &&
              runtime.words.at(0xa000u + 0x138u) == 42u &&
              runtime.words.at(0xc000u + 0x138u) == 42u,
          "mode zero filters missing recipe and selects first positive quantity");
    Check(runtime.shown_name == "selected formula" &&
              runtime.shown_description == "equipment desc" &&
              runtime.calls.back() == "refresh",
          "hover item resolves recipe and refreshes details");
}

void CheckOtherModes() {
    constexpr std::array<std::string_view, 3> labels{
        "", "xuyaoqianli", "xuyaolingxu"};
    for (std::uint32_t mode = 1u; mode <= 2u; ++mode) {
        Runtime runtime;
        runtime.hover = 0u;
        (void)pal4::SmithWindow::LoadData(Runtime::object, mode, runtime);
        Check(runtime.words.at(Runtime::object + 0xb4u) == mode &&
                  runtime.columns ==
                      (std::vector<std::tuple<std::uint32_t, float,
                                              std::string>>{
                          {0u, 0.5f, "mingcheng"},
                          {1u, 0.395f, std::string(labels[mode])}}) &&
                  runtime.cell_texts ==
                      (std::vector<std::string>{
                          "formula", mode == 1u ? "-12" : "15"}) &&
                  runtime.shown_name.empty() &&
                  runtime.shown_description.empty(),
              "mode one/two column label, signed count and empty hover");
    }

    Runtime unsupported;
    unsupported.hover = 0u;
    (void)pal4::SmithWindow::LoadData(Runtime::object, 3u,
                                      unsupported);
    Check(unsupported.columns.empty() &&
              unsupported.cell_texts ==
                  (std::vector<std::string>{"formula"}),
          "unsupported mode adds no columns or quantity cell");
}

void CheckPrescriptionDescription() {
    Runtime runtime;
    runtime.words[Runtime::object + 0xb4u] = 0u;
    pal4::SmithWindow::UpdatePrescriptionItemDescription(
        Runtime::object, Runtime::recipe, runtime);
    Check(runtime.shown_description == "equipment desc" &&
              runtime.calls == (std::vector<std::string>{
                  "read", "read", "equipment",
                  "describe-equipment", "utf8-buffer",
                  "read", "set-text"}),
          "mode zero description comes from equipment lookup");

    runtime.calls.clear();
    runtime.words[Runtime::object + 0xb4u] = 1u;
    pal4::SmithWindow::UpdatePrescriptionItemDescription(
        Runtime::object, Runtime::recipe, runtime);
    Check(runtime.shown_description == "plain desc" &&
              runtime.calls == (std::vector<std::string>{
                  "read", "read", "utf8", "read", "set-text"}),
          "other modes use recipe description field +0x28");

    runtime.calls.clear();
    runtime.words[Runtime::recipe + 0x28u] = 0u;
    pal4::SmithWindow::UpdatePrescriptionItemDescription(
        Runtime::object, Runtime::recipe, runtime);
    Check(runtime.shown_description.empty() &&
              runtime.calls.size() == 5u,
          "null direct description becomes empty text");

    runtime.calls.clear();
    pal4::SmithWindow::UpdatePrescriptionItemDescription(
        Runtime::object, 0u, runtime);
    Check(runtime.shown_description.empty() &&
              runtime.calls == (std::vector<std::string>{
                  "read", "set-text"}),
          "null recipe clears description");
}

void CheckMissingEffectAndFailure() {
    Runtime no_effect;
    no_effect.has_effect = false;
    (void)pal4::SmithWindow::LoadData(Runtime::object, 0u,
                                      no_effect);
    Check(no_effect.cell_texts ==
              (std::vector<std::string>{"formula", "3"}) &&
              no_effect.cells.size() == 2u &&
              std::get<1>(no_effect.cells.back()) == 2u,
          "missing effect leaves second column empty");

    Runtime no_inventory;
    no_inventory.inventory_count = 0;
    no_inventory.hover = 0u;
    (void)pal4::SmithWindow::LoadData(Runtime::object, 0u,
                                      no_inventory);
    Check(no_inventory.row_ids.empty() &&
              no_inventory.shown_name.empty(),
          "empty inventory still configures columns and clears details");

    Runtime failed;
    failed.fail_name = true;
    bool threw = false;
    try {
        (void)pal4::SmithWindow::LoadData(Runtime::object, 0u, failed);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && failed.row_ids.size() == 1u &&
              failed.cells.empty() &&
              failed.calls.back() == "create-cell",
          "first-cell allocation failure stops before item writes");
}

void CheckPrescriptionName() {
    Runtime runtime;
    pal4::SmithWindow::UpdatePrescriptionItemName(
        Runtime::object, Runtime::recipe, runtime);
    Check(runtime.shown_name == "selected formula" &&
              runtime.calls == (std::vector<std::string>{
                  "read", "utf8", "read", "set-text"}),
          "selected recipe name uses field +0x18 and window +0x50");

    runtime.calls.clear();
    runtime.words[Runtime::recipe + 0x18u] = 0u;
    pal4::SmithWindow::UpdatePrescriptionItemName(
        Runtime::object, Runtime::recipe, runtime);
    Check(runtime.shown_name.empty() &&
              runtime.calls.size() == 4u,
          "null recipe name pointer sets empty text");

    runtime.calls.clear();
    pal4::SmithWindow::UpdatePrescriptionItemName(
        Runtime::object, 0u, runtime);
    Check(runtime.shown_name.empty() &&
              runtime.calls == (std::vector<std::string>{
                  "read", "set-text"}),
          "null recipe skips conversion and clears text");
}

}  // namespace

int main() {
    try {
        CheckModeZero();
        CheckOtherModes();
        CheckMissingEffectAndFailure();
        CheckPrescriptionName();
        CheckPrescriptionDescription();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
