#include "pal4/smith_window.h"

#include <cstdint>

namespace pal4 {

void SmithWindow::UpdateCategoryPageButtons(
    const std::uint32_t object,
    ISmithWindowCategoryPageRuntime& runtime) {
    const auto begin = runtime.ReadWord(object + 0xccu);
    bool visible = false;
    if (begin != 0u &&
        ((runtime.ReadWord(object + 0xd0u) - begin) >> 2u) != 0u) {
        const auto index = runtime.ReadWord(object + 0xb0u);
        runtime.UpdateEquipmentList(object,
                                    runtime.ReadWord(begin + index * 4u));
        const auto current_begin = runtime.ReadWord(object + 0xccu);
        if (current_begin != 0u)
            visible = ((runtime.ReadWord(object + 0xd0u) -
                        current_begin) >> 2u) > 1u;
    }
    runtime.SetVisible(runtime.ReadWord(object + 0x58u), visible);
    runtime.SetVisible(runtime.ReadWord(object + 0x5cu), visible);
}

}  // namespace pal4
