#include "pal4/arena_effect_manager.h"

#include <bit>

namespace pal4 {

std::uint32_t ArenaEffectManager::Register(
    std::uint8_t& guard, std::uint32_t& registered,
    IArenaEffectRuntime& runtime) {
    if ((guard & 1u) != 0u) return registered;
    guard |= 1u;
    registered = runtime.RegisterSingleton(kSingletonName, kSingletonId,
                                           0x522730u, 0x7de5d0u);
    return registered;
}

std::uint32_t ArenaEffectManager::Create(
    ArenaEffectSharedState& shared, IArenaEffectRuntime& runtime) {
    const auto object = runtime.Allocate(kObjectSize);
    if (object == 0u) return 0u;
    const auto initialized = Initialize(object, shared, runtime);
    if (initialized == 0u) runtime.Free(object);
    return initialized;
}

std::uint32_t ArenaEffectManager::Initialize(
    const std::uint32_t object, ArenaEffectSharedState& shared,
    IArenaEffectRuntime& runtime) {
    (void)runtime.Call(ArenaEffectCall::InitializeBase, {object});
    runtime.WriteByte(object + 260u, 0u);
    for (const auto offset : {264u, 268u, 272u, 276u})
        runtime.WriteWord(object + offset, 0u);
    // The original reads two uninitialized stack bytes here.
    runtime.WriteByte(object + 280u, 0u);
    runtime.WriteByte(object + 281u, 0u);
    runtime.WriteByte(object + 288u, 0u);

    const auto candidate = runtime.Allocate(24u);
    if (candidate == 0u) return 0u;
    runtime.WriteWord(candidate + 4u, 0u);
    runtime.WriteWord(candidate + 20u, 1u);
    const bool created_sentinel = shared.sentinel == 0u;
    if (created_sentinel) {
        shared.sentinel = candidate;
        runtime.WriteWord(candidate, 0u);
        runtime.WriteWord(candidate + 8u, 0u);
    } else {
        runtime.Free(candidate);
    }

    const auto head = runtime.Allocate(24u);
    if (head == 0u) {
        if (created_sentinel) {
            runtime.Free(shared.sentinel);
            shared.sentinel = 0u;
        }
        return 0u;
    }
    ++shared.instance_count;
    runtime.WriteWord(head + 4u, shared.sentinel);
    runtime.WriteWord(head + 20u, 0u);
    runtime.WriteWord(object + 284u, head);
    runtime.WriteWord(object + 292u, 0u);
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 8u, head);
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 296u, 0u);
    return object;
}

std::uint32_t ArenaEffectManager::Cleanup(
    const std::uint32_t object, ArenaEffectSharedState& shared,
    IArenaEffectRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    (void)runtime.Call(ArenaEffectCall::ResetExisting, {object + 260u});
    CleanupTreeContainer(object + 280u, shared, runtime);
    return runtime.Call(ArenaEffectCall::DestroyBase, {object});
}

std::uint32_t ArenaEffectManager::CleanupTree(
    const std::uint32_t node, ArenaEffectSharedState& shared,
    IArenaEffectRuntime& runtime) {
    auto current = node;
    while (current != shared.sentinel) {
        (void)CleanupTree(runtime.ReadWord(current + 8u), shared, runtime);
        const auto left = runtime.ReadWord(current);
        runtime.Free(current);
        current = left;
    }
    return shared.sentinel;
}

void ArenaEffectManager::CleanupTreeContainer(
    const std::uint32_t tree, ArenaEffectSharedState& shared,
    IArenaEffectRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    if (head != 0u) {
        if (runtime.ReadWord(tree + 12u) != 0u) {
            (void)CleanupTree(runtime.ReadWord(head + 4u), shared, runtime);
            runtime.WriteWord(head + 4u, shared.sentinel);
            runtime.WriteWord(tree + 12u, 0u);
            runtime.WriteWord(head, head);
            runtime.WriteWord(head + 8u, head);
            (void)runtime.Call(ArenaEffectCall::ResetFirstTreeElement, {tree});
        } else if (runtime.ReadWord(head) != head) {
            (void)runtime.Call(ArenaEffectCall::RemoveUncountedTreeNodes,
                               {tree, head});
        }
        runtime.Free(head);
    }
    runtime.WriteWord(tree + 4u, 0u);
    runtime.WriteWord(tree + 12u, 0u);
    if (shared.instance_count != 0u && --shared.instance_count == 0u) {
        runtime.Free(shared.sentinel);
        shared.sentinel = 0u;
    }
}

