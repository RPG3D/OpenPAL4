#include "pal4/system_manager_handlers.h"

#include <array>

namespace pal4 {
namespace {

constexpr std::array<SystemManagerHandlers::Binding, 13> kHandlers{{
    {0x8b62e0u, 2u, 0x527ff0u, "re_join_stage"},
    {0x8b5f8cu, 2u, 0x527ff0u, "me_join_stage"},
    {0x8b599cu, 2u, 0x527ff0u, "ae_become_alive"},
    {0x8b5998u, 2u, 0x528080u, "ae_become_dead"},
    {0x8b5f90u, 2u, 0x528080u, "me_leave_stage"},
    {0x8b62e4u, 2u, 0x528080u, "re_leave_stage"},
    {0x8b8b8cu, 2u, 0x5280c0u, "combat_over"},
    {0x8b8b84u, 3u, 0x5280d0u, "combat_prepare"},
    {0x8b8b88u, 3u, 0x5283d0u, "combat_start"},
    {0x8b8b90u, 2u, 0x5283e0u, "close_system"},
    {0x8b5950u, 3u, 0x5286d0u, "ae_state_changed"},
    {0x8b5958u, 3u, 0x528730u, "ae_end_action"},
    {0x8b5954u, 3u, 0x528700u, "ae_start_action"},
}};

}  // namespace

std::span<const SystemManagerHandlers::Binding> SystemManagerHandlers::Bindings() {
    return kHandlers;
}

void ISystemManagerHandlerRuntime::Unregister(
    std::uint32_t router, std::uint32_t key, std::uint32_t handle) {
    const auto group = SystemEventRouterTree::FindOrCreateGroup(
        router, key, 0x8f609cu, *this);
    const auto head = ReadWord(group + 4u);
    auto node = ReadWord(head);
    while (node != head) {
        if (ReadWord(node + 12u) == handle) {
            const auto next = ReadWord(node);
            const auto previous = ReadWord(node + 4u);
            WriteWord(previous, next);
            WriteWord(next + 4u, previous);
            CleanupPayload(node + 8u);
            Free(node);
            WriteWord(group + 8u, ReadWord(group + 8u) - 1u);
            break;
        }
        node = ReadWord(node);
    }
}

void SystemManagerHandlers::Initialize(
    const std::uint32_t manager, ISystemManagerHandlerRuntime& runtime) {
    Cleanup(manager, runtime);
    for (std::uint32_t index = 0; index < kHandlers.size(); ++index) {
        const auto& spec = kHandlers[index];
        const auto key = runtime.ReadWord(spec.key_global);
        const auto router = runtime.EventRouter();
        const auto handle = runtime.Register(
            router, spec.mode, key, manager, spec.callback);
        runtime.WriteWord(manager + 4u + index * 8u, handle);
        runtime.WriteWord(manager + 8u + index * 8u, key);
    }
}

std::uint32_t SystemManagerHandlers::Cleanup(
    const std::uint32_t manager, ISystemManagerHandlerRuntime& runtime) {
    for (std::uint32_t index = 0; index < kHandlers.size(); ++index) {
        const auto slot = manager + 4u + index * 8u;
        const auto handle = runtime.ReadWord(slot);
        if (!handle) continue;
        const auto router = runtime.EventRouter();
        runtime.Unregister(router, runtime.ReadWord(slot + 4u), handle);
        runtime.WriteWord(slot, 0);
        runtime.WriteWord(slot + 4u, 0);
    }
    return 0;
}

std::uint32_t SystemManagerLifecycle::Construct(
    const std::uint32_t manager, ISystemManagerLifecycleRuntime& runtime) {
    for (auto index = 0u; index < kHandlers.size(); ++index) {
        runtime.WriteWord(manager + 4u + index * 8u, 0);
        runtime.WriteWord(manager + 8u + index * 8u, 0);
    }
    runtime.WriteWord(manager + 112u, 0);
    // IDA copies an indeterminate stack byte into +108; use a defined value.
    runtime.WriteByte(manager + 108u, 0);
    runtime.WriteWord(manager + 116u, 0);
    runtime.WriteWord(manager + 120u, 0);
    runtime.WriteWord(manager, 0x844908u);
    Deactivate(manager, runtime);
    SystemManagerHandlers::Initialize(manager, runtime);
    return manager;
}

std::uint32_t SystemManagerLifecycle::Destruct(
    const std::uint32_t manager, ISystemManagerLifecycleRuntime& runtime) {
    runtime.WriteWord(manager, 0x844908u);
    CleanupMemoryBlock(manager + 108u, runtime);
    return 0;
}

std::uint32_t SystemManagerLifecycle::UpdateState(
    const std::uint32_t manager, ISystemManagerLifecycleRuntime& runtime) {
    runtime.TouchSkillSystem();
    const auto previous_end = runtime.ReadWord(manager + 116u);
    runtime.WriteWord(manager + 116u, runtime.ReadWord(manager + 112u));
    return previous_end;
}

void SystemManagerLifecycle::Deactivate(
    const std::uint32_t manager, ISystemManagerLifecycleRuntime& runtime) {
    runtime.TouchSkillSystem();
    runtime.WriteByte(manager + 124u, 0);
}

void SystemManagerLifecycle::Activate(
    const std::uint32_t manager, ISystemManagerLifecycleRuntime& runtime) {
    runtime.TouchSkillSystem();
    runtime.WriteByte(manager + 124u, 1u);
}

void SystemManagerLifecycle::CleanupMemoryBlock(
    const std::uint32_t block, ISystemManagerLifecycleRuntime& runtime) {
    runtime.Free(runtime.ReadWord(block + 4u));
    runtime.WriteWord(block + 4u, 0);
    runtime.WriteWord(block + 8u, 0);
    runtime.WriteWord(block + 12u, 0);
}

void SystemManagerCallbacks::AddObject(
    const std::uint32_t manager, const std::uint32_t event_object,
    ISystemManagerCallbackRuntime& runtime) {
    const auto object = runtime.InternalObject(event_object);
    if (!object) {
        runtime.TouchSkillSystem();
        return;
    }
    runtime.PrepareObject(object);
    for (auto slot = runtime.ReadWord(manager + 112u);
         slot != runtime.ReadWord(manager + 116u); slot += 4u) {
        if (runtime.ReadWord(slot) == object) return;
    }
    runtime.AppendObject(manager, object);
    runtime.TouchSkillSystem();
    runtime.Update(manager);
}

void SystemManagerCallbacks::ProcessObject(
    const std::uint32_t manager, const std::uint32_t event_object,
    ISystemManagerCallbackRuntime& runtime) {
    (void)SystemManagerUpdate::RemoveObject(
        manager, runtime.InternalObject(event_object), runtime);
    runtime.TouchSkillSystem();
    runtime.Update(manager);
}

std::uint32_t SystemManagerCallbacks::InvokeManager(
    const std::uint32_t manager, ISystemManagerCallbackRuntime& runtime) {
    return runtime.InvokeManagerSlot0(manager);
}

}  // namespace pal4
