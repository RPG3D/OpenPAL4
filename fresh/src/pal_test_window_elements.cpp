#include "pal4/pal_test_window_elements.h"

#include <string>

namespace pal4 {
namespace {

void Subscribe(std::uint32_t window, std::string_view event,
               std::uint32_t handler, std::uint32_t object_address,
               IPalTestWindowElementsRuntime& runtime) {
    const auto callback = runtime.AllocateCallback(0x18u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 8u, handler);
        runtime.WriteWord(callback + 0xcu, 0u);
        runtime.WriteWord(callback + 0x10u, object_address);
        runtime.WriteWord(callback, PalTestWindowElements::kCallbackVtable);
    }
    std::uint32_t reference{};
    try {
        reference = runtime.Subscribe(window, event, callback);
    } catch (...) {
        if (callback != 0u) runtime.FreeCallback(callback);
        throw;
    }
    if (reference == 0u) return;
    const auto count = runtime.ReferenceCount(reference) - 1u;
    runtime.SetReferenceCount(reference, count);
    if (count == 0u) runtime.DeleteReference(reference, 1u);
}

}  // namespace

void PalTestWindowElements::Initialize(
    PalTestWindowState& state, std::uint32_t object_address,
    IPalTestWindowElementsRuntime& runtime) {
    const auto prefix = runtime.ReadPrefix(state.Read(0x14u));
    const auto close = runtime.FindWindow(prefix + "BtnClose");
    state.Write(0x44u, close);
    Subscribe(close, "Clicked", kCloseHandler, object_address, runtime);

    state.Write(0x40u, runtime.FindWindow(prefix + "EditQuestion"));
    state.Write(0x64u, runtime.FindWindow(prefix + "PanelQuestion"));

    for (std::uint32_t index = 0u; index < 3u; ++index) {
        const auto answer = runtime.FindWindow(
            prefix + "StaticAnswer" + std::to_string(index + 1u));
        state.Write(0x58u + index * 4u, answer);
        for (std::uint32_t colour_state = 0u; colour_state < 4u;
             ++colour_state)
            runtime.SetAnswerTextColour(answer, colour_state, 0xff000000u);
        runtime.WriteWord(answer + 0x118u, index);
        Subscribe(answer, "MouseButtonUp", kAnswerHandler,
                  object_address, runtime);
    }

    const auto ok = runtime.FindWindow(prefix + "BtnOk");
    state.Write(0x48u, ok);
    Subscribe(ok, "Clicked", kOKHandler, object_address, runtime);

    constexpr std::string_view names[]{"BtnHistory", "BtnStory", "BtnWorld"};
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        const auto button = runtime.FindWindow(prefix + std::string(names[index]));
        state.Write(0x4cu + index * 4u, button);
        runtime.WriteWord(button + 0x118u, index);
        Subscribe(button, "Clicked", kCategoryHandler,
                  object_address, runtime);
    }
    for (std::uint32_t index = 0u; index < 3u; ++index)
        runtime.SetupMouseEventHandlers(object_address + 0x28u,
                                         state.Read(0x4cu + index * 4u));
    state.Write(0x88u, object_address);
}

}  // namespace pal4