bool ArenaEffectManager::InitializeGameManagers(
    const std::uint32_t object, IArenaEffectRuntime& runtime) {
    return static_cast<std::uint8_t>(runtime.Call(
        ArenaEffectCall::InitializeGameManagers, {object + 260u})) != 0u;
}

std::uint32_t ArenaEffectManager::CleanupScriptCallback(
    const std::uint32_t object, ArenaEffectSharedState& shared,
    IArenaEffectRuntime& runtime) {
    const auto singleton = runtime.Call(ArenaEffectCall::GetEffectSingleton,
                                        {});
    (void)runtime.Call(ArenaEffectCall::StopActive, {singleton + 260u});
    const auto previous = runtime.ReadWord(object + 296u);
    if (previous != 0u) {
        shared.script_callback = previous;
        runtime.WriteWord(object + 296u, 0u);
    }
    return previous;
}

void ArenaEffectManager::CleanupAndShutdown(
    const std::uint32_t object, ArenaEffectSharedState& shared,
    IArenaEffectRuntime& runtime) {
    (void)CleanupScriptCallback(object, shared, runtime);
    (void)runtime.Call(ArenaEffectCall::ResetExisting, {object + 260u});
    (void)runtime.Call(ArenaEffectCall::CleanupEffects, {object});
    (void)runtime.Call(ArenaEffectCall::Disable, {object});
}

void ArenaEffectManager::CleanupAndShutdownEffects(
    const std::uint32_t object, ArenaEffectSharedState& shared,
    IArenaEffectRuntime& runtime) {
    (void)CleanupScriptCallback(object, shared, runtime);
    (void)runtime.Call(ArenaEffectCall::CleanupEffects, {object});
    (void)runtime.Call(ArenaEffectCall::Disable, {object});
}

void ArenaEffectManager::RenderPasses(
    const std::uint32_t object, const std::uint32_t pass,
    IArenaEffectRuntime& runtime) {
    (void)runtime.Call(ArenaEffectCall::RenderPasses,
                       {object + 260u, pass});
}

std::uint32_t ArenaEffectManager::UpdateRenderParams(
    const std::uint32_t object, const std::uint32_t first,
    const std::uint32_t second, IArenaEffectRuntime& runtime) {
    return runtime.Call(ArenaEffectCall::UpdateRenderParameters,
                        {object + 260u, first, second});
}

std::uint32_t ArenaEffectManager::HandleScriptCallback(
    const std::uint32_t object, const std::uint32_t argument,
    ArenaEffectSharedState& shared, IArenaEffectRuntime& runtime) {
    (void)runtime.Call(ArenaEffectCall::HandleScriptExecution,
                       {object + 260u, argument});
    const auto previous = runtime.ReadWord(object + 296u);
    if (previous != 0u) {
        shared.script_callback = previous;
        runtime.WriteWord(object + 296u, 0u);
    }
    if (static_cast<std::int32_t>(runtime.ReadWord(object + 264u)) > 0) {
        const auto next = runtime.ReadWord(object + 300u);
        runtime.WriteWord(object + 296u, shared.script_callback);
        shared.script_callback = next;
    }
    return previous;
}

bool ArenaEffectManager::AddActiveObject(
    const std::uint32_t clump, IArenaEffectRuntime& runtime) {
    const auto arena = runtime.Call(ArenaEffectCall::GetGameArenaManager,
                                    {});
    (void)runtime.Call(ArenaEffectCall::AddClump, {arena, clump});
    return true;
}

std::uint8_t ArenaEffectManager::Enable(
    const std::uint32_t object, const std::uint32_t first,
    const std::uint32_t second, IArenaEffectRuntime& runtime) {
    return static_cast<std::uint8_t>(runtime.Call(ArenaEffectCall::Enable,
                                                   {object, first, second}));
}

std::uint8_t ArenaEffectManager::Draw(
    const std::uint32_t object, const std::uint32_t first,
    const std::uint32_t second, const std::uint32_t third,
    const std::uint32_t fourth, const std::uint32_t fifth,
    const std::uint32_t sixth, IArenaEffectRuntime& runtime) {
    return static_cast<std::uint8_t>(runtime.Call(
        ArenaEffectCall::Draw,
        {object, first, second, third, fourth, fifth, sixth}));
}

std::uint32_t ArenaEffectManager::RenderTransform(
    const double value, IArenaEffectRuntime& runtime) {
    const auto bits = std::bit_cast<std::uint64_t>(value);
    return runtime.Call(ArenaEffectCall::RenderTransform,
                        {static_cast<std::uint32_t>(bits),
                         static_cast<std::uint32_t>(bits >> 32u)});
}

}  // namespace pal4
