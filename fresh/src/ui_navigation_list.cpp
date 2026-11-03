#include "pal4/ui_navigation_list.h"

namespace pal4 {
namespace {

std::uint32_t CurrentWindow(const std::uint32_t list,
                            IUiNavigationActivationRuntime& runtime) {
    const auto begin = runtime.ReadWord(list + 8u);
    const auto index = runtime.ReadWord(list + 20u);
    return runtime.ReadWord(begin + index * 4u);
}

bool IsUnavailable(const std::uint32_t window,
                   IUiNavigationInputRuntime& runtime) {
    return runtime.IsDisabled(window) ||
           runtime.ReadByte(window + 1202u) != 0u ||
           !runtime.IsVisible(window);
}

}  // namespace

std::uint32_t UiNavigationList::Construct(
    const std::uint32_t list, IUiNavigationConstructionRuntime& runtime) {
    runtime.WriteByte(list + 4u,
                      static_cast<std::uint8_t>((list >> 8u) & 0xffu));
    runtime.WriteWord(list + 8u, 0u);
    runtime.WriteWord(list + 12u, 0u);
    runtime.WriteWord(list + 16u, 0u);
    runtime.WriteWord(list, kVtable);
    runtime.WriteWord(list + 20u, 0u);
    return list;
}

std::uint8_t UiNavigationList::SelectCurrent(
    const std::uint32_t list, IUiNavigationActivationRuntime& runtime) {
    const auto begin = runtime.ReadWord(list + 8u);
    const auto end = runtime.ReadWord(list + 12u);
    for (auto cursor = begin; cursor != end; cursor += 4u)
        runtime.SetButtonSelected(runtime.ReadWord(cursor), false);
    if (begin != 0u && end != begin)
        SetSelected(list, CurrentWindow(list, runtime), runtime);
    return 1u;
}

std::uint8_t UiNavigationList::RedrawAll(
    const std::uint32_t list, IUiNavigationSelectionRuntime& runtime) {
    const auto begin = runtime.ReadWord(list + 8u);
    const auto end = runtime.ReadWord(list + 12u);
    for (auto cursor = begin; cursor != end; cursor += 4u) {
        const auto window = runtime.ReadWord(cursor);
        runtime.WriteByte(window + 1201u, 0u);
        runtime.RequestRedraw(window);
    }
    return 1u;
}

std::uint8_t UiNavigationList::HandleKey(
    const std::uint32_t list, const std::uint32_t event,
    IUiNavigationInputRuntime& runtime) {
    const auto begin = runtime.ReadWord(list + 8u);
    if (begin == 0u || runtime.ReadWord(list + 12u) == begin) return 0u;
    const auto key = runtime.ReadWord(event + 16u);
    if (key == 203u || key == 205u) {
        const auto old_index = runtime.ReadWord(list + 20u);
        (void)runtime.InvokeCycle(list, key == 203u ? 28u : 24u,
                                  old_index);
        SetSelected(list, CurrentWindow(list, runtime), runtime);
        return 1u;
    }
    if (key == 28u) {
        runtime.MouseAtCenter(CurrentWindow(list, runtime));
        return 1u;
    }
    return 0u;
}

void UiNavigationList::SetSelected(
    const std::uint32_t list, const std::uint32_t window,
    IUiNavigationSelectionRuntime& runtime) {
    const auto begin = runtime.ReadWord(list + 8u);
    const auto end = runtime.ReadWord(list + 12u);
    for (auto cursor = begin; cursor != end; cursor += 4u) {
        const auto item = runtime.ReadWord(cursor);
        runtime.WriteByte(item + 1201u, 0u);
        runtime.RequestRedraw(item);
    }
    runtime.WriteByte(window + 1201u, 1u);
    runtime.RequestRedraw(window);
}

std::uint8_t UiNavigationList::CycleNext(
    const std::uint32_t list, const std::uint32_t previous_index,
    IUiNavigationInputRuntime& runtime) {
    auto index = runtime.ReadWord(list + 20u) + 1u;
    runtime.WriteWord(list + 20u, index);
    const auto begin = runtime.ReadWord(list + 8u);
    const auto end = runtime.ReadWord(list + 12u);
    if (begin == 0u || index >= (end - begin) / 4u) {
        index = 0u;
        runtime.WriteWord(list + 20u, index);
    }
    if (index != previous_index &&
        IsUnavailable(CurrentWindow(list, runtime), runtime))
        return static_cast<std::uint8_t>(
            runtime.InvokeCycle(list, 24u, previous_index));
    return static_cast<std::uint8_t>(index);
}

std::uint8_t UiNavigationList::CyclePrevious(
    const std::uint32_t list, const std::uint32_t previous_index,
    IUiNavigationInputRuntime& runtime) {
    auto index = runtime.ReadWord(list + 20u) - 1u;
    runtime.WriteWord(list + 20u, index);
    if (static_cast<std::int32_t>(index) < 0) {
        const auto begin = runtime.ReadWord(list + 8u);
        const auto count = begin != 0u
                               ? (runtime.ReadWord(list + 12u) - begin) / 4u
                               : 0u;
        index = count - 1u;
        runtime.WriteWord(list + 20u, index);
    }
    if (index != previous_index &&
        IsUnavailable(CurrentWindow(list, runtime), runtime))
        return static_cast<std::uint8_t>(
            runtime.InvokeCycle(list, 28u, previous_index));
    return static_cast<std::uint8_t>(index);
}

std::uint32_t UiNavigationList::SetupMouseHandlers(
    const std::uint32_t list, const std::uint32_t window,
    IUiNavigationSetupRuntime& runtime) {
    runtime.AppendWindow(list + 4u, window);
    const auto subscribe = [&](const std::string_view event_name,
                               const std::uint32_t callback) {
        const auto record = runtime.Allocate(0xcu);
        if (record != 0u) {
            runtime.WriteWord(record + 4u, callback);
            runtime.WriteWord(record + 8u, list);
            runtime.WriteWord(record, 0x844100u);
        }
        const auto reference = runtime.Subscribe(window, event_name, record);
        return reference != 0u
                   ? runtime.ReleaseSubscription(reference) : 0u;
    };
    (void)subscribe("MouseMove", 0x4af760u);
    return subscribe("MouseButtonUp", 0x4af770u);
}

std::uint8_t UiNavigationList::OnHoveredAndNotify(
    const std::uint32_t list, const std::uint32_t event,
    IUiNavigationHoverRuntime& runtime) {
    const auto target = runtime.ReadWord(event + 8u);
    if (runtime.IsDisabled(target) ||
        runtime.ReadByte(target + 1202u) != 0u)
        return 1u;

    const auto begin = runtime.ReadWord(list + 8u);
    const auto end = runtime.ReadWord(list + 12u);
    for (auto cursor = begin; cursor != end; cursor += 4u) {
        const auto window = runtime.ReadWord(cursor);
        if (window != target) {
            runtime.WriteByte(window + 1201u, 0u);
            runtime.RequestRedraw(window);
        }
    }
    if (begin != 0u) {
        for (auto cursor = begin; cursor != end; cursor += 4u) {
            if (runtime.ReadWord(cursor) == target) {
                runtime.NotifyHoverHandlers(runtime.UiManager() + 308u);
                break;
            }
        }
    }
    return 1u;
}

std::uint8_t UiNavigationList::OnHoveredAndTrack(
    const std::uint32_t list, const std::uint32_t event,
    IUiNavigationHoverRuntime& runtime) {
    const auto target = runtime.ReadWord(event + 8u);
    if (runtime.IsDisabled(target)) return 1u;
    const auto begin = runtime.ReadWord(list + 8u);
    if (begin == 0u) return 1u;
    const auto end = runtime.ReadWord(list + 12u);
    for (auto cursor = begin; cursor != end; cursor += 4u) {
        const auto window = runtime.ReadWord(cursor);
        if (window == target) {
            runtime.WriteWord(list + 20u, (cursor - begin) / 4u);
        } else {
            runtime.WriteByte(window + 1201u, 0u);
            runtime.RequestRedraw(window);
        }
    }
    return 1u;
}

std::uint8_t UiNavigationList::HandleVerticalKey(
    const std::uint32_t list, const std::uint32_t event,
    IUiNavigationInputRuntime& runtime) {
    const auto begin = runtime.ReadWord(list + 8u);
    if (begin == 0u || runtime.ReadWord(list + 12u) == begin) return 0u;
    const auto key = runtime.ReadWord(event + 16u);
    if (key == 203u || key == 205u) return 0u;
    if (key == 200u || key == 208u) {
        (void)runtime.InvokeCycle(list, key == 200u ? 28u : 24u,
                                  runtime.ReadWord(list + 20u));
        SetSelected(list, CurrentWindow(list, runtime), runtime);
        return 1u;
    }
    return HandleKey(list, event, runtime);
}

std::uint32_t UiNavigationList::ForwardMouseMove(
    const std::uint32_t list, IUiNavigationHoverRuntime& runtime) {
    return runtime.InvokeList(list, 32u);
}

std::uint32_t UiNavigationList::ForwardMouseButtonUp(
    const std::uint32_t list, IUiNavigationHoverRuntime& runtime) {
    return runtime.InvokeList(list, 36u);
}

}  // namespace pal4
