#include "pal4/numeric_spinner.h"

#include <limits>
#include <bit>
#include <stdexcept>
#include <string>

namespace pal4 {

std::uint32_t NumericSpinner::Construct(
    const std::uint32_t item, std::string_view name,
    const std::int32_t initial_value, INumericSpinnerRuntime& runtime) {
    runtime.ConstructListboxItem(item, std::to_string(initial_value));
    runtime.WriteByte(item + 444u, runtime.InitialNameByte());
    runtime.WriteWord(item + 448u, 0u);
    runtime.WriteWord(item + 452u, 0u);
    runtime.WriteWord(item + 456u, 0u);
    runtime.WriteWord(item, kVtable);

    name = name.substr(0, name.find('\0'));
    if (name.size() > 0xfffffffdu)
        throw std::length_error("numeric spinner name");
    if (name.empty()) {
        runtime.SetErrorFlag(item + 444u, 1u);
    } else {
        runtime.SetErrorFlag(item + 444u, 1u);
        runtime.ReallocateName(item + 444u,
                               static_cast<std::uint32_t>(name.size()));
        const auto data = runtime.ReadWord(item + 448u);
        runtime.CopyName(data, name);
        runtime.WriteWord(item + 452u,
                          static_cast<std::uint32_t>(name.size()));
        runtime.WriteByte(data + static_cast<std::uint32_t>(name.size()), 0u);
    }

    runtime.WriteWord(item + 440u,
                      static_cast<std::uint32_t>(initial_value));
    runtime.WriteWord(item + 424u, 0u);
    runtime.WriteWord(item + 428u, 0u);
    runtime.WriteWord(item + 436u, 0u);
    runtime.WriteWord(item + 432u, 0u);
    runtime.WriteWord(item + 472u,
                      static_cast<std::uint32_t>(
                          std::numeric_limits<std::int32_t>::max()));
    runtime.WriteWord(item + 468u,
                      static_cast<std::uint32_t>(
                          std::numeric_limits<std::int32_t>::min()));
    runtime.WriteWord(item + 476u, 0u);
    runtime.CreateControls(item);
    return item;
}

void NumericSpinner::NoopThreeArgs(std::uint32_t, std::uint32_t,
                                   std::uint32_t) noexcept {}

std::uint32_t NumericSpinner::CopyPair(
    const std::uint32_t item, const std::uint32_t output,
    INumericSpinnerRuntime& runtime) {
    runtime.WriteWord(output, runtime.ReadWord(item + 460u));
    runtime.WriteWord(output + 4u, runtime.ReadWord(item + 464u));
    return output;
}

void NumericSpinner::Destruct(const std::uint32_t item,
                              INumericSpinnerRuntime& runtime) {
    runtime.WriteWord(item, kVtable);
    const auto root = runtime.ReadWord(item + 436u);
    if (root != 0u) {
        runtime.RemoveChild(root, runtime.ReadWord(item + 424u));
        const auto middle_root = runtime.ReadWord(item + 436u);
        const auto middle = runtime.ReadWord(item + 428u);
        runtime.RemoveChild(middle_root, middle);
        const auto right_root = runtime.ReadWord(item + 436u);
        const auto right = runtime.ReadWord(item + 432u);
        runtime.RemoveChild(right_root, right);
        const auto attached_root = runtime.ReadWord(item + 436u);
        const auto parent = runtime.ReadWord(attached_root + 68u);
        if (parent != 0u) runtime.RemoveChild(parent, attached_root);
        runtime.DestroyWindow(runtime.ReadWord(item + 424u));
        runtime.DestroyWindow(runtime.ReadWord(item + 428u));
        runtime.DestroyWindow(runtime.ReadWord(item + 432u));
        runtime.DestroyWindow(runtime.ReadWord(item + 436u));
        runtime.WriteWord(item + 436u, 0u);
    }

    const auto data = runtime.ReadWord(item + 448u);
    if (data != 0u) {
        const auto count = runtime.ReadByte(data - 1u);
        if (count == 0u || count == 0xffu)
            runtime.Free(data - 1u);
        else
            runtime.WriteByte(data - 1u,
                              static_cast<std::uint8_t>(count - 1u));
    }
    runtime.WriteWord(item + 448u, 0u);
    runtime.WriteWord(item + 452u, 0u);
    runtime.WriteWord(item + 456u, 0u);
    runtime.WriteWord(item, runtime.BaseItemVtable());
    runtime.DestroyString(item + 156u);
    runtime.DestroyString(item + 4u);
}

void NumericSpinner::Layout(const std::uint32_t item,
                            const float compare,
                            const std::array<float, 4>& rect,
                            INumericSpinnerRuntime& runtime) {
    if (runtime.ReadWord(item + 436u) == 0u)
        runtime.CreateControls(item);
    const auto root = runtime.ReadWord(item + 436u);
    if (runtime.ReadWord(root + 68u) == 0u)
        runtime.AddChild(runtime.ReadWord(item + 316u), root);
    runtime.SetRect(root, rect);

    const auto left = runtime.ReadWord(item + 424u);
    const auto middle = runtime.ReadWord(item + 432u);
    const auto right = runtime.ReadWord(item + 428u);
    if (!(rect[0] > compare)) {
        runtime.SetX(left, 0.0f);
        runtime.SetY(left, 0.0f);
        runtime.SetX(middle, runtime.GetWidth(left));
        runtime.SetY(middle, 0.0f);
        const auto middle_width = runtime.GetWidth(middle);
        runtime.SetX(right, runtime.GetWidth(left) + middle_width);
        runtime.SetY(right, 0.0f);
        return;
    }

    const auto parent = runtime.ReadWord(item + 316u);
    const auto offset = static_cast<float>(runtime.FloatToLong(
        runtime.ReadFloat(parent + 1988u)));
    const auto extent = rect[1] - rect[0];
    runtime.SetX(left, 0.0f);
    runtime.SetY(left, extent - runtime.GetHeight(left) - offset);
    runtime.SetX(middle, runtime.GetWidth(left));
    runtime.SetY(middle, extent - runtime.GetHeight(middle) - offset);
    const auto middle_width = runtime.GetWidth(middle);
    runtime.SetX(right, runtime.GetWidth(left) + middle_width);
    runtime.SetY(right, extent - runtime.GetHeight(right) - offset);
}

void NumericSpinner::Reset(const std::uint32_t item,
                           INumericSpinnerRuntime& runtime) {
    runtime.SetEnabled(runtime.ReadWord(item + 424u), true);
    runtime.SetEnabled(runtime.ReadWord(item + 428u), true);
    runtime.SetText(runtime.ReadWord(item + 432u),
                    runtime.ReadCString(0x8dc780u));
    runtime.WriteWord(item + 472u, 0u);
    runtime.WriteWord(item + 468u, 0u);
    runtime.WriteWord(item + 440u, 0u);
}

void NumericSpinner::RemoveFromParent(
    const std::uint32_t item, INumericSpinnerRuntime& runtime) {
    const auto root = runtime.ReadWord(item + 436u);
    if (root == 0u) return;
    const auto parent = runtime.ReadWord(root + 68u);
    if (parent != 0u) runtime.RemoveChild(parent, root);
}

void NumericSpinner::UpdateValue(const std::uint32_t item,
                                 const std::int32_t value,
                                 INumericSpinnerRuntime& runtime) {
    const auto previous = runtime.ReadWord(item + 440u);
    const auto difference = std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(value) - previous);
    const auto direction_button = runtime.ReadWord(
        item + (difference > 0 ? 428u : 424u));
    if (direction_button != 0u &&
        runtime.IsDisabled(direction_button, false)) return;

