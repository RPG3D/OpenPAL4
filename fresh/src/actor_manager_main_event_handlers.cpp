#include "pal4/actor_manager_main_event_handlers.h"

#include <array>

namespace pal4 {

namespace {

constexpr std::array<ActorManagerMainEventHandlers::Binding, ActorManagerMainEventHandlers::kBindingCount>
    kBindings{{
        {0x8b8b84u, 0x53cf20u,"combat_prepare"},
        {0x8b8b88u, 0x57b8d0u,"combat_start"},
        {0x8b8b8cu, 0x53d3a0u,"combat_over"},
        {0x8b62d4u, 0x53d3f0u,"re_start_ui_control"},
        {0x8b62d8u, 0x53d650u,"re_close_ui_control"},
        {0x8b5960u, 0x53d680u,"ae_protect_action"},
        {0x8b5970u, 0x53d900u,"ae_critical_attack"},
        {0x8b5974u, 0x53d830u,"ae_fend_offed"},
        {0x8b5988u, 0x53da70u,"ae_fleed"},
        {0x8b5f8cu, 0x53d9a0u,"me_join_stage"},
    }};

}  // namespace

std::span<const ActorManagerMainEventHandlers::Binding> ActorManagerMainEventHandlers::Bindings() {return kBindings;}

std::uint32_t ActorManagerMainEventHandlers::Cleanup(
    const std::uint32_t manager,
    IActorManagerMainEventRuntime& runtime) {
    for (std::uint32_t index = 0u; index < kBindingCount; ++index) {
        const auto slot = manager + kFirstSlot + 8u * index;
        const auto handle = runtime.ReadWord(slot);
        if (handle == 0u) continue;

        const auto router = runtime.EventRouter();
        const auto key = runtime.ReadWord(slot + 4u);
        const auto group = runtime.FindEventGroup(router, key);
        if (group != 0u) {
            const auto head = runtime.ReadWord(group + 4u);
            for (auto node = runtime.ReadWord(head); node != head;
                 node = runtime.ReadWord(node)) {
                if (runtime.ReadWord(node + 12u) != handle) continue;
                const auto previous = runtime.ReadWord(node + 4u);
                const auto next = runtime.ReadWord(node);
                runtime.WriteWord(previous, next);
                runtime.WriteWord(next + 4u, previous);
                runtime.ReleaseOwnedSlot(node + 8u);
                runtime.FreeNode(node);
                runtime.WriteWord(group + 8u,
                                  runtime.ReadWord(group + 8u) - 1u);
                break;
            }
        }
        runtime.WriteWord(slot, 0u);
        runtime.WriteWord(slot + 4u, 0u);
    }
    return 0u;
}

std::uint32_t ActorManagerMainEventHandlers::Subscribe(
    const std::uint32_t manager,
    IActorManagerMainEventRuntime& runtime) {
    static_cast<void>(Cleanup(manager, runtime));
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u; index < kBindings.size(); ++index) {
        const auto slot = manager + kFirstSlot + 8u * index;
        const auto key = runtime.ReadWord(kBindings[index].key_global);
        result = runtime.Register(runtime.EventRouter(), 1u, key,
                                  manager, kBindings[index].callback);
        if (index == kBindings.size() - 1u) {
            runtime.WriteWord(slot + 4u, key);
            runtime.WriteWord(slot, result);
        } else {
            runtime.WriteWord(slot, result);
            runtime.WriteWord(slot + 4u, key);
        }
    }
    return result;
}

std::uint32_t ActorManagerLifecycle::Construct(
    const std::uint32_t manager,
    IActorManagerLifecycleRuntime& runtime) {
    runtime.InitializeGameObject(manager);
    for (std::uint32_t index = 0u;
         index < ActorManagerMainEventHandlers::kBindingCount; ++index) {
        const auto slot = manager +
            ActorManagerMainEventHandlers::kFirstSlot + 8u * index;
        runtime.WriteWord(slot, 0u);
        runtime.WriteWord(slot + 4u, 0u);
    }
    runtime.ConstructNameNode(manager + 384u);
    runtime.ConstructNameNode(manager + 392u);
    runtime.WriteWord(manager, kVtable);
    static_cast<void>(ActorManagerMainEventHandlers::Subscribe(
        manager, runtime));
    return manager;
}

std::uint32_t ActorManagerLifecycle::Destroy(
    const std::uint32_t manager,
    IActorManagerLifecycleRuntime& runtime) {
    runtime.WriteWord(manager, kVtable);
    static_cast<void>(ActorManagerMainEventHandlers::Cleanup(
        manager, runtime));
    runtime.ResetNameNode(manager + 392u);
    runtime.ResetNameNode(manager + 384u);
    return runtime.CleanupGameObject(manager);
}

std::uint32_t ActorManagerLifecycle::ResetEffectArgumentPair(
    std::uint32_t first, std::uint32_t second,
    const std::uint32_t third,
    IActorManagerLifecycleRuntime& runtime) {
    static_cast<void>(third);
    return runtime.ResetEffectHandle(first, second);
}

}  // namespace pal4
