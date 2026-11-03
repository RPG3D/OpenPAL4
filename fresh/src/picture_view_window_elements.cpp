#include "pal4/picture_view_window_elements.h"

namespace pal4 {
namespace {

void Subscribe(std::uint32_t window, std::uint32_t object_address,
               std::uint32_t handler,
               IPictureViewWindowElementsRuntime& runtime) {
    const auto callback = runtime.AllocateCallback(0x18u);
    if (callback != 0u) {
        runtime.WriteCallbackWord(callback + 8u, handler);
        runtime.WriteCallbackWord(callback + 0xcu, 0u);
        runtime.WriteCallbackWord(callback + 0x10u, object_address);
        runtime.WriteCallbackWord(
            callback, PictureViewWindowElements::kCallbackVtable);
    }
    const auto reference = runtime.SubscribeClicked(window, callback);
    if (reference == 0u) return;
    const auto count = runtime.ReferenceCount(reference) - 1u;
    runtime.SetReferenceCount(reference, count);
    if (count == 0u) runtime.DeleteReference(reference, 1u);
}

}  // namespace

void PictureViewWindowElements::Initialize(
    PictureViewWindowState& state, std::uint32_t object_address,
    IPictureViewWindowElementsRuntime& runtime) {
    const auto close = runtime.FindWindow("PictureViewWindow/BtnClose");
    state.Write(0x40u, close);
    Subscribe(close, object_address, 0x44ad50u, runtime);

    const auto image = runtime.FindWindow("PictureViewWindow/Image");
    state.Write(0x4cu, image);
    runtime.SetImageFormatting(image, 2u, 2u);

    const auto left = runtime.FindWindow("PictureViewWindow/BtnLeft");
    state.Write(0x44u, left);
    Subscribe(left, object_address, 0x44aeb0u, runtime);

    const auto right = runtime.FindWindow("PictureViewWindow/BtnRight");
    state.Write(0x48u, right);
    Subscribe(right, object_address, 0x44aef0u, runtime);

    runtime.BindMouseEvents(object_address + 0x28u, left);
    runtime.BindMouseEvents(object_address + 0x28u, right);
}

}  // namespace pal4
