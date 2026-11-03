#include "pal4/actor_manager_combat_prepare.h"

#include <array>

namespace pal4 {

namespace {

void CreateEffect(const std::uint32_t manager,
                  const std::uint32_t name, const bool store,
                  IActorManagerCombatPrepareRuntime& runtime) {
    auto handle = runtime.CreateEffect(
        manager, name, ActorManagerCombatPrepare::kPosition, 1u, 0u);
    if (store) {
        runtime.WriteWord(manager + 384u, handle.id);
        runtime.WriteWord(manager + 388u, handle.owner);
    }
    runtime.ResetTemporaryEffect(handle);
}

void CreateRandomEffect(
    const std::uint32_t manager,
    const std::array<std::uint32_t, 3>& literals,
    const bool store,
    IActorManagerCombatPrepareRuntime& runtime) {
    std::array<std::uint32_t, 3> texts{};
    try {
        for (std::uint32_t index = 0u; index < texts.size(); ++index)
            texts[index] = runtime.CreateText(literals[index]);
        const auto chosen = texts[runtime.RandomIndex(3u)];
        CreateEffect(manager,
                     chosen == 0u ? ActorManagerCombatPrepare::kEmptyText
                                  : chosen,
                     store, runtime);
    } catch (...) {
        for (std::uint32_t index = 3u; index != 0u; --index)
            if (texts[index - 1u] != 0u)
                runtime.ReleaseText(texts[index - 1u]);
        throw;
    }
    for (std::uint32_t index = 3u; index != 0u; --index)
        if (texts[index - 1u] != 0u)
            runtime.ReleaseText(texts[index - 1u]);
}

}  // namespace

std::uint32_t ActorManagerCombatPrepare::Handle(
    const std::uint32_t manager, const std::uint32_t command_address,
    IActorManagerCombatPrepareRuntime& runtime) {
    const auto first_combat = runtime.CombatSystem();
    if (!runtime.CombatInactive(first_combat)) {
        const auto combat = runtime.CombatSystem();
        if (runtime.ReadWord(combat + 492u) != 0u) {
            const auto name = runtime.ReadWord(combat + 488u);
            CreateEffect(manager, name == 0u ? kEmptyText : name,
                         false, runtime);
        } else {
            CreateRandomEffect(manager,
                               {0x8b6784u, 0x8b677cu, 0x8b6774u},
                               false, runtime);
        }
    }

    const auto second_combat = runtime.CombatSystem();
    if (runtime.CombatInactive(second_combat)) {
        const auto combat = runtime.CombatSystem();
        runtime.WriteWord(combat + 508u, 2u);
        const auto key = runtime.ReadWord(kCombatStartKey);
        return static_cast<std::uint8_t>(runtime.DispatchAddress(
            runtime.EventRouter(), key, command_address, 1u));
    }

    const auto command = runtime.ReadWord(command_address);
    if (command == 1u || command == 2u) {
        CreateEffect(manager, command == 1u ? 0x8b6770u : 0x8b676cu,
                     true, runtime);
    } else {
        const auto combat = runtime.CombatSystem();
        if (runtime.ReadWord(combat + 476u) != 0u) {
            const auto name = runtime.ReadWord(combat + 472u);
            CreateEffect(manager, name == 0u ? kEmptyText : name,
                         true, runtime);
        } else {
            CreateRandomEffect(manager,
                               {0x8b6768u, 0x8b6764u, 0x8b6760u},
                               true, runtime);
        }
    }

    if (runtime.EffectStillActive(manager + 384u)) return 1u;
    const auto combat = runtime.CombatSystem();
    runtime.WriteWord(combat + 508u, 2u);
    const auto key = runtime.ReadWord(kCombatStartKey);
    return static_cast<std::uint8_t>(runtime.DispatchByte(
        runtime.EventRouter(), key, 0u, 1u));
}

std::uint32_t ActorManagerCombatPrepare::HandleTypeOne(
    const std::uint32_t manager, const std::uint32_t command_address,
    IActorManagerCombatPrepareRuntime& runtime) {
    if (runtime.CombatInactive(runtime.CombatSystem())) return 1u;
    if (runtime.ReadWord(command_address) != 1u) return command_address;
    CreateEffect(manager, 0x8b6788u, false, runtime);
    return 0u;
}

}  // namespace pal4