    const auto maximum = std::bit_cast<std::int32_t>(
        runtime.ReadWord(item + 472u));
    runtime.WriteWord(item + 440u,
                      static_cast<std::uint32_t>(value));
    if (value > maximum)
        runtime.WriteWord(item + 440u,
                          static_cast<std::uint32_t>(maximum));
    const auto minimum = std::bit_cast<std::int32_t>(
        runtime.ReadWord(item + 468u));
    if (std::bit_cast<std::int32_t>(
            runtime.ReadWord(item + 440u)) < minimum)
        runtime.WriteWord(item + 440u,
                          static_cast<std::uint32_t>(minimum));

    const auto result = std::bit_cast<std::int32_t>(
        runtime.ReadWord(item + 440u));
    const auto text = std::to_string(result);
    runtime.SetItemText(item, text);
    const auto text_window = runtime.ReadWord(item + 432u);
    if (text_window != 0u) runtime.SetText(text_window, text);
    const auto left = runtime.ReadWord(item + 424u);
    if (left != 0u) runtime.SetEnabled(left, result != minimum);
    const auto right = runtime.ReadWord(item + 428u);
    if (right != 0u) runtime.SetEnabled(right, result != maximum);
    const auto callback = runtime.ReadWord(item + 476u);
    if (callback != 0u) runtime.InvokeChangeCallback(callback);
    if (runtime.ReadByte(item + 421u) != 0u) {
        const auto disable_right = runtime.ReadWord(item + 428u);
        if (disable_right != 0u) runtime.SetEnabled(disable_right, false);
        const auto disable_left = runtime.ReadWord(item + 424u);
        if (disable_left != 0u) runtime.SetEnabled(disable_left, false);
    }
}

std::uint8_t NumericSpinner::OnLeftClick(
    const std::uint32_t item, INumericSpinnerRuntime& runtime) {
    const auto next = std::bit_cast<std::int32_t>(
        runtime.ReadWord(item + 440u) - 1u);
    UpdateValue(item, next, runtime);
    const auto row = runtime.ReadWord(item + 308u);
    const auto list = runtime.ReadWord(item + 316u);
    runtime.SelectGridCell(list, row, 0u, true);
    return 0u;
}

std::uint8_t NumericSpinner::OnRightClick(
    const std::uint32_t item, INumericSpinnerRuntime& runtime) {
    const auto next = std::bit_cast<std::int32_t>(
        runtime.ReadWord(item + 440u) + 1u);
    UpdateValue(item, next, runtime);
    const auto row = runtime.ReadWord(item + 308u);
    const auto list = runtime.ReadWord(item + 316u);
    runtime.SelectGridCell(list, row, 0u, true);
    return 0u;
}

std::uint8_t NumericSpinner::OnLeftDown(
    const std::uint32_t item, const std::uint32_t event,
    INumericSpinnerRuntime& runtime) {
    if (runtime.ReadWord(event + 28u) == 0u)
        runtime.DispatchUiEvent(0x8b185cu,
                                runtime.ReadWord(item + 308u), 0u);
    return 1u;
}

std::uint8_t NumericSpinner::OnRightDown(
    const std::uint32_t item, const std::uint32_t event,
    INumericSpinnerRuntime& runtime) {
    if (runtime.ReadWord(event + 28u) == 0u)
        runtime.DispatchUiEvent(0x8b1860u,
                                runtime.ReadWord(item + 308u), 0u);
    return 1u;
}

std::uint8_t NumericSpinner::OnButtonUp(
    const std::uint32_t item, INumericSpinnerRuntime& runtime) {
    runtime.DispatchUiEvent(0x8b1864u,
                            runtime.ReadWord(item + 308u), 0u);
    return 1u;
}

}  // namespace pal4
