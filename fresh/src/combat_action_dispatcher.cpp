#include "pal4/combat_action_dispatcher.h"

namespace pal4 {

void CombatActionDispatcher::CreateAndAssign(
    CombatActionKind kind, std::uint32_t resource,
    std::uint32_t actor, std::uint32_t event,
    bool has_target, ICombatActionDispatchRuntime& runtime) {
    const auto manager = runtime.ActorManager();
    const auto first_target = has_target ?
        runtime.ReadWord(event + 4u) : 0u;
    const auto first = runtime.CreateTask(
        kind, resource, actor, first_target, 0u);
    runtime.RegisterTask(manager + 96u, first);
    const auto second_target = has_target ?
        runtime.ReadWord(event + 4u) : 0u;
    const auto second = runtime.CreateTask(
        kind, resource, actor, second_target, 0u);
    runtime.AssignOwnedChild(actor, second);
}

std::uint32_t CombatActionDispatcher::Handle(
    std::uint32_t owner, std::uint32_t event,
    ICombatActionDispatchRuntime& runtime) {
    const auto actor = runtime.ActorFromFacet(event);
    if (runtime.ReadWord(actor + 804u) != 1u)
        return actor;

    const auto system = runtime.CombatSystem();
    runtime.WriteWord(system + 508u, 2u);
    const auto kind = runtime.ReadWord(event + 12u);
    switch (kind) {
    case 0u: {
        const auto database = runtime.GameDatabase();
        const auto resource = runtime.FindMagic(
            database + 12u, runtime.ReadWord(event + 8u));
        if (resource == 0u)
            return resource;
        CreateAndAssign(CombatActionKind::Magic, resource,
                        actor, event, true, runtime);
        break;
    }
    case 1u: {
        const auto database = runtime.GameDatabase();
        const auto resource = runtime.FindStunt(
            database + 12u, runtime.ReadWord(event + 8u));
        if (resource == 0u)
            return resource;
        CreateAndAssign(CombatActionKind::Stunt, resource,
                        actor, event, true, runtime);
        break;
    }
    case 2u: {
        const auto database = runtime.GameDatabase();
        const auto resource = runtime.FindProperty(
            database, runtime.ReadWord(event + 8u));
        CreateAndAssign(CombatActionKind::Property, resource,
                        actor, event, true, runtime);
        break;
    }
    case 3u:
        CreateAndAssign(CombatActionKind::Flee, 0u,
                        actor, event, false, runtime);
        break;
    case 4u:
        CreateAndAssign(CombatActionKind::Attack, 0u,
                        actor, event, true, runtime);
        break;
    case 5u:
        CreateAndAssign(CombatActionKind::Defend, 0u,
                        actor, event, false, runtime);
        break;
    default:
        break;
    }

    runtime.ResetActionComponent(actor + 716u);
    const auto router = runtime.ScriptEventRouter();
    runtime.DispatchStopSelect(router, 0x8b8b78u, event, 1u);
    runtime.WriteByte(owner + 72u, 0u);
    runtime.WriteWord(owner + 64u, 0u);
    runtime.WriteWord(owner + 68u, 0u);
    return 0u;
}

}  // namespace pal4
