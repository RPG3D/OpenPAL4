#include "pal4/equipment_control_visibility.h"

#include <cstdint>
#include <initializer_list>

namespace pal4 {

void EquipmentControlVisibility::SetVisible(
    EquipmentWindowState& state, bool visible,
    IEquipmentControlVisibilityRuntime& runtime) {
    for (const auto offset : {0xbcu, 0xb8u, 0xc0u, 0xc4u, 0xb4u})
        runtime.SetVisible(state.Read(offset), visible);

    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto window = state.Read(0xe0u + index * 4u);
        runtime.SetVisible(window, false);
        runtime.SetImage(window, runtime.IconImage(index, 0u, 0u));
    }

    if (visible) {
        const auto category = state.Read(0xf0u);
        runtime.SetImage(state.Read(0xb0u),
                         state.Read(category >= 5u ? 0xdcu :
                                    0xc8u + category * 4u));
    } else {
        runtime.SetImage(state.Read(0xb0u),
                         runtime.GetImage("zi0", "zhuangbei"));
    }
    runtime.SetModelVisible(state, visible);
}

}  // namespace pal4
