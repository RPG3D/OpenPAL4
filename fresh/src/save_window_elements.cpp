#include "pal4/save_window_elements.h"

#include "pal4/preview_text_codec.h"

namespace pal4 {
namespace {

void Subscribe(std::uint32_t window, std::string_view event,
               std::uint32_t handler, std::uint32_t object_address,
               ISaveWindowElementsRuntime& runtime) {
    const auto callback = runtime.AllocateCallback(0x18u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 0x10u, object_address);
        runtime.WriteWord(callback + 8u, handler);
        runtime.WriteWord(callback + 0xcu, 0u);
        runtime.WriteWord(callback, SaveWindowElements::kCallbackVtable);
    }
    const auto reference = runtime.Subscribe(window, event, callback);
    if (reference == 0u) return;
    const auto count = runtime.ReferenceCount(reference) - 1u;
    runtime.SetReferenceCount(reference, count);
    if (count == 0u) runtime.DeleteReference(reference, 1u);
}

}  // namespace

void SaveWindowElements::Initialize(
    SaveWindowState& state, std::uint32_t object_address,
    ISaveWindowElementsRuntime& runtime) {
    const auto prefix = runtime.ReadPrefix(state.Read(0x18u));
    for (std::uint32_t i = 0u; i < 10u; ++i) {
        const auto button = runtime.FindWindow(
            prefix + "BtnLoad" + std::to_string(i));
        state.Write(0x2cu + i * 4u, button);
        runtime.WriteWord(button + 0x118u, i);
        Subscribe(button, "Clicked", kSaveClickHandler,
                  object_address, runtime);
        const auto raw = runtime.TriggerItemText(i + 205u);
        runtime.SetText(button,
                        PreviewTextCodec::DecodeUtf8(runtime.ConvertToUtf8(raw)));
    }

    const auto close = runtime.FindWindow(prefix + "BtnClose");
    state.Write(0x58u, close);
    Subscribe(close, "Clicked", kCloseHandler, object_address, runtime);

    const auto list = runtime.FindWindow(prefix + "List");
    state.Write(0x54u, list);
    Subscribe(list, "MouseClick", kListClickHandler,
              object_address, runtime);
    Subscribe(list, "HoverRowChanged", kHoverHandler,
              object_address, runtime);

    const auto select_mask = runtime.FindWindow(prefix + "ImgSelectMask");
    state.Write(0x5cu, select_mask);
    runtime.SetEnabled(select_mask, false);
    const auto static_mask = runtime.FindWindow(prefix + "StaticMask");
    state.Write(0x60u, static_mask);
    runtime.SetHorizontalFormatting(static_mask, 2u);

    while (runtime.ColumnCount(list) != 0u)
        runtime.RemoveColumn(list, 0u);
    runtime.SetSortDirection(list, 0u);
    runtime.SetUserColumnSizingEnabled(list, false);
    runtime.SetUserColumnDraggingEnabled(list, false);
    runtime.SetUserSortControlEnabled(list, false);
    runtime.AddColumn(list, U"", 0u, 1.0f);
}

}  // namespace pal4
