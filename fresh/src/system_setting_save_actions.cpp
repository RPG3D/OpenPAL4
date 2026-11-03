#include "pal4/system_setting_save_actions.h"

namespace pal4 {
namespace {

void StoreSelection(const std::uint32_t frame, const std::int32_t slot,
                    ISystemSettingSaveResultRuntime& runtime) {
    runtime.WriteWord(runtime.GlobalData() + 44u,
                      static_cast<std::uint32_t>(slot));
    const auto list = runtime.ReadWord(frame + 104u);
    const auto scrollbar = runtime.ReadWord(list + 1204u);
    const auto position = runtime.ReadWord(scrollbar + 1216u);
    runtime.WriteWord(runtime.GlobalData() + 48u, position);
    runtime.SaveSettings(runtime.GlobalData());
}

}  // namespace

std::uint8_t SystemSettingSaveActions::OnClick(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemSettingSaveResultRuntime& runtime) {
    if (event == 1u) {
        const auto item = runtime.HoverItem(runtime.ReadWord(frame + 104u));
        if (item != 0u) {
            const auto slot = static_cast<std::int32_t>(
                runtime.ReadWord(item + 440u));
            StoreSelection(frame, slot, runtime);
            runtime.InvokeSlotHelper(slot);
            runtime.PopulateSaveList(frame, slot / 10, 1u);
        }
    }
    return 1u;
}

std::uint8_t SystemSettingSaveActions::OnDoubleClick(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemSettingSaveResultRuntime& runtime) {
    if (event == 1u) {
        const auto item = runtime.HoverItem(runtime.ReadWord(frame + 104u));
        if (item != 0u && runtime.ReadWord(item + 492u) != 0u) {
            const auto slot = static_cast<std::int32_t>(
                runtime.ReadWord(item + 440u));
            StoreSelection(frame, slot, runtime);
            const auto node = runtime.CreateXmlNode();
            try {
                runtime.SetIntAttribute(node, "PALGAMEIV_LOADING_TYPE", 1);
                runtime.SetIntAttribute(node, "PALGAMEIV_LOADING_ARCHIVE", slot);
                runtime.ProcessUiEvent(runtime.FrameManager() + 260u,
                                       0x8a80b4u, 0u, 0u);
                runtime.InitializeGameState(runtime.Game(), 6u, node);
            } catch (...) {
                runtime.DestroyXmlNode(node);
                throw;
            }
            runtime.DestroyXmlNode(node);
        }
    }
    return 1u;
}

std::uint8_t SystemSettingSaveActions::OnChangePage(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemSettingSavePageRuntime& runtime) {
    const auto source = runtime.ReadWord(event + 8u);
    const auto page = static_cast<std::int32_t>(runtime.ReadWord(source + 280u));
    runtime.PopulateSaveList(frame, page, 0u);
    return 1u;
}

std::uint8_t SystemSettingSaveActions::OnContextMenuEvent(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemSettingSaveContextEventRuntime& runtime) {
    (void)event;
    (void)runtime.ShowSaveContextMenu(frame);
    return 1u;
}

std::uint32_t SystemSettingSaveActions::ShowContextMenu(
    const std::uint32_t frame, ISystemSettingSaveContextRuntime& runtime) {
    const auto item = runtime.HoverItem(runtime.ReadWord(frame + 104u));
    if (item == 0u) {
        return 0u;
    }
    const auto mode = runtime.ReadWord(frame + 504u);
    const auto populated = runtime.ReadWord(item + 492u) != 0u;
    std::uint32_t function = 0u;
    std::uint32_t trigger = 0u;
    if (mode == 1u) {
        function = 0x4816b0u;
        trigger = populated ? 109u : 231u;
    } else if (mode == 2u && populated) {
        function = 0x481740u;
        trigger = 229u;
    } else {
        return item;
    }
    const auto allocation = runtime.Allocate(0x18u);
    std::uint32_t callback = 0u;
    if (allocation != 0u) {
        if (mode == 1u) {
            callback = runtime.ConstructCallback(allocation, function, frame);
        } else {
            callback = allocation;
            runtime.WriteWord(callback + 8u, function);
            runtime.WriteWord(callback + 12u, 0u);
            runtime.WriteWord(callback + 16u, frame);
            runtime.WriteWord(callback, 0x843c08u);
        }
    }
    const auto text = runtime.TriggerText(trigger);
    runtime.CreateCommonDialog(runtime.FrameManager());
    std::uint32_t result = 0u;
    try {
        result = runtime.ShowDialog(text, 1050253722u,
                                    1053609165u, 3u, callback);
    } catch (...) {
        if (callback != 0u) {
            (void)runtime.ReleaseCallback(callback);
        }
        throw;
    }
    return callback != 0u ? runtime.ReleaseCallback(callback) : result;
}

std::uint8_t SystemSettingSaveActions::OnHover(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemSettingSaveActionsRuntime& runtime) {
    (void)event;
    (void)runtime.HoverItem(runtime.ReadWord(frame + 104u));
    return 1u;
}

std::uint8_t SystemSettingSaveActions::OnKey(
    const std::uint32_t member, const std::uint32_t event,
    ISystemSettingSaveNavigationRuntime& runtime) {
    if (runtime.ReadWord(event + 16u) == 1u) {
        runtime.RemoveActive(runtime.FrameManager() + 308u, member);
        runtime.ShowSystemFrame(runtime.ReadWord(member + 4u));
        return 1u;
    }
    const auto frame = runtime.ReadWord(member + 4u);
    const auto list = runtime.ReadWord(frame + 104u);
    if (runtime.RowCount(list) == 0u) {
        return 0u;
    }
    switch (runtime.ReadWord(event + 16u)) {
    case 200u:
        if (runtime.HoverItem(list) != 0u &&
            static_cast<std::int32_t>(runtime.ReadWord(list + 1216u)) > 0) {
            runtime.UpHoverItem(list);
        }
        break;
    case 208u:
        if (runtime.HoverItem(list) != 0u &&
            runtime.ReadWord(list + 1216u) < runtime.RowCount(list) - 1u) {
            runtime.DownHoverItem(list);
        }
        break;
    case 203u: {
        const auto mask = runtime.ReadWord(frame + 176u);
        const auto page = static_cast<std::int32_t>(
            runtime.ReadWord(mask + 280u));
        if (page > 0) {
            runtime.PopulateSaveList(frame, page - 1, 0u);
        }
        break;
    }
    case 205u: {
        const auto mask = runtime.ReadWord(frame + 176u);
        const auto page = static_cast<std::int32_t>(
            runtime.ReadWord(mask + 280u));
        if (page < 9) {
            runtime.PopulateSaveList(frame, page + 1, 0u);
        }
        break;
    }
    case 28u:
        (void)runtime.ShowSaveContextMenu(frame);
        break;
    default:
        break;
    }
    return 1u;
}

std::uint32_t ISystemSettingSaveActionsRuntime::ShowSaveContextMenu(std::uint32_t frame) {
    return SystemSettingSaveActions::ShowContextMenu(frame, *this);
}

}  // namespace pal4
