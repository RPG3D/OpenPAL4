#include "pal4/trade_window_input.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kKeyEscape = 1u;
constexpr std::uint32_t kKeyUp = 200u;
constexpr std::uint32_t kKeyDown = 208u;
constexpr std::uint32_t kKeyLeft = 203u;
constexpr std::uint32_t kKeyRight = 205u;
constexpr std::uint32_t kKeyEnter = 28u;

std::uint32_t Frame(const std::uint32_t input,
                    ITradeWindowInputRuntime& runtime) {
    return runtime.ReadWord(input + 4u);
}

std::uint32_t List(const std::uint32_t input,
                   ITradeWindowInputRuntime& runtime) {
    return runtime.ReadWord(Frame(input, runtime) + 0x28u);
}

}  // namespace

void TradeWindowInput::SetItemGlow(const std::uint32_t input,
                                   const bool glow,
                                   ITradeWindowInputRuntime& runtime) {
    const auto frame = Frame(input, runtime);
    const auto left = runtime.ReadWord(frame + 0x44u);
    const auto right = runtime.ReadWord(frame + 0x48u);
    runtime.WriteByte(input + 8u, glow ? 1u : 0u);
    if (!runtime.IsVisible(left, false)) {
        runtime.WriteByte(input + 8u, 0u);
    }
    if (glow) {
        if (runtime.IsVisible(right, false)) {
            runtime.WriteByte(right + 0x4b1u, 1u);
            runtime.RequestRedraw(right);
        }
    } else {
        runtime.WriteByte(right + 0x4b1u, 0u);
        runtime.RequestRedraw(right);
        runtime.WriteByte(left + 0x4b1u, 0u);
        runtime.RequestRedraw(left);
    }
}

bool TradeWindowInput::OnStatusChanged(const std::uint32_t input,
                                       ITradeWindowInputRuntime& runtime) {
    SetItemGlow(input, false, runtime);
    return true;
}

bool TradeWindowInput::OnKeyboardInput(const std::uint32_t input,
                                       const std::uint32_t event,
                                       ITradeWindowInputRuntime& runtime) {
    const auto key = runtime.ReadWord(event + 0x10u);
    if (key == kKeyEscape) {
        runtime.CloseTradeWindow(Frame(input, runtime));
        return true;
    }
    if (key == kKeyEnter) {
        runtime.ShowTradePrompt(Frame(input, runtime));
        return true;
    }
    if (key == kKeyUp || key == kKeyDown) {
        const auto glow = runtime.ReadByte(input + 8u) != 0u;
        if (key == kKeyUp && glow) return true;
        const auto list = List(input, runtime);
        const auto rows = runtime.RowCount(list);
        if (key == kKeyDown && glow) {
            if (rows != 0) {
                SetItemGlow(input, false, runtime);
                runtime.SetHoverRowIndex(list, 0);
            }
            return true;
        }
        if (rows == 0 || runtime.HoverItem(list) == 0u) return true;
        const auto row = runtime.ReadWord(list + 0x4c0u);
        if (key == kKeyUp) {
            if (row != 0u) {
                runtime.UpHoverItem(list);
            } else {
                SetItemGlow(input, true, runtime);
            }
        } else if (row < static_cast<std::uint32_t>(
                              runtime.RowCount(list) - 1)) {
            runtime.DownHoverItem(list);
        }
        return true;
    }
    if (key == kKeyLeft || key == kKeyRight) {
        if (runtime.ReadByte(input + 8u) != 0u) {
            const auto frame = Frame(input, runtime);
            const auto pressed = runtime.ReadWord(frame +
                (key == kKeyLeft ? 0x48u : 0x44u));
            const auto target = runtime.ReadWord(frame +
                (key == kKeyLeft ? 0x44u : 0x48u));
            runtime.WriteByte(pressed + 0x4b1u, 0u);
            runtime.RequestRedraw(pressed);
            runtime.HandleMouseEvent(target);
        }
        return true;
    }
    return false;
}

bool TradeWindowInput::OnQuantityKeyInput(const std::uint32_t input,
                                    const std::uint32_t event,
                                    ITradeWindowInputRuntime& runtime) {
    const auto key = runtime.ReadWord(event + 0x10u);
    if ((key == kKeyLeft || key == kKeyRight) &&
        runtime.ReadByte(input + 8u) == 0u) {
        const auto list = List(input, runtime);
        const auto hovered = runtime.HoverItem(list);
        if (hovered != 0u) {
            const auto row = runtime.HoverRowOfItem(list, hovered);
            const auto item = runtime.GridItem(list, row, 1);
            const auto value = runtime.SpinnerValue(item);
            runtime.SetSpinnerValue(item, value +
                (key == kKeyLeft ? -1 : 1));
            runtime.RequestRedraw(list);
        }
        return true;
    }
    return OnKeyboardInput(input, event, runtime);
}

}  // namespace pal4
