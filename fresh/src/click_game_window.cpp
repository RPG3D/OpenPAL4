#include "pal4/click_game_window.h"

#include <string>

namespace pal4 {
namespace {

void Subscribe(const std::uint32_t object,
               const ClickGameWindowEvent event,
               const std::uint32_t handler,
               IClickGameWindowRuntime& runtime) {
    const auto callback = runtime.Allocate(0xcu);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, handler);
        runtime.WriteWord(callback + 8u, object);
        runtime.WriteWord(callback, 0x843e18u);
    }
    const auto subscription = runtime.Subscribe(
        runtime.ReadWord(object + 0x24u), event, callback);
    if (subscription != 0u)
        runtime.ReleaseSubscription(subscription);
}

}  // namespace

std::uint32_t ClickGameWindow::Construct(
    const std::uint32_t object, const std::uint32_t layout_name,
    IClickGameWindowRuntime& runtime) {
    runtime.LoadLayout(object, layout_name);
    runtime.WriteWord(object, 0x843e20u);
    runtime.WriteWord(object + 0x30u, 0u);

    std::string image_name(runtime.WindowName(object));
    image_name += "ImgBar";
    const auto image_bar = runtime.GetWindow(object, image_name);
    runtime.WriteWord(object + 0x28u, image_bar);
    runtime.WriteFloat(object + 0x2cu, runtime.GetWidth(image_bar));

    Subscribe(object, ClickGameWindowEvent::KeyUp, 0x49f6d0u, runtime);
    Subscribe(object, ClickGameWindowEvent::MouseClick,
              0x49f6a0u, runtime);
    return object;
}

}  // namespace pal4
