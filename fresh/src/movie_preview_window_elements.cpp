#include "pal4/movie_preview_window_elements.h"

namespace pal4 {
namespace {

void Subscribe(std::uint32_t window, std::string_view event,
               std::uint32_t handler, std::uint32_t object_address,
               IMoviePreviewWindowElementsRuntime& runtime) {
    const auto callback = runtime.AllocateCallback(0x18u);
    if (callback != 0u) {
        runtime.WriteCallbackWord(callback + 0x10u, object_address);
        runtime.WriteCallbackWord(callback + 8u, handler);
        runtime.WriteCallbackWord(callback + 0xcu, 0u);
        runtime.WriteCallbackWord(callback,
                                  MoviePreviewWindowElements::kCallbackVtable);
    }
    const auto reference = runtime.Subscribe(window, event, callback);
    if (reference == 0u) return;
    const auto count = runtime.ReferenceCount(reference) - 1u;
    runtime.SetReferenceCount(reference, count);
    if (count == 0u) runtime.DeleteReference(reference, 1u);
}

}  // namespace

void MoviePreviewWindowElements::Initialize(
    MoviePreviewWindowState& state, std::uint32_t object_address,
    IMoviePreviewWindowElementsRuntime& runtime) {
    const auto prefix = runtime.ReadPrefix(state.Read(0x18u));
    for (std::uint32_t i = 0u; i < 13u; ++i) {
        const auto image = runtime.FindWindow(
            prefix + "Img" + std::to_string(i + 1u));
        state.Write(0x2cu + i * 4u, image);
        runtime.SetAlwaysOnTop(image, true);
        Subscribe(image, "MouseMove", kMouseMoveHandler,
                  object_address, runtime);
        Subscribe(image, "MouseClick", kMouseClickHandler,
                  object_address, runtime);
    }

    state.Write(0x60u, runtime.FindWindow(prefix + "StaticInfo"));
    const auto close = runtime.FindWindow(prefix + "BtnClose");
    state.Write(0x64u, close);
    Subscribe(close, "Clicked", kCloseHandler, object_address, runtime);
    const auto mask = runtime.FindWindow(prefix + "ImgSelectMask");
    state.Write(0x68u, mask);
    runtime.SetVisible(mask, false);
    state.Write(0x6cu, runtime.ReadWord(state.Read(0x2cu) + 0x6b0u));
}

}  // namespace pal4
