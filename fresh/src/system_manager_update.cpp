#include "pal4/system_manager_update.h"

namespace pal4 {

float SystemObjectProperty::Get(const SystemObjectPropertyRecord& object) {
    return object.value;
}
namespace {

class ActorIteratorGuard final {
public:
    ActorIteratorGuard(ISystemManagerUpdateRuntime& runtime,
                       const std::uint32_t handle)
        : runtime_(runtime), handle_(handle) {}
    ~ActorIteratorGuard() {
        if (handle_) runtime_.ReleaseActorIterator(handle_);
    }
    ActorIteratorGuard(const ActorIteratorGuard&) = delete;
    ActorIteratorGuard& operator=(const ActorIteratorGuard&) = delete;

private:
    ISystemManagerUpdateRuntime& runtime_;
    std::uint32_t handle_;
};

bool AnyReadyActor(const std::uint32_t iterator,
                   ISystemManagerUpdateRuntime& runtime) {
    while (!runtime.IteratorFinished(iterator)) {
        if (runtime.ActorReady(runtime.IteratorCurrent(iterator))) return true;
        runtime.IteratorAdvance(iterator);
    }
    return false;
}

}  // namespace

std::uint8_t SystemManagerUpdate::Dispatch(
    const std::uint32_t manager, const float seconds,
    ISystemManagerUpdateRuntime& runtime) {
    if (!CanUpdate(manager, runtime)) return 0;
    const auto result = runtime.CombatInactive() ?
        OutOfCombat(manager, runtime) : InCombat(manager, seconds, runtime);
    return static_cast<std::uint8_t>(result);
}

std::uint32_t SystemManagerUpdate::OutOfCombat(
    const std::uint32_t manager, ISystemManagerUpdateRuntime& runtime) {
    std::vector<std::uint32_t> ready;
    do {
        for (auto slot = runtime.ReadWord(manager + 112u);
             slot != runtime.ReadWord(manager + 116u); slot += 4u) {
            const auto object = runtime.ReadWord(slot);
            if (AdvanceObject(object, 20.0f, runtime)) ready.push_back(object);
        }
        const auto begin = runtime.ReadWord(manager + 112u);
        if (!begin || begin == runtime.ReadWord(manager + 116u)) break;
    } while (!ProcessEvents(manager, ready, runtime));
    return runtime.ReleaseEvents(ready);
}

std::uint32_t SystemManagerUpdate::InCombat(
    const std::uint32_t manager, const float seconds,
    ISystemManagerUpdateRuntime& runtime) {
    std::vector<std::uint32_t> ready;
    const auto elapsed = seconds * 1000.0f;
    for (auto slot = runtime.ReadWord(manager + 112u);
         slot != runtime.ReadWord(manager + 116u); slot += 4u) {
        const auto object = runtime.ReadWord(slot);
        if (AdvanceObject(object, elapsed, runtime)) ready.push_back(object);
    }
    (void)ProcessEvents(manager, ready, runtime);
    return runtime.ReleaseEvents(ready);
}

bool SystemManagerUpdate::AdvanceObject(
    const std::uint32_t object, const float elapsed,
    ISystemManagerUpdateRuntime& runtime) {
    const auto rate = runtime.ObjectRate(object);
    const auto value = runtime.ObjectProperty(object) + rate * elapsed;
    const auto clamped = value > 64.0f ? 64.0f : value;
    runtime.SetObjectProperty(object, clamped);
    return clamped >= 64.0f;
}

bool SystemManagerUpdate::ProcessEvents(
    const std::uint32_t manager,
    const std::vector<std::uint32_t>& ready_objects,
    ISystemManagerUpdateRuntime& runtime) {
    if (ready_objects.empty()) return false;
    auto selected = ready_objects.front();
    for (auto index = 1u; index < ready_objects.size(); ++index) {
        const auto candidate = ready_objects[index];
        const auto selected_rate = runtime.ObjectRate(selected);
        const auto candidate_rate = runtime.ObjectRate(candidate);
        if (candidate_rate > selected_rate)
            selected = candidate;
    }
    const auto state = runtime.CombatState();
    if (state == 1u || state == 3u) {
        runtime.TouchSkillSystem();
        if (runtime.CombatState() != 3u || !AllObjectsReady(manager, runtime)) {
            runtime.PauseManager(manager);
            return true;
        }
        runtime.SetCombatState(2u);
    }
    if (!IsActive(manager, runtime)) runtime.StartObject(selected);
    return true;
}

std::uint32_t SystemManagerUpdate::SwapBuffers(
    const std::uint32_t manager, ISystemManagerUpdateRuntime& runtime) {
    const auto old_end = runtime.ReadWord(manager + 116u);
    runtime.WriteWord(manager + 116u, runtime.ReadWord(manager + 112u));
    return old_end;
}

std::uint8_t SystemManagerUpdate::IsActive(
    const std::uint32_t manager, ISystemManagerUpdateRuntime& runtime) {
    return runtime.ReadByte(manager + 124u);
}

bool SystemManagerUpdate::CanUpdate(
    const std::uint32_t manager, ISystemManagerUpdateRuntime& runtime) {
    if (IsActive(manager, runtime)) return false;
    const auto state = static_cast<std::int32_t>(runtime.CombatState());
    return state != 0 && (state <= 3 || state > 7);
}

void SystemManagerUpdate::Update(
    const std::uint32_t manager, ISystemManagerUpdateRuntime& runtime) {
    if (runtime.CombatState() == 1u) return;
    runtime.TouchSkillSystem();
    runtime.TouchGlobalSystem();
    if (!IsActive(manager, runtime)) return;
    runtime.TouchSkillSystem();
    if (AllObjectsReady(manager, runtime)) {
        runtime.TouchSkillSystem();
        runtime.InvokeManagerSlot4(manager);
    } else {
        runtime.TouchSkillSystem();
    }
}

bool SystemManagerUpdate::AllObjectsReady(
    const std::uint32_t manager, ISystemManagerUpdateRuntime& runtime) {
    for (auto slot = runtime.ReadWord(manager + 112u);
         slot != runtime.ReadWord(manager + 116u); slot += 4u) {
        if (!runtime.ObjectReady(runtime.ReadWord(slot))) return false;
    }
    const auto first = runtime.BeginActors(0u);
    ActorIteratorGuard first_guard(runtime, first);
    const auto first_ready = AnyReadyActor(first, runtime);
    runtime.TouchSkillSystem();
    const auto second = runtime.BeginActors(1u);
    ActorIteratorGuard second_guard(runtime, second);
    const auto second_ready = AnyReadyActor(second, runtime);
    runtime.TouchSkillSystem();
    return first_ready && second_ready;
}

std::uint32_t SystemManagerUpdate::RemoveObject(
    const std::uint32_t manager, const std::uint32_t object,
    ISystemManagerArrayRuntime& runtime) {
    const auto end = runtime.ReadWord(manager + 116u);
    auto slot = runtime.ReadWord(manager + 112u);
    while (slot != end && runtime.ReadWord(slot) != object) slot += 4u;
    if (slot == end) return end;
    for (auto cursor = slot + 4u; cursor != end; cursor += 4u)
        runtime.WriteWord(cursor - 4u, runtime.ReadWord(cursor));
    runtime.WriteWord(manager + 116u, end - 4u);
    return end;
}

std::uint8_t SystemManagerUpdate::HandleDialogParam(
    const std::uint32_t manager, const std::uint32_t parameter,
    ISystemManagerUpdateRuntime& runtime) {
    const auto actor = runtime.DialogDataFromParam(parameter);
    const auto ready = runtime.ActorReady(actor);
    if (ready) return ready;
    return static_cast<std::uint8_t>(RemoveObject(
        manager, runtime.InternalObject(parameter), runtime));
}

std::uint8_t SystemManagerUpdate::SetCombatStateFromParam(
    const std::uint32_t parameter, ISystemManagerUpdateRuntime& runtime) {
    const auto actor = runtime.DialogDataFromParam(parameter);
    const auto ready = runtime.ActorReady(actor);
    if (!ready) return ready;
    const auto combat = runtime.CombatSystemAddress();
    runtime.SetCombatState(3u);
    return static_cast<std::uint8_t>(combat);
}

std::uint8_t SystemManagerUpdate::UpdateCombatState(
    const std::uint32_t manager, ISystemManagerUpdateRuntime& runtime) {
    Update(manager, runtime);
    auto combat = runtime.CombatSystemAddress();
    if (runtime.CombatState() == 3u) {
        if (!AllObjectsReady(manager, runtime)) return 0;
        combat = runtime.CombatSystemAddress();
        runtime.SetCombatState(2u);
    }
    return static_cast<std::uint8_t>(combat);
}

}  // namespace pal4
