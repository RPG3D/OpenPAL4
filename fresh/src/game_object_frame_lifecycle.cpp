#include "pal4/game_object_frame_lifecycle.h"

#include <initializer_list>

namespace pal4 {
namespace {
void ConstructFields(std::uint32_t object,IGameObjectFrameLifecycleRuntime& runtime) {
    runtime.InitEffectSystem(object);
    runtime.WriteByte(object + 260u, 0u);
    for (const auto offset : {264u, 268u, 272u, 276u})
        runtime.WriteWord(object + offset, 0u);
    // IDA reads two uninitialized stack bytes here; define both as zero.
    runtime.WriteByte(object + 280u, 0u);
    runtime.WriteByte(object + 281u, 0u);
    runtime.WriteByte(object + 288u, 0u);
}
std::uint32_t FinishConstruct(std::uint32_t object,IGameObjectFrameLifecycleRuntime& runtime) {
    runtime.WriteWord(object,GameObjectFrameLifecycle::kVtable);
    (void)GameObjectFrameLifecycle::SetUpdateHandler(object,GameObjectFrameLifecycle::kFrameCallback,runtime);
    runtime.WriteWord(object+296u,0u);
    return object;
}
}
std::uint32_t GameObjectFrameLifecycle::ConstructHostManaged(
    std::uint32_t object,IGameObjectFrameLifecycleRuntime& runtime) {
    ConstructFields(object,runtime);
    runtime.WriteWord(object+284u,0u);
    runtime.WriteWord(object+292u,0u);
    return FinishConstruct(object,runtime);
}
std::uint32_t GameObjectFrameLifecycle::DestroyHostManaged(
    std::uint32_t object,IGameObjectFrameLifecycleRuntime& runtime) {
    runtime.WriteWord(object,kVtable);
    DeinitializeHelper(object,runtime);
    runtime.ResetManager(object+260u);
    return runtime.DestroyEffectSystem(object);
}
std::uint32_t GameObjectFrameLifecycle::Construct(
    const std::uint32_t object,
    IGameObjectFrameLifecycleRuntime& runtime) {
    ConstructFields(object,runtime);

    const auto candidate = runtime.Allocate(24u);
    if (candidate == 0u) return 0u;
    runtime.WriteWord(candidate + 4u, 0u);
    runtime.WriteWord(candidate + 20u, 1u);
    auto shared_head = runtime.ReadWord(kSharedTreeHead);
    if (shared_head == 0u) {
        shared_head = candidate;
        runtime.WriteWord(kSharedTreeHead, shared_head);
        runtime.WriteWord(shared_head, 0u);
        runtime.WriteWord(shared_head + 8u, 0u);
    } else {
        runtime.Free(candidate);
    }
    runtime.WriteWord(kSharedTreeCount,
                      runtime.ReadWord(kSharedTreeCount) + 1u);

    const auto local_head = runtime.Allocate(24u);
    if (local_head == 0u) return 0u;
    runtime.WriteWord(local_head + 4u, shared_head);
    runtime.WriteWord(local_head + 20u, 0u);
    runtime.WriteWord(object + 284u, local_head);
    runtime.WriteWord(object + 292u, 0u);
    runtime.WriteWord(local_head, local_head);
    runtime.WriteWord(local_head + 8u, local_head);
    return FinishConstruct(object,runtime);
}

std::uint32_t GameObjectFrameLifecycle::Destroy(
    const std::uint32_t object,
    IGameObjectFrameLifecycleRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    DeinitializeHelper(object, runtime);
    const auto manager = object + 260u;
    DestroyManagerTree(manager, runtime);
    return runtime.DestroyEffectSystem(object);
}

void GameObjectFrameLifecycle::DestroyManagerTree(
    const std::uint32_t manager,
    IGameObjectFrameLifecycleRuntime& runtime) {
    runtime.ResetManager(manager);

    const auto head = runtime.ReadWord(manager + 24u);
    const auto count = runtime.ReadWord(manager + 32u);
    const auto holder = manager + 20u;
    auto node = runtime.ReadWord(head);
    if (count != 0u) {
        runtime.CleanupTree(holder, runtime.ReadWord(head + 4u));
        runtime.WriteWord(head + 4u, runtime.ReadWord(kSharedTreeHead));
        runtime.WriteWord(manager + 32u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        runtime.GetFirstElement(holder, manager);
    } else {
        while (node != head) {
            const auto next = runtime.NextNode(node);
            runtime.RemoveNode(holder, node);
            node = next;
        }
    }

    runtime.Free(head);
    runtime.WriteWord(manager + 24u, 0u);
    runtime.WriteWord(manager + 32u, 0u);
    const auto remaining = runtime.ReadWord(kSharedTreeCount) - 1u;
    runtime.WriteWord(kSharedTreeCount, remaining);
    if (remaining == 0u) {
        const auto shared_head = runtime.ReadWord(kSharedTreeHead);
        runtime.WriteWord(kSharedTreeHead, 0u);
        if (shared_head != 0u) runtime.Free(shared_head);
    }
}

std::uint8_t GameObjectFrameLifecycle::Initialize(
    const std::uint32_t object,
    IGameObjectFrameLifecycleRuntime& runtime) {
    runtime.DisableEffectSystem(object);
    const auto game = runtime.GameInstance();
    const auto combat = runtime.CombatSystem();
    const auto camera_slot = runtime.CurrentCameraSlot(combat);
    runtime.EnableEffectSystem(object, runtime.ReadWord(camera_slot),
                               game + 428u);
    return static_cast<std::uint8_t>(
        runtime.InitializeManagers(object + 260u));
}

void GameObjectFrameLifecycle::DeinitializeHelper(
    const std::uint32_t object,
    IGameObjectFrameLifecycleRuntime& runtime) {
    runtime.ResetManager(object + 260u);
    runtime.DisableEffectSystem(object);
}

std::uint32_t GameObjectFrameLifecycle::SetUpdateHandler(
    const std::uint32_t object, const std::uint32_t callback,
    IGameObjectFrameLifecycleRuntime& runtime) {
    runtime.WriteWord(object + 300u, callback);
    return callback;
}

}  // namespace pal4
