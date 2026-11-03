#include "pal4/combat_ending_window_visibility.h"

#include <array>

namespace pal4 {
namespace {
template<class Insert,class Remove>
std::uint32_t UpdateVisibility(
    const std::uint32_t window, const std::uint8_t visible,
    const std::uint32_t parent,
    ICombatEndingWindowVisibilityRuntime& runtime,
    Insert insert,Remove remove) {
    runtime.SetParent(window, visible, parent);
    constexpr std::array static_offsets{84u, 80u, 76u, 72u};
    for (const auto offset : static_offsets) {
        runtime.SetVisible(runtime.ReadWord(window + offset), false);
    }
    for (std::uint32_t i = 0; i < 4u; ++i) {
        runtime.SetVisible(runtime.ReadWord(window + 116u + 76u * i), false);
        runtime.CleanupHint(runtime.ReadWord(window + 684u + 4u * i));
        runtime.PlayStoredSound(window + 700u + 4u * i);
    }
    runtime.WriteWord(window + 436u, 3u);
    const auto handler = window + 40u;
    if (visible != 0u)
        return insert(handler);
    remove(handler);
    return runtime.ResetListFlags(window + 44u);
}
}

std::uint32_t CombatEndingWindowVisibility::Update(
    std::uint32_t window,std::uint8_t visible,std::uint32_t parent,
    ICombatEndingWindowVisibilityRuntime& runtime,IEventHandlerArrayRuntime& array_runtime){
    // Preserve the original manager lookup after all visibility mutations.
    return UpdateVisibility(window,visible,parent,runtime,
        [&](auto handler){return EventHandlerArray::Insert(runtime.FrameManager()+0x134u,handler,array_runtime);},
        [&](auto handler){(void)EventHandlerArray::Remove(runtime.FrameManager()+0x134u,handler,array_runtime);});
}
std::uint32_t CombatEndingWindowVisibility::Update(
    std::uint32_t window,std::uint8_t visible,std::uint32_t parent,
    ICombatEndingWindowVisibilityRuntime& runtime,ICombatEndingWindowActivityRuntime& activity){
    return UpdateVisibility(window,visible,parent,runtime,
        [&](auto handler){return activity.InsertEndingHandler(handler);},
        [&](auto handler){activity.RemoveEndingHandler(handler);});
}

}  // namespace pal4
