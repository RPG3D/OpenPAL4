#pragma once

#include "pal4/native_player_role_load_runtime.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/pal_actor_control.h"
#include "pal4/player_manager_object.h"
#include "pal4/player_manager_operations.h"
#include "pal4/script_builtin_handlers.h"

namespace pal4 {

// The caller owns the Role catalog and slot text storage through the load;
// slot text storage must survive the full player-manager lifetime.
class NativeScriptNewGameRuntime final : public IScriptNewGameRuntime {
public:
    NativeScriptNewGameRuntime(const NativeRoleRecordCatalog& roles,
                               PlayerManagerObjectState& players,
                               PalActorControlState& actor_control,
                               IPlayerManagerOperationsRuntime& operations,
                               IPlayerManagerLoadRuntime& load_host,
                               NativePlayerSlotCopyRuntime& copy_runtime,
                               IPalActorControlRuntime& actor_runtime,
                               std::uint32_t actor_event_context,
                               IPlayerActorAssetLoader* actor_assets = nullptr,
                               IGameResourceManagerRuntime* transforms = nullptr,
                               NativePkFileManagerRuntime* packages = nullptr,
                               IPlayerManagerOperationsRuntime* loaded_operations = nullptr);

    std::uint32_t InitializePlayerManager() override;
    void ResetAllPlayerStates(std::uint32_t manager) override;
    void LoadAndProcessPlayerData(std::uint32_t manager) override;
    std::uint32_t ActorControlInstance() override;
    void ShutdownActorControl(std::uint32_t control) override;
    int InitializeActorControl(std::uint32_t control) override;

private:
    PlayerManagerObjectState& players_;
    PalActorControlState& actor_control_;
    IPlayerManagerOperationsRuntime& operations_;
    NativePlayerRoleLoadRuntime role_runtime_;
    IPalActorControlRuntime& actor_runtime_;
    std::uint32_t actor_event_context_;
};

}  // namespace pal4
