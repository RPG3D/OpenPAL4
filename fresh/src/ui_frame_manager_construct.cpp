#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>

namespace pal4 {

std::uint32_t UiFrameManagerSingleton::Construct(
    const std::uint32_t manager, const std::uint8_t seed,
    IUiFrameManagerConstructRuntime& runtime) {
    runtime.ConstructEventTree(manager + 0x104u);
    runtime.ConstructTree(manager + 0x114u, seed);
    runtime.WriteByte(manager + 0x124u, seed);
    runtime.SetErrorFlag(manager + 0x124u, 0u);
    runtime.WriteByte(manager + 0x134u, seed);
    runtime.WriteWord(manager + 0x138u, 0u);
    runtime.WriteWord(manager + 0x13cu, 0u);
    runtime.WriteWord(manager + 0x140u, 0u);
    runtime.InitializeEffects(manager);
    runtime.WriteWord(manager + 0x144u, 0u);
    runtime.WriteWord(manager + 0x148u, 0u);
    runtime.InitializeXml(manager + 0x14cu);

    const auto pointers = manager + 0x188u;
    runtime.WriteByte(pointers, seed);
    for (std::uint32_t offset = 4u; offset <= 16u; offset += 4u)
        runtime.WriteWord(pointers + offset, 0u);
    const auto collection_memory = runtime.Allocate(0xfcu);
    runtime.WriteWord(manager + 0x19cu,
                      collection_memory == 0u ? 0u
                          : runtime.ConstructCollection(collection_memory));

    const auto tree = runtime.Allocate(0x18u);
    if (tree != 0u) {
        runtime.WriteByte(tree, seed);
        runtime.WriteWord(tree + 4u,
                          runtime.AllocateAndInitializeNode(tree));
        runtime.WriteWord(tree + 8u, 0u);
        runtime.WriteByte(tree + 12u, seed);
        const auto head = runtime.Allocate(0x24u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 4u, head);
        runtime.WriteWord(tree + 16u, head);
        runtime.WriteWord(tree + 20u, 0u);
    }
    runtime.WriteWord(manager + 0x1a0u, tree);
    for (std::uint32_t offset = 0x1a4u;
         offset <= 0x1d8u; offset += 4u)
        runtime.WriteWord(manager + offset, 0u);
    runtime.InitializeStruct(manager + 0x1dcu);
    runtime.WriteByte(manager + 0x220u, 0u);
    runtime.WriteByte(manager + 0x221u, 0u);
    runtime.WriteByte(manager + 0x223u, 0u);
    runtime.WriteWord(manager, 0x8443a4u);

    const auto begin = runtime.ReadWord(pointers + 4u);
    const auto capacity_end = runtime.ReadWord(pointers + 12u);
    if (begin == 0u || (capacity_end - begin) / 4u < 0x20u) {
        const auto replacement = runtime.Allocate(0x80u);
        const auto end = runtime.ReadWord(pointers + 8u);
        for (auto source = begin, destination = replacement;
             source != end; source += 4u, destination += 4u)
            runtime.CopyDwordIfDestinationNonNull(destination, source);
        runtime.FinishRange(begin, end);
        runtime.Free(begin);
        runtime.WriteWord(pointers + 12u, replacement + 0x80u);
        const auto size = runtime.CollectionSize(pointers);
        runtime.WriteWord(pointers + 4u, replacement);
        runtime.WriteWord(pointers + 8u, replacement + 4u * size);
    }

    const auto sounds = runtime.Allocate(0x14u);
    runtime.WriteWord(manager + 0x1a4u,
                      sounds == 0u ? 0u
                          : runtime.InitializeSounds(sounds));
    return manager;
}

}  // namespace pal4
