#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>

namespace pal4 {

std::uint32_t UiFrameManagerSingleton::DestructManager(
    const std::uint32_t manager,
    IUiFrameManagerFullDestructRuntime& runtime) {
    runtime.WriteWord(manager, 0x8443a4u);
    const auto sound = runtime.ReadWord(manager + 0x1a4u);
    if (sound != 0u) {
        runtime.DestructSound(sound + 4u);
        (void)runtime.Free(sound);
    }
    runtime.WriteWord(manager + 0x1a4u, 0u);
    runtime.DestructLateStruct(manager + 0x1dcu);

    (void)runtime.Free(runtime.ReadWord(manager + 0x18cu));
    for (std::uint32_t offset = 0x18cu; offset <= 0x194u;
         offset += 4u)
        runtime.WriteWord(manager + offset, 0u);
    runtime.DestructXml(manager + 0x14cu);
    runtime.DestructEffects(manager);

    (void)runtime.Free(runtime.ReadWord(manager + 0x138u));
    for (std::uint32_t offset = 0x138u; offset <= 0x140u;
         offset += 4u)
        runtime.WriteWord(manager + offset, 0u);

    const auto tree = manager + 0x114u;
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

    const auto head = runtime.ReadWord(tree + 4u);
    auto iterator = runtime.ReadWord(head);
    const bool bulk = runtime.ReadWord(tree + 12u) != 0u &&
                      iterator == runtime.GetFirstElement(tree) &&
                      head == runtime.ReadWord(tree + 4u);
    if (bulk) {
        runtime.FreeNodes(tree, runtime.ReadWord(head + 4u));
        runtime.WriteWord(head + 4u, runtime.SharedNil());
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        (void)runtime.GetFirstElement(tree);
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
    if (users == 0u) {
        const auto nil = runtime.SharedNil();
        runtime.SetSharedNil(0u);
        if (nil != 0u) (void)runtime.Free(nil);
    }
    return runtime.DestructEventTree(manager + 0x104u);
}

}  // namespace pal4
