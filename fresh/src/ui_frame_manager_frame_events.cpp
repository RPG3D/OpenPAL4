#include "pal4/ui_frame_manager_singleton.h"

namespace pal4 {

std::uint32_t UiFrameManagerSingleton::CleanupFrameEvents(
    const std::uint32_t container,
    IUiFrameManagerFrameEventsRuntime& runtime) {
    const auto event_head = runtime.ReadWord(container + 0x10u);
    for (auto node = runtime.ReadWord(event_head); node != event_head;) {
        const auto next = runtime.ReadWord(node);
        runtime.CleanupEventNode(container + 0x0cu, node);
        node = next;
    }
    runtime.Free(runtime.ReadWord(container + 0x10u));
    runtime.WriteWord(container + 0x10u, 0u);
    runtime.WriteWord(container + 0x14u, 0u);

    const auto head = runtime.ReadWord(container + 4u);
    for (auto node = runtime.ReadWord(head); node != head;) {
        const auto next = runtime.ReadWord(node);
        const auto prev = runtime.ReadWord(node + 4u);
        runtime.WriteWord(prev, next);
        runtime.WriteWord(next + 4u, prev);
        runtime.FreeNodePayload(node + 8u, 0u);
        runtime.Free(node);
        runtime.WriteWord(container + 8u,
                          runtime.ReadWord(container + 8u) - 1u);
        node = next;
    }
    runtime.Free(runtime.ReadWord(container + 4u));
    runtime.WriteWord(container + 4u, 0u);
    runtime.WriteWord(container + 8u, 0u);
    return 0u;
}

}  // namespace pal4
