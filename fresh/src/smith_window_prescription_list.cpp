#include "pal4/smith_window.h"

#include <array>
#include <cstdint>
#include <new>
#include <string>
#include <string_view>

namespace pal4 {
namespace {

void ConfigureColumns(const std::uint32_t list,
                      const std::uint32_t mode,
                      ISmithWindowPrescriptionRuntime& runtime) {
    constexpr std::array<std::string_view, 3> labels{
        "leibie", "xuyaoqianli", "xuyaolingxu"};
    const auto columns = mode == 0u ? 3u :
                         mode == 1u || mode == 2u ? 2u : 0u;
    for (std::uint32_t column = 0u; column < columns; ++column) {
        const auto width = column == 0u ? 0.5f :
                           mode == 0u ? 0.197f : 0.395f;
        runtime.AddColumn(list, column, width);
        const auto label = column == 0u ? "mingcheng" :
                           mode == 0u ? labels[0] : labels[mode];
        if (mode == 0u && column == 2u)
            runtime.SetColumnLabelImage(list, column, "zi0", "dengji");
        else
            runtime.SetColumnLabelImage(list, column, "zi0", label);
    }
}

std::uint32_t CreateRequiredCell(const std::uint32_t list,
                                 const std::string_view text,
                                 ISmithWindowPrescriptionRuntime& runtime) {
    const auto cell = runtime.CreateCell(list, text);
    if (cell == 0u) throw std::bad_alloc();
    return cell;
}

void SetCell(const std::uint32_t list, const std::uint32_t column,
             const std::uint32_t cell,
             ISmithWindowPrescriptionRuntime& runtime) {
    runtime.SetItem(list, cell, column, runtime.RowCount(list) - 1u);
}

void PopulateRow(const std::uint32_t list,
                 const std::uint32_t id, const std::uint32_t recipe,
                 const std::uint32_t mode,
                 ISmithWindowPrescriptionRuntime& runtime) {
    runtime.AddRow(list, 0u);
    const auto name = runtime.Utf8Text(runtime.ReadWord(recipe + 8u));
    const auto name_cell = CreateRequiredCell(list, name, runtime);
    runtime.WriteWord(name_cell + 0x138u, id);
    SetCell(list, 0u, name_cell, runtime);

    if (mode == 0u) {
        const auto equipment = runtime.EquipmentForId(
            runtime.ReadWord(recipe + 0x68u));
        const auto effect = runtime.EffectName(equipment);
        if (effect) {
            const auto effect_cell = runtime.CreateCell(list, *effect);
            SetCell(list, 1u, effect_cell, runtime);
        }

        const auto quantity = SmithWindowItemQuantity::FirstPositive(
            equipment, runtime);
        const auto quantity_cell = CreateRequiredCell(
            list, std::to_string(quantity), runtime);
        runtime.WriteWord(quantity_cell + 0x138u, id);
        SetCell(list, 2u, quantity_cell, runtime);
    } else if (mode == 1u || mode == 2u) {
        const auto offset = mode == 1u ? 0x6cu : 0x70u;
        const auto value = static_cast<std::int32_t>(
            runtime.ReadWord(recipe + offset));
        const auto value_cell = CreateRequiredCell(
            list, std::to_string(value), runtime);
        runtime.WriteWord(value_cell + 0x138u, id);
        SetCell(list, 1u, value_cell, runtime);
    }
}

}  // namespace

std::uint32_t SmithWindow::LoadData(
    const std::uint32_t object, const std::uint32_t mode,
    ISmithWindowPrescriptionRuntime& runtime) {
    const auto list = runtime.ReadWord(object + 0x44u);
    runtime.WriteWord(object + 0xb4u, mode);
    runtime.SetSortDirection(list, 0u);
    runtime.ResetList(list);
    const auto old_columns = runtime.ColumnCount(list);
    for (std::int32_t i = 0; i < old_columns; ++i)
        runtime.RemoveColumn(list, 0u);
    runtime.SetUserColumnSizing(list, false);
    runtime.SetUserColumnDragging(list, false);
    runtime.SetUserSortControl(list, false);
    ConfigureColumns(list, mode, runtime);

    const auto count = runtime.InventoryCount(mode);
    for (std::int32_t index = 0; index < count; ++index) {
        const auto id = runtime.InventoryIdAt(
            mode, static_cast<std::uint32_t>(index));
        if (!runtime.HasRecipe(id)) continue;
        PopulateRow(list, id, runtime.RecipeForId(id), mode, runtime);
    }

    const auto hover = runtime.HoverItem(list);
    std::uint32_t recipe = 0u;
    if (hover != 0u) {
        const auto id = runtime.ReadWord(hover + 0x138u);
        if (runtime.HasRecipe(id)) recipe = runtime.RecipeForId(id);
    }
    UpdatePrescriptionItemName(object, recipe, runtime);
    UpdatePrescriptionItemDescription(object, recipe, runtime);
    return runtime.Refresh(object);
}

}  // namespace pal4
