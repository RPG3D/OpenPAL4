#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>

namespace pal4 {

std::uint8_t UiFrameManagerSingleton::Destruct(
    const std::uint32_t manager,
    IUiFrameManagerDestructRuntime& runtime) {
    const auto header = runtime.ReadWord(manager + 4u);
    const auto nil = runtime.NilNode();
    auto node = runtime.ReadWord(header);
    while (node != header) {
        runtime.DestroyCursor(runtime.ReadWord(node + 28u));
        const auto right = runtime.ReadWord(node + 8u);
        if (right != nil) {
            node = runtime.FindMaxNode(right);
        } else {
            auto parent = runtime.ReadWord(node + 4u);
            while (node == runtime.ReadWord(parent + 8u)) {
                node = parent;
                parent = runtime.ReadWord(parent + 4u);
            }
            if (runtime.ReadWord(node + 8u) != parent)
                node = parent;
        }
    }

    if (runtime.ReadWord(manager + 12u) != 0u) {
        runtime.FreeNodes(manager, runtime.ReadWord(header + 4u));
        runtime.WriteWord(header + 4u, nil);
        runtime.WriteWord(manager + 12u, 0u);
        runtime.WriteWord(header, header);
        runtime.WriteWord(header + 8u, header);
        runtime.FirstElement(manager);
    } else {
        node = runtime.ReadWord(header);
        while (node != header) {
            const auto next = runtime.NextNode(node);
            runtime.DeleteNode(manager, node);
            node = next;
        }
    }

    const auto name_record = manager + 16u;
    const auto pointer = runtime.ReadWord(name_record + 4u);
    if (pointer != 0u) {
        const auto reference = runtime.ReadByte(pointer - 1u);
        if (reference != 0u && reference != 0xffu) {
            runtime.WriteByte(pointer - 1u,
                              static_cast<std::uint8_t>(
                                  reference - 1u));
            return runtime.SetErrorFlag(name_record, 0u);
        }
    }
    return runtime.SetErrorFlag(name_record, 1u);
}

}  // namespace pal4
