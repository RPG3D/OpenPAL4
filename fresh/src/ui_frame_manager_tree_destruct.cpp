#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>

namespace pal4 {
namespace {

std::uint32_t ReleaseTree(const std::uint32_t tree,
                          const bool refresh_first,
                          IUiFrameManagerTreeDestructRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto iterator = runtime.ReadWord(head);
    if (runtime.ReadWord(tree + 12u) != 0u) {
        runtime.FreeNodes(tree, runtime.ReadWord(head + 4u));
        runtime.WriteWord(head + 4u, runtime.SharedNil());
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        if (refresh_first) runtime.FirstElement(tree);
    } else {
        while (iterator != head) {
            const auto node = runtime.NextNode(iterator);
            runtime.DeleteNode(tree, node);
        }
    }
    (void)runtime.Free(head);
    runtime.WriteWord(tree + 4u, 0u);
    runtime.WriteWord(tree + 12u, 0u);
    const auto users = runtime.SharedNilUsers() - 1u;
    runtime.SetSharedNilUsers(users);
    if (users != 0u) return users;
    const auto nil = runtime.SharedNil();
    runtime.SetSharedNil(0u);
    if (nil != 0u) return runtime.Free(nil);
    return 0u;
}

}  // namespace

std::uint32_t UiFrameManagerSingleton::DestructEmbeddedTree(
    const std::uint32_t tree,
    IUiFrameManagerTreeDestructRuntime& runtime) {
    const auto name = runtime.ReadWord(tree + 20u);
    if (name != 0u) {
        const auto header = name - 1u;
        const auto reference = runtime.ReadByte(header);
        if (reference == 0u || reference == 0xffu)
            (void)runtime.Free(header);
        else
            runtime.WriteByte(header,
                              static_cast<std::uint8_t>(reference - 1u));
    }
    for (std::uint32_t offset = 20u; offset <= 28u; offset += 4u)
        runtime.WriteWord(tree + offset, 0u);
    return ReleaseTree(tree, false, runtime);
}

std::uint32_t UiFrameManagerSingleton::DestructEventTree(
    const std::uint32_t tree,
    IUiFrameManagerTreeDestructRuntime& runtime) {
    return ReleaseTree(tree, true, runtime);
}

std::uint32_t UiFrameManagerSingleton::DestructAuxiliaryTree(
    const std::uint32_t tree,
    IUiFrameManagerTreeDestructRuntime& runtime) {
    return ReleaseTree(tree, false, runtime);
}

}  // namespace pal4
