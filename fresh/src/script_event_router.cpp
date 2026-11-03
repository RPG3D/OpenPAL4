#include "pal4/script_event_router.h"
#include "pal4/system_event_router_tree.h"

namespace pal4 {

std::uint32_t ScriptEventRouter::GetInstance(
    IScriptEventRouterRuntime& runtime) {
    const auto guard = runtime.ReadByte(kSingletonGuard);
    if ((guard & 1u) != 0u)
        return runtime.ReadWord(kSingletonPointer);
    runtime.WriteByte(kSingletonGuard, guard | 1u);
    runtime.GetSingletonManager();
    const auto instance = runtime.RegisterSingleton(
        "cbEventRouter", 1027u, kCreateEntry, kDestroyEntry);
    runtime.WriteWord(kSingletonPointer, instance);
    return instance;
}

std::uint32_t ScriptEventRouter::Create(
    IScriptEventRouterRuntime& runtime) {
    const auto router = runtime.AllocateObject(20u);
    if (router == 0u) return 0u;
    runtime.WriteByte(router, 0u);
    runtime.WriteByte(router + 1u, 0u);
    runtime.WriteByte(router + 8u, 0u);

    const auto proposed_nil = runtime.AllocateNode(32u);
    if (proposed_nil == 0u) {
        (void)runtime.Free(router);
        return 0u;
    }
    runtime.WriteWord(proposed_nil + 4u, 0u);
    runtime.WriteWord(proposed_nil + 28u, 1u);
    auto nil = runtime.ReadWord(kNilPointer);
    if (nil == 0u) {
        nil = proposed_nil;
        runtime.WriteWord(kNilPointer, nil);
        runtime.WriteWord(nil, 0u);
        runtime.WriteWord(nil + 8u, 0u);
    }
    runtime.WriteWord(kInstanceCount,
                      runtime.ReadWord(kInstanceCount) + 1u);
    if (nil != proposed_nil) (void)runtime.Free(proposed_nil);

    const auto head = runtime.AllocateNode(32u);
    if (head == 0u) {
        const auto remaining = runtime.ReadWord(kInstanceCount) - 1u;
        runtime.WriteWord(kInstanceCount, remaining);
        if (remaining == 0u) {
            runtime.WriteWord(kNilPointer, 0u);
            (void)runtime.Free(nil);
        }
        (void)runtime.Free(router);
        return 0u;
    }
    runtime.WriteWord(head + 4u, nil);
    runtime.WriteWord(head + 28u, 0u);
    runtime.WriteWord(router + 4u, head);
    runtime.WriteWord(router + 12u, 0u);
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 8u, head);
    runtime.WriteByte(router + 16u, 0u);
    return router;
}

std::uint32_t ScriptEventRouter::Destroy(
    const std::uint32_t router, IScriptEventRouterRuntime& runtime) {
    if (router == 0u) return 0u;
    (void)Destruct(router, runtime);
    return runtime.Free(router);
}

std::uint32_t ScriptEventRouter::Destruct(
    const std::uint32_t router, IScriptEventRouterRuntime& runtime) {
    const auto head = runtime.ReadWord(router + 4u);
    const auto count = runtime.ReadWord(router + 12u);
    auto first = runtime.ReadWord(head);
    if (count != 0u) {
        (void)ScriptEventRouterTree::EraseRecursive(
            runtime.ReadWord(head + 4u), runtime);
        runtime.WriteWord(head + 4u, runtime.ReadWord(kNilPointer));
        runtime.WriteWord(router + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        runtime.ResetHeadCursor(router);
    } else {
        while (first != head) {
            const auto next = ScriptEventRouterTree::NextNode(
                first, runtime);
            (void)ScriptEventRouterTree::EraseNode(router, first, runtime);
            first = next;
        }
    }
    (void)runtime.Free(head);
    runtime.WriteWord(router + 4u, 0u);
    runtime.WriteWord(router + 12u, 0u);
    const auto remaining = runtime.ReadWord(kInstanceCount) - 1u;
    runtime.WriteWord(kInstanceCount, remaining);
    if (remaining != 0u) return remaining;
    const auto nil = runtime.ReadWord(kNilPointer);
    runtime.WriteWord(kNilPointer, 0u);
    if (nil != 0u) return runtime.Free(nil);
    return nil;
}

std::uint32_t ScriptEventRouter::ProcessEvent(
    const std::uint32_t router, const std::uint32_t key,
    const std::uint32_t payload, const std::uint32_t,
    IScriptEventRouterRuntime& runtime) {
    if (runtime.ReadByte(router + 16u) != 0u) return 0u;
    const auto node = runtime.FindNode(router, key);
    if (node == runtime.ReadWord(router + 4u)) return 0u;
    const auto callbacks = runtime.ReadWord(node + 20u);
    for (auto current = runtime.ReadWord(callbacks);
         current != callbacks; current = runtime.ReadWord(current)) {
        runtime.InvokeCallback(runtime.ReadWord(current + 12u), payload);
    }
    return runtime.ReadWord(node + 24u);
}

std::uint32_t ScriptEventRouter::RegisterHandler(
    std::uint32_t router, std::uint32_t mode,
    std::uint32_t key, std::uint32_t owner,
    std::uint32_t callback, ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        router, mode, key, owner, callback,
        runtime.ReadWord(kNilPointer), runtime);
}

}  // namespace pal4
