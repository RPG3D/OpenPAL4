#include "pal4/actor_manager_property_registry.h"

#include <initializer_list>
#include <new>

namespace pal4 {
namespace {

struct RegistrySpec {
    std::uint32_t address;
    std::uint32_t cleanup;
    std::uint32_t db_key_offset;
    std::uint32_t user_name_vtable;
    std::uint32_t getter_vtable;
    std::uint32_t game_data_offset;
};

constexpr RegistrySpec kCombatSpec{
    ActorManagerPropertyRegistry::kAddress,
    ActorManagerPropertyRegistry::kCleanup,
    960u,
    ActorManagerPropertyRegistry::kUNameVtable,
    ActorManagerPropertyRegistry::kGetterVtable,
    896u};
constexpr RegistrySpec kPlayerSpec{
    ActorManagerPlayerPropertyRegistry::kAddress,
    ActorManagerPlayerPropertyRegistry::kCleanup,
    ActorManagerPlayerPropertyRegistry::kDbKeyOffset,
    ActorManagerPlayerPropertyRegistry::kUNameVtable,
    ActorManagerPlayerPropertyRegistry::kGetterVtable,
    1112u};

std::uint32_t InitializeRegistry(
    bool& constructed, bool& initialized, const RegistrySpec& spec,
    IActorManagerPropertyRegistryRuntime& runtime,
    IActorManagerPlayerPropertyRegistryRuntime* player_runtime) {
    if (!constructed) {
        constructed = true;
        for (const auto offset : {0u, 4u, 8u, 12u, 16u, 20u, 24u, 28u,
                                  32u, 36u, 40u, 44u})
            runtime.WriteWord(spec.address + offset, 0u);
        runtime.RegisterCleanup(spec.cleanup);
    }
    if (initialized) return spec.address;
    initialized = true;

    ActorManagerRegistryText db_text;
    try {
        const auto db_key = runtime.Allocate(24u);
        if (!db_key) throw std::bad_alloc();
        runtime.WriteWord(db_key, ActorManagerPropertyRegistry::kDbKeyBaseVtable);
        runtime.InitializeText(db_key + 8u);
        runtime.WriteWord(db_key, ActorManagerPropertyRegistry::kDbKeyVtable);
        runtime.SetText(db_text, "DBKey");
        runtime.AppendEmptyText(db_text);
        runtime.AssignText(db_key + 8u, db_text);
        runtime.WriteWord(db_key + 4u, spec.db_key_offset);
        runtime.PushBack(spec.address, db_key);

        const auto user_name = runtime.Allocate(36u);
        if (!user_name) throw std::bad_alloc();
        runtime.WriteWord(user_name, ActorManagerPropertyRegistry::kUNameBaseVtable);
        runtime.InitializeOwnedSlot(user_name + 4u);
        runtime.InitializeText(user_name + 20u);
        runtime.WriteWord(user_name, spec.user_name_vtable);
        ActorManagerRegistryText name_text;
        try {
            runtime.SetText(name_text, "UName");
            runtime.AssignText(user_name + 20u, name_text);
        } catch (...) {
            runtime.ReleaseText(name_text);
            throw;
        }
        runtime.ReleaseText(name_text);

        const auto getter = runtime.Allocate(16u);
        if (getter) {
            runtime.WriteWord(getter, spec.getter_vtable);
            runtime.WriteWord(getter + 8u,
                              ActorManagerPropertyRegistry::kGetterCallback);
            runtime.WriteWord(getter + 12u, 0u);
        }
        runtime.AssignOwnedSlot(user_name + 4u, getter);
        runtime.ReleaseOwnedSlot(getter);

        const auto setter = runtime.Allocate(16u);
        if (setter) runtime.ConstructSetter(
            setter, ActorManagerPropertyRegistry::kSetterCallback);
        runtime.AssignOwnedSlot(user_name + 12u, setter);
        if (setter) runtime.ReleaseOwnedSlot(setter);
        runtime.PushBack(spec.address, user_name);

        runtime.ProcessAll(spec.address, runtime.GameData(),
                           spec.game_data_offset);
        runtime.ProcessAll(spec.address, runtime.GameObject(), 0u);
        if (player_runtime)
            runtime.ProcessAll(spec.address,
                               player_runtime->ScriptEventSystem(), 896u);
        runtime.Sort(spec.address);
    } catch (...) {
        runtime.ReleaseText(db_text);
        throw;
    }
    runtime.ReleaseText(db_text);
    return spec.address;
}

}  // namespace

std::uint32_t ActorManagerPropertyRegistry::ForwardInitialize(
    IActorManagerPropertyRegistryRuntime& runtime) {
    return Initialize(runtime);
}

std::uint32_t ActorManagerPropertyRegistry::Initialize(
    IActorManagerPropertyRegistryRuntime& runtime) {
    return InitializeRegistry(constructed_, initialized_, kCombatSpec,
                              runtime, nullptr);
}

void ActorManagerPropertyRegistry::Cleanup(
    IActorManagerPropertyRegistryRuntime& runtime) {
    runtime.CleanupCollection(kAddress);
}

std::uint32_t ActorManagerPlayerPropertyRegistry::ForwardInitialize(
    IActorManagerPlayerPropertyRegistryRuntime& runtime) {
    return Initialize(runtime);
}

std::uint32_t ActorManagerPlayerPropertyRegistry::Initialize(
    IActorManagerPlayerPropertyRegistryRuntime& runtime) {
    return InitializeRegistry(constructed_, initialized_, kPlayerSpec,
                              runtime, &runtime);
}

void ActorManagerPlayerPropertyRegistry::Cleanup(
    IActorManagerPlayerPropertyRegistryRuntime& runtime) {
    runtime.CleanupCollection(kAddress);
}

}  // namespace pal4
