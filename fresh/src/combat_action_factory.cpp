#include "pal4/combat_action_factory.h"

#include "pal4/check_protect_task.h"
#include "pal4/cast_magic_sequence_task.h"
#include "pal4/combat_defend_sequence_task.h"
#include "pal4/combat_dead_sequence_task.h"
#include "pal4/combat_end_action_task.h"
#include "pal4/combat_flee_sequence_task.h"
#include "pal4/combat_fend_off_sequence_task.h"
#include "pal4/combat_miss_sequence_task.h"
#include "pal4/combat_stunt_sequence_task.h"
#include "pal4/combat_use_property_sequence_task.h"
#include "pal4/level_up_action_task.h"
#include "pal4/quick_attack_task.h"
#include "pal4/quick_cast_magic_sequence_task.h"
#include "pal4/win_action_task.h"

namespace pal4 {
namespace {

struct ActionConstructionGuard {
    ICombatActionFactoryRuntime& runtime;
    std::uint32_t object;
    bool container_initialized = false;
    bool name_initialized = false;

    ~ActionConstructionGuard() {
        if (object == 0u) return;
        if (name_initialized) runtime.CleanupName(object + 20u);
        if (container_initialized) runtime.DestroyContainer(object + 4u);
        runtime.Free(object);
    }
};

}  // namespace

std::uint32_t CombatActionFactory::Create(
    const std::uint32_t source, const std::uint32_t target,
    const std::uint8_t mode, ICombatActionFactoryRuntime& runtime) {
    if (source == 0u || target == 0u) return 0u;
    const auto quick = runtime.CombatInactive();
    const auto object = runtime.Allocate(
        quick ? QuickAttackTask::kSize : CheckProtectTask::kSize);
    if (object == 0u) return 0u;
    ActionConstructionGuard guard{runtime, object};

    runtime.InitializeContainer(object + 4u);
    guard.container_initialized = true;
    runtime.WriteByte(object + 20u, mode);
    guard.name_initialized = true;
    if (quick) {
        runtime.SetNameErrorFlag(object + 20u, 0u);
        constexpr std::string_view name = "QuickAttack";
        if (runtime.AllocateName(object + 20u,
                                 static_cast<std::uint32_t>(name.size()),
                                 1u))
            runtime.CopyName(object + 20u, name);
        runtime.WriteWord(object + 48u, target);
        runtime.WriteWord(object + 36u, 0u);
        runtime.WriteByte(object + 40u, 1u);
        runtime.WriteWord(object + 44u, source);
        runtime.WriteByte(object + 52u, 1u);
        runtime.WriteByte(object + 53u, 1u);
        runtime.WriteWord(object, QuickAttackTask::kVtable);
    } else {
        runtime.WriteWord(object + 24u, 0u);
        runtime.WriteWord(object + 28u, 0u);
        runtime.WriteWord(object + 32u, 0u);
        constexpr std::string_view name = "CheckProtect";
        if (runtime.AllocateName(object + 20u,
                                 static_cast<std::uint32_t>(name.size()),
                                 1u))
            runtime.CopyName(object + 20u, name);
        runtime.WriteWord(object + 44u, source);
        runtime.WriteByte(object + 40u, 1u);
        runtime.WriteWord(object + 36u, 0u);
        runtime.WriteWord(object + 48u, target);
        runtime.WriteWord(object + 52u, 0u);
        runtime.WriteByte(object + 56u, mode);
        runtime.WriteWord(object, CheckProtectTask::kVtable);
    }
    guard.object = 0u;
    return object;
}

namespace {

std::uint32_t CreateNamedCombatAction(
    const std::uint32_t magic, const std::uint32_t source,
    const std::uint32_t target, ICombatActionFactoryRuntime& runtime,
    const std::string_view quick_name, const std::string_view active_name,
    const std::uint32_t quick_vtable,
    const std::uint32_t active_vtable) {
    if (magic == 0u || source == 0u || target == 0u) return 0u;
    const auto quick = runtime.CombatInactive();
    const auto object = runtime.Allocate(56u);
    if (object == 0u) return 0u;
    ActionConstructionGuard guard{runtime, object};

    runtime.InitializeContainer(object + 4u);
    guard.container_initialized = true;
    runtime.WriteByte(object + 20u, static_cast<std::uint8_t>(source));
    guard.name_initialized = true;
    if (quick) {
        runtime.SetNameErrorFlag(object + 20u, 0u);
        if (runtime.AllocateName(object + 20u,
                                 static_cast<std::uint32_t>(quick_name.size()),
                                 1u))
            runtime.CopyName(object + 20u, quick_name);
        runtime.WriteWord(object + 48u, target);
        runtime.WriteWord(object + 36u, 0u);
        runtime.WriteByte(object + 40u, 1u);
        runtime.WriteWord(object + 44u, source);
        runtime.WriteWord(object + 52u, magic);
        runtime.WriteWord(object, quick_vtable);
    } else {
        runtime.WriteWord(object + 24u, 0u);
        runtime.WriteWord(object + 28u, 0u);
        runtime.WriteWord(object + 32u, 0u);
        if (runtime.AllocateName(object + 20u,
                                 static_cast<std::uint32_t>(active_name.size()),
                                 1u))
            runtime.CopyName(object + 20u, active_name);
        runtime.WriteWord(object + 36u, 0u);
        runtime.WriteByte(object + 40u, 1u);
        runtime.WriteWord(object + 44u, source);
        runtime.WriteWord(object + 48u, target);
        runtime.WriteWord(object + 52u, magic);
        runtime.WriteWord(object, active_vtable);
    }
    guard.object = 0u;
    return object;
}

}  // namespace

std::uint32_t CombatMagicActionFactory::Create(
    const std::uint32_t magic, const std::uint32_t source,
    const std::uint32_t target, ICombatActionFactoryRuntime& runtime) {
    return CreateNamedCombatAction(
        magic, source, target, runtime, "QuickCastMagic", "CastMagic",
        QuickCastMagicSequenceTask::kVtable, CastMagicSequenceTask::kVtable);
}

std::uint32_t CombatStuntActionFactory::Create(
    const std::uint32_t stunt, const std::uint32_t source,
    const std::uint32_t target, ICombatActionFactoryRuntime& runtime) {
    return CreateNamedCombatAction(
        stunt, source, target, runtime, "QuickStunt", "Stunt",
        QuickStuntSequenceTask::kVtable, StuntSequenceTask::kVtable);
}

std::uint32_t CombatUsePropertyActionFactory::Create(
    const std::uint32_t property, const std::uint32_t source,
    const std::uint32_t target, ICombatActionFactoryRuntime& runtime) {
    return CreateNamedCombatAction(
        property, source, target, runtime,
        "QuickUseProperty", "UseProperty",
        QuickUsePropertySequenceTask::kVtable,
        UsePropertySequenceTask::kVtable);
}

std::uint32_t CombatDefendActionFactory::Create(
    const std::uint32_t actor, ICombatActionFactoryRuntime& runtime) {
    if (actor == 0u) return 0u;
    const auto quick = runtime.CombatInactive();
    const auto object = runtime.Allocate(DefendSequenceTask::kSize);
    if (object == 0u) return 0u;
    ActionConstructionGuard guard{runtime, object};

    runtime.InitializeContainer(object + 4u);
    guard.container_initialized = true;
    runtime.WriteByte(object + 20u, static_cast<std::uint8_t>(actor));
    guard.name_initialized = true;
    if (quick) {
        runtime.SetNameErrorFlag(object + 20u, 0u);
    } else {
        runtime.WriteWord(object + 24u, 0u);
        runtime.WriteWord(object + 28u, 0u);
        runtime.WriteWord(object + 32u, 0u);
    }
    constexpr std::string_view name = "Defend";
    if (runtime.AllocateName(object + 20u,
                             static_cast<std::uint32_t>(name.size()), 1u))
        runtime.CopyName(object + 20u, name);
    runtime.WriteWord(object + 36u, 0u);
    runtime.WriteByte(object + 40u, 1u);
    runtime.WriteWord(object + 44u, actor);
    runtime.WriteWord(object, DefendSequenceTask::kVtable);
    guard.object = 0u;
    return object;
}

std::uint32_t CombatFleeActionFactory::Create(
    const std::uint32_t actor, ICombatActionFactoryRuntime& runtime) {
    if (actor == 0u) return 0u;
    const auto quick = runtime.CombatInactive();
    const auto object = runtime.Allocate(FleeSequenceTask::kSize);
    if (object == 0u) return 0u;
    ActionConstructionGuard guard{runtime, object};

    runtime.InitializeContainer(object + 4u);
    guard.container_initialized = true;
    runtime.WriteByte(object + 20u, static_cast<std::uint8_t>(actor));
    guard.name_initialized = true;
    if (quick) {
        runtime.SetNameErrorFlag(object + 20u, 0u);
    } else {
        runtime.WriteWord(object + 24u, 0u);
        runtime.WriteWord(object + 28u, 0u);
        runtime.WriteWord(object + 32u, 0u);
    }
    constexpr std::string_view name = "Flee";
    if (runtime.AllocateName(object + 20u,
                             static_cast<std::uint32_t>(name.size()), 1u)) {
        runtime.CopyName(object + 20u, name);
        if (quick)
            runtime.SetNameSize(object + 20u,
                                static_cast<std::uint32_t>(name.size()));
    }
    runtime.WriteWord(object + 36u, 0u);
    runtime.WriteByte(object + 40u, 1u);
    runtime.WriteWord(object + 44u, actor);
    runtime.WriteWord(object, quick ? QuickFleeSequenceTask::kVtable
                                    : FleeSequenceTask::kVtable);
    guard.object = 0u;
    return object;
}

namespace {

std::uint32_t CreatePairedActorAction(
    const std::uint32_t source, const std::uint32_t target,
    ICombatActionFactoryRuntime& runtime,
    const std::string_view quick_name, const std::string_view active_name,
    const std::uint32_t quick_vtable,
    const std::uint32_t active_vtable) {
    if (source == 0u || target == 0u) return 0u;
    const auto quick = runtime.CombatInactive();
    const auto object = runtime.Allocate(52u);
    if (object == 0u) return 0u;
    ActionConstructionGuard guard{runtime, object};

    runtime.InitializeContainer(object + 4u);
    guard.container_initialized = true;
    runtime.WriteByte(object + 20u, static_cast<std::uint8_t>(source));
    guard.name_initialized = true;
    if (quick) {
        runtime.SetNameErrorFlag(object + 20u, 0u);
        if (runtime.AllocateName(object + 20u,
                                 static_cast<std::uint32_t>(quick_name.size()),
                                 1u))
            runtime.CopyName(object + 20u, quick_name);
        runtime.WriteWord(object + 48u, target);
        runtime.WriteWord(object + 36u, 0u);
        runtime.WriteByte(object + 40u, 1u);
        runtime.WriteWord(object + 44u, source);
        runtime.WriteWord(object, quick_vtable);
    } else {
        runtime.WriteWord(object + 24u, 0u);
        runtime.WriteWord(object + 28u, 0u);
        runtime.WriteWord(object + 32u, 0u);
        if (runtime.AllocateName(object + 20u,
                                 static_cast<std::uint32_t>(active_name.size()),
                                 1u))
            runtime.CopyName(object + 20u, active_name);
        runtime.WriteWord(object + 36u, 0u);
        runtime.WriteByte(object + 40u, 1u);
        runtime.WriteWord(object + 44u, source);
        runtime.WriteWord(object + 48u, target);
        runtime.WriteWord(object, active_vtable);
    }
    guard.object = 0u;
    return object;
}

}  // namespace

std::uint32_t CombatDeadActionFactory::Create(
    const std::uint32_t source, const std::uint32_t target,
    ICombatActionFactoryRuntime& runtime) {
    return CreatePairedActorAction(
        source, target, runtime, "QuickDead", "Dead",
        QuickDeadSequenceTask::kVtable, DeadSequenceTask::kVtable);
}

std::uint32_t CombatFendOffActionFactory::Create(
    const std::uint32_t source, const std::uint32_t target,
    ICombatActionFactoryRuntime& runtime) {
    return CreatePairedActorAction(
        source, target, runtime, "QuickFendOff", "FendOff",
        QuickFendOffSequenceTask::kVtable, FendOffSequenceTask::kVtable);
}

std::uint32_t CombatMissActionFactory::Create(
    const std::uint32_t source, const std::uint32_t target,
    ICombatActionFactoryRuntime& runtime) {
    return CreatePairedActorAction(
        source, target, runtime, "quickMiss", "MissTask",
        QuickMissSequenceTask::kVtable, MissSequenceTask::kVtable);
}

std::uint32_t CombatEndActionFactory::Create(
    const std::uint32_t actor, ICombatActionFactoryRuntime& runtime) {
    if (actor == 0u) return 0u;
    const auto inactive = runtime.CombatInactive();
    const auto object = runtime.Allocate(CombatEndActionTask::kSize);
    if (object == 0u) return 0u;
    ActionConstructionGuard guard{runtime, object};

    runtime.InitializeContainer(object + 4u);
    guard.container_initialized = true;
    runtime.WriteByte(object + 20u, static_cast<std::uint8_t>(actor));
    guard.name_initialized = true;
    if (inactive) {
        runtime.SetNameErrorFlag(object + 20u, 0u);
    } else {
        runtime.WriteWord(object + 24u, 0u);
        runtime.WriteWord(object + 28u, 0u);
        runtime.WriteWord(object + 32u, 0u);
    }
    constexpr std::string_view name = "EndAction";
    if (runtime.AllocateName(object + 20u,
                             static_cast<std::uint32_t>(name.size()), 1u)) {
        runtime.CopyName(object + 20u, name);
        runtime.SetNameSize(object + 20u,
                            static_cast<std::uint32_t>(name.size()));
    }
    runtime.WriteWord(object + 36u, 0u);
    runtime.WriteByte(object + 40u, 1u);
    runtime.WriteWord(object + 44u, actor);
    runtime.WriteWord(object, CombatEndActionTask::kVtable);
    guard.object = 0u;
    return object;
}

namespace {

std::uint32_t CreateActiveActorAction(
    const std::uint32_t actor, ICombatActionFactoryRuntime& runtime,
    const std::string_view name, const std::uint32_t vtable) {
    const auto object = runtime.Allocate(48u);
    if (object == 0u) return 0u;
    ActionConstructionGuard guard{runtime, object};

    runtime.InitializeContainer(object + 4u);
    guard.container_initialized = true;
    runtime.WriteByte(object + 20u, static_cast<std::uint8_t>(actor));
    runtime.WriteWord(object + 24u, 0u);
    runtime.WriteWord(object + 28u, 0u);
    runtime.WriteWord(object + 32u, 0u);
    guard.name_initialized = true;
    runtime.SetNameErrorFlag(object + 20u, 1u);
    runtime.ReallocateName(object + 20u,
                           static_cast<std::uint32_t>(name.size()));
    runtime.CopyName(object + 20u, name);
    runtime.SetNameSize(object + 20u,
                        static_cast<std::uint32_t>(name.size()));
    runtime.WriteWord(object + 36u, 0u);
    runtime.WriteByte(object + 40u, 1u);
    runtime.WriteWord(object + 44u, actor);
    runtime.WriteWord(object, vtable);
    guard.object = 0u;
    return object;
}

}  // namespace

std::uint32_t CombatWinActionFactory::Create(
    const std::uint32_t actor, ICombatActionFactoryRuntime& runtime) {
    if (actor == 0u || runtime.CombatInactive()) return 0u;
    return CreateActiveActorAction(actor, runtime, "WinAction",
                                   WinActionTask::kVtable);
}

std::uint32_t CombatLevelUpActionFactory::Create(
    const std::uint32_t actor,
    ICombatLevelUpActionFactoryRuntime& runtime) {
    if (actor == 0u || runtime.ReadWord(actor + 828u) == 7u ||
        runtime.CombatInactive()) return 0u;
    return CreateActiveActorAction(actor, runtime, "LevelUp",
                                   LevelUpActionTask::kVtable);
}

std::uint32_t CombatDefendActionInitiator::Create(
    const std::uint32_t actor,
    ICombatDefendActionInitiatorRuntime& runtime) {
    if (!runtime.IsActorReady(actor)) return 0u;
    return CombatDefendActionFactory::Create(actor, runtime);
}

std::uint32_t CombatFleeActionInitiator::Create(
    const std::uint32_t actor,
    ICombatFleeActionInitiatorRuntime& runtime) {
    if (!runtime.IsActorReady(actor)) return 0u;
    return CombatFleeActionFactory::Create(actor, runtime);
}

namespace {

std::uint32_t CreateFendOffDeathTask(
    const std::uint32_t task, IFendOffDeathFactoryRuntime& runtime,
    const std::string_view name, const std::uint32_t vtable) {
    if (!runtime.ActorCheckStateThree(runtime.ReadWord(task + 48u)))
        return 0u;
    const auto object = runtime.Allocate(52u);
    if (object == 0u) return 0u;
    ActionConstructionGuard guard{runtime, object};

    const auto former_target = runtime.ReadWord(task + 48u);
    const auto former_source = runtime.ReadWord(task + 44u);
    runtime.InitializeContainer(object + 4u);
    guard.container_initialized = true;
    // The native function copies an unwritten stack byte into this header.
    runtime.WriteByte(object + 20u, 0u);
    runtime.WriteWord(object + 24u, 0u);
    runtime.WriteWord(object + 28u, 0u);
    runtime.WriteWord(object + 32u, 0u);
    guard.name_initialized = true;
    runtime.SetNameErrorFlag(object + 20u, 1u);
    runtime.ReallocateName(object + 20u,
                           static_cast<std::uint32_t>(name.size()));
    runtime.CopyName(object + 20u, name);
    runtime.SetNameSize(object + 20u,
                        static_cast<std::uint32_t>(name.size()));
    runtime.WriteWord(object + 36u, 0u);
    runtime.WriteWord(object + 48u, former_source);
    runtime.WriteByte(object + 40u, 1u);
    runtime.WriteWord(object + 44u, former_target);
    runtime.WriteWord(object, vtable);
    guard.object = 0u;
    return object;
}

}  // namespace

std::uint32_t FendOffSequenceTask::CreateDeathTask(
    const std::uint32_t task, IFendOffDeathFactoryRuntime& runtime) {
    return CreateFendOffDeathTask(task, runtime, "Dead",
                                  DeadSequenceTask::kVtable);
}

std::uint32_t QuickFendOffSequenceTask::CreateDeathTask(
    const std::uint32_t task, IFendOffDeathFactoryRuntime& runtime) {
    return CreateFendOffDeathTask(task, runtime, "QuickDead",
                                  QuickDeadSequenceTask::kVtable);
}

}  // namespace pal4
