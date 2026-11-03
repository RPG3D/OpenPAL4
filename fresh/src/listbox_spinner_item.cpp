#include "pal4/listbox_spinner_item.h"

#include <bit>

namespace pal4 {

std::uint32_t ListboxSpinnerItem::Construct(
    const std::uint32_t item, const std::int32_t value,
    IListboxSpinnerItemRuntime& runtime) {
    runtime.ConstructBase(item, runtime.ReadCString(0x8dc780u));
    runtime.WriteWord(item + 428u, 0u);
    runtime.WriteWord(item + 432u, 0u);
    runtime.WriteWord(item, kVtable);
    const auto spinner = runtime.CreateOrGetSpinner(value);
    runtime.WriteWord(item + 424u, spinner);
    runtime.ResetSpinner(spinner);
    return item;
}

void ListboxSpinnerItem::Destruct(
    const std::uint32_t item, IListboxSpinnerItemRuntime& runtime) {
    runtime.WriteWord(item, kVtable);
    runtime.RemoveSpinner(runtime.ReadWord(item + 424u));
    runtime.WriteWord(item, runtime.BaseItemVtable());
    runtime.DestroyString(item + 156u);
    runtime.DestroyString(item + 4u);
}

std::uint32_t ListboxSpinnerItem::PrepareAndCallUpdate(
    const std::uint32_t item, IListboxSpinnerItemRuntime& runtime) {
    const auto spinner = runtime.ReadWord(item + 424u);
    runtime.WriteWord(spinner + 308u, runtime.ReadWord(item + 308u));
    runtime.WriteWord(spinner + 316u, runtime.ReadWord(item + 316u));
    return runtime.CallVirtual(spinner, 12u);
}

std::uint32_t ListboxSpinnerItem::CallUpdate(
    const std::uint32_t item, IListboxSpinnerItemRuntime& runtime) {
    return runtime.CallVirtual(runtime.ReadWord(item + 424u), 20u);
}

std::int32_t ListboxSpinnerItem::GetCurrentValue(
    const std::uint32_t item, IListboxSpinnerItemRuntime& runtime) {
    return std::bit_cast<std::int32_t>(
        runtime.ReadWord(runtime.ReadWord(item + 424u) + 440u));
}

void ListboxSpinnerItem::SetCurrentValue(
    const std::uint32_t item, const std::int32_t value,
    IListboxSpinnerItemRuntime& runtime) {
    runtime.UpdateSpinnerValue(runtime.ReadWord(item + 424u), value);
}

void ListboxSpinnerItem::SetRange(
    const std::uint32_t item, const std::int32_t minimum,
    const std::int32_t maximum, IListboxSpinnerItemRuntime& runtime) {
    const auto spinner = runtime.ReadWord(item + 424u);
    runtime.WriteWord(spinner + 472u,
                      static_cast<std::uint32_t>(maximum));
    const auto current = std::bit_cast<std::int32_t>(
        runtime.ReadWord(spinner + 440u));
    runtime.WriteWord(spinner + 468u,
                      static_cast<std::uint32_t>(minimum));
    runtime.UpdateSpinnerValue(spinner, current);
}

std::uint32_t ListboxSpinnerItem::SetChangeCallback(
    const std::uint32_t item, const std::uint32_t first,
    const std::uint32_t second, IListboxSpinnerItemRuntime& runtime) {
    runtime.WriteWord(item + 432u, second);
    runtime.WriteWord(item + 428u, first);
    runtime.WriteWord(runtime.ReadWord(item + 424u) + 476u,
                      item + 428u);
    return item + 428u;
}

}  // namespace pal4
