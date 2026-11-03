#include "pal4/actor_manager_data.h"

#include <initializer_list>

namespace pal4 {

std::uint32_t ActorManagerDataLifecycle::Initialize(
    const std::uint32_t object, const std::uint32_t property,
    const std::uint32_t owned_property,
    ActorManagerTreeGlobals& globals,
    IActorManagerDataRuntime& runtime) {
    runtime.InitializeScriptBase(object);
    runtime.InitializeLocaleFacet(object + 716u);
    runtime.InitializeComplex(object + 728u);
    runtime.WriteWord(object + 724u, 0x844cccu);
    runtime.WriteWord(object + 800u, owned_property);
    runtime.WriteByte(object + 796u, owned_property != 0u);
    runtime.WriteWord(object + 804u, property);
    runtime.WriteWord(object + 808u, 0u);
    runtime.WriteByte(object + 812u,
                      static_cast<std::uint8_t>(owned_property));
    for (const auto offset : {816u, 820u, 824u, 828u, 832u})
        runtime.WriteWord(object + offset, 0u);
    runtime.WriteByte(object + 836u,
                      static_cast<std::uint8_t>(owned_property));
    runtime.WriteByte(object + 837u,
                      static_cast<std::uint8_t>(owned_property));
    runtime.WriteByte(object + 844u, 0u);

    const auto candidate = runtime.Allocate(28u);
    runtime.WriteWord(candidate + 4u, 0u);
    runtime.WriteWord(candidate + 24u, 1u);
    if (!globals.nil_node) {
        globals.nil_node = candidate;
        runtime.WriteWord(candidate, 0u);
        runtime.WriteWord(candidate + 8u, 0u);
    } else {
        (void)runtime.Free(candidate);
    }
    ++globals.references;
    const auto head = runtime.Allocate(28u);
    runtime.WriteWord(head + 4u, globals.nil_node);
    runtime.WriteWord(head + 24u, 0u);
    runtime.WriteWord(object + 840u, head);
    runtime.WriteWord(object + 848u, 0u);
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 8u, head);

    for (const auto offset : {852u, 860u, 868u, 876u, 884u})
        runtime.ConstructNameNode(object + offset);
    runtime.WriteWord(object + 716u, 0x844cb8u);
    runtime.WriteWord(object, 0x844a68u);
    runtime.WriteWord(object + 724u, 0x844a5cu);
    return object;
}

std::uint32_t ActorManagerDataLifecycle::Property(
    const std::uint32_t object, IActorManagerDataRuntime& runtime) {
    return runtime.ReadWord(object + 832u);
}

std::uint32_t ActorManagerDataLifecycle::Delete(
    const std::uint32_t object, const std::uint8_t flags,
    IActorManagerDataRuntime& runtime) {
    Destroy(object, runtime);
    if (flags & 1u) (void)runtime.Free(object);
    return object;
}

std::uint32_t ActorManagerDataLifecycle::CleanupTree(
    const std::uint32_t tree, ActorManagerTreeGlobals& globals,
    IActorManagerDataRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto cursor = runtime.ReadWord(head);
    if (runtime.ReadWord(tree + 12u)) {
        runtime.DestroyTreeRoot(tree, runtime.ReadWord(head + 4u));
        runtime.WriteWord(head + 4u, globals.nil_node);
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        runtime.GetFirstTreeElement(tree);
    } else {
        while (cursor != head) {
            const auto node = cursor;
            cursor = runtime.AdvanceTreeCursor(cursor);
            runtime.EraseTreeNode(tree, node);
        }
    }
    (void)runtime.Free(head);
    runtime.WriteWord(tree + 4u, 0u);
    runtime.WriteWord(tree + 12u, 0u);
    const auto remaining = --globals.references;
    if (remaining || !globals.nil_node) return remaining;
    const auto nil = globals.nil_node;
    globals.nil_node = 0;
    return runtime.Free(nil);
}

void ActorManagerDataLifecycle::Destroy(
    const std::uint32_t object, IActorManagerDataRuntime& runtime) {
    runtime.WriteWord(object, 0x844a68u);
    runtime.WriteWord(object + 716u, 0x844cb8u);
    runtime.WriteWord(object + 724u, 0x844a5cu);

    const auto head = runtime.ReadWord(object + 840u);
    for (auto cursor = runtime.ReadWord(head); cursor != head;
         cursor = runtime.AdvanceTreeCursor(cursor))
        runtime.HandleEffect(cursor + 16u);
    runtime.ClearTreeRange(object + 836u, runtime.ReadWord(head), head);
    for (const auto offset : {852u, 860u, 868u, 876u, 884u})
        runtime.HandleEffect(object + offset);
    for (const auto offset : {884u, 876u, 868u, 860u, 852u})
        runtime.ResetNameNode(object + offset);
    runtime.CleanupOtherTree(object + 836u);

    const auto text = runtime.ReadWord(object + 816u);
    if (text) {
        const auto count = runtime.ReadByte(text - 1u);
        if (count == 0u || count == 0xffu)
            (void)runtime.Free(text - 1u);
        else
            runtime.WriteByte(text - 1u,
                              static_cast<std::uint8_t>(count - 1u));
    }
    for (const auto offset : {816u, 820u, 824u})
        runtime.WriteWord(object + offset, 0u);
    if (runtime.ReadByte(object + 796u)) {
        const auto owned = runtime.ReadWord(object + 800u);
        if (owned) runtime.DestroyOwnedProperty(owned, 1u);
    }
    runtime.WriteWord(object + 724u, 0x844cccu);
    runtime.CleanupComplex(object + 728u);
    runtime.CleanupLocaleFacet(object + 716u);
    runtime.WriteWord(object, 0x8430e8u);
    for (const auto offset : {696u, 680u}) {
        (void)runtime.Free(runtime.ReadWord(object + offset));
        for (auto field = offset; field < offset + 12u; field += 4u)
            runtime.WriteWord(object + field, 0u);
    }
    runtime.ResetScriptBase(object);
}

}  // namespace pal4
