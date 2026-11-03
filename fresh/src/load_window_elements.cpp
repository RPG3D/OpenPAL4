#include "pal4/load_window_elements.h"

namespace pal4 {

namespace {

std::uint32_t FindChild(const LoadWindowState& state,
                        std::string_view suffix,
                        ILoadWindowElementsRuntime& runtime) {
    std::string name(runtime.LayoutPrefix(state));
    name.append(suffix);
    return runtime.FindWindow(name);
}

void ReleaseReference(std::uint32_t reference,
                      ILoadWindowElementsRuntime& runtime) {
    if (reference == 0u) return;
    const auto count = runtime.ReferenceCount(reference) - 1u;
    runtime.SetReferenceCount(reference, count);
    if (count == 0u) runtime.DeleteCallback(reference, 1u);
}

void Subscribe(std::uint32_t window, std::uint32_t object_address,
               LoadWindowEvent event, std::uint32_t handler,
               ILoadWindowElementsRuntime& runtime) {
    const auto callback = runtime.AllocateCallback(0x18u);
    if (callback != 0u) {
        runtime.WriteCallbackWord(callback + 0x10u, object_address);
        runtime.WriteCallbackWord(callback + 8u, handler);
        runtime.WriteCallbackWord(callback + 0xcu, 0u);
        runtime.WriteCallbackWord(callback, LoadWindowElements::kCallbackVtable);
    }
    ReleaseReference(runtime.Subscribe(window, event, callback), runtime);
}

void SetWindowText(std::uint32_t window, std::string_view text,
                   ILoadWindowElementsRuntime& runtime) {
    const auto utf8 = runtime.ConvertToUtf8(text);
    const auto nul = utf8.find('\0');
    const auto length = nul == std::string::npos ? utf8.size() : nul;
    const auto cegui_string = runtime.CreateCeguiString(
        std::string_view(utf8.data(), length));
    try {
        runtime.SetText(window, cegui_string);
    } catch (...) {
        runtime.DestroyCeguiString(cegui_string);
        throw;
    }
    runtime.DestroyCeguiString(cegui_string);
}

}  // namespace

std::uint32_t LoadWindowElements::Initialize(
    LoadWindowState& state, std::uint32_t object_address,
    ILoadWindowElementsRuntime& runtime) {
    runtime.InitializeNameBuffer();
    try {
        for (std::uint32_t index = 0u; index < 10u; ++index) {
            const auto suffix = std::string("BtnLoad") +
                                std::to_string(index);
            const auto window = FindChild(state, suffix, runtime);
            state.Write(0x2cu + index * 4u, window);
            runtime.SetWindowUserId(window, index);
            Subscribe(window, object_address, LoadWindowEvent::Clicked,
                      kPopulateHandler, runtime);

            const auto database = runtime.GameDatabase();
            const auto record = runtime.TriggerItem(
                database + 0x34u, index + 205u);
            SetWindowText(window, runtime.ItemText(record), runtime);
        }

        const auto close = FindChild(state, "BtnClose", runtime);
        state.Write(0x58u, close);
        Subscribe(close, object_address, LoadWindowEvent::Clicked,
                  kCloseHandler, runtime);

        const auto list = FindChild(state, "List", runtime);
        state.Write(0x54u, list);
        Subscribe(list, object_address, LoadWindowEvent::MouseClick,
                  kMouseClickHandler, runtime);
        Subscribe(list, object_address, LoadWindowEvent::HoverRowChanged,
                  kHoverHandler, runtime);

        const auto select_mask = FindChild(state, "ImgSelectMask", runtime);
        state.Write(0x5cu, select_mask);
        runtime.SetEnabled(select_mask, false);

        const auto static_mask = FindChild(state, "StaticMask", runtime);
        state.Write(0x60u, static_mask);
        runtime.SetHorizontalFormatting(static_mask, 2u);

        while (runtime.ColumnCount(list) != 0u) {
            runtime.RemoveColumn(list, 0u);
        }
        runtime.SetSortDirection(list, 0u);
        runtime.SetUserColumnSizingEnabled(list, false);
        runtime.SetUserColumnDraggingEnabled(list, false);
        runtime.SetUserSortControlEnabled(list, false);

        const auto header = runtime.CreateCeguiString("");
        try {
            runtime.AddColumn(list, header, 0u, 1.0f);
        } catch (...) {
            runtime.DestroyCeguiString(header);
            throw;
        }
        runtime.DestroyCeguiString(header);
    } catch (...) {
        (void)runtime.CleanupNameBuffer();
        throw;
    }
    return runtime.CleanupNameBuffer();
}

}  // namespace pal4
