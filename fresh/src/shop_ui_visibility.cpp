#include "pal4/shop_ui_visibility.h"

namespace pal4 {

void ShopUiVisibility::SetVisible(const std::uint32_t frame,
                                  const std::uint8_t visible,
                                  const std::uint32_t parent,
                                  IShopUiVisibilityRuntime& runtime) {
    if (visible != 0u) runtime.Update(frame);
    runtime.SetParent(frame, visible, parent);

    const auto item = frame == 0u ? 0u : frame + 0x28u;
    const auto manager = runtime.FrameManager();
    if (visible != 0u) {
        runtime.InsertActive(manager + 0x134u, item);
        runtime.UpdateSystemDialogVisibility(runtime.FrameManager(), 8u);
    } else {
        runtime.RemoveActive(manager + 0x134u, item);
    }
}

}  // namespace pal4
