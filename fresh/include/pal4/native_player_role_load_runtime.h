#pragma once

#include "pal4/player_actor_asset_loader.h"
#include "pal4/player_manager_loader.h"
#include "pal4/player_manager_slot_data_copier.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameResourceManagerRuntime;
class IPlayerManagerOperationsRuntime;
class NativePkFileManagerRuntime;
class NativeRoleRecordCatalog;

// Supplies original Role records to the rewritten PlayerManagerLoader.
// Keep the caller-owned Role catalog alive through the load; the native slot
// copier owns destination strings after each Role record has been copied.
// Loaded actor assets stay with their owner on success and are rolled back
// if the load or postprocessing fails.
class NativePlayerRoleLoadRuntime final : public IPlayerManagerLoadRuntime {
public:
    NativePlayerRoleLoadRuntime(const NativeRoleRecordCatalog& roles,
                                IPlayerManagerLoadRuntime& host,
                                IPlayerManagerSlotCopyRuntime& copier,
                                IPlayerActorAssetLoader* actors = nullptr,
                                IGameResourceManagerRuntime* transforms = nullptr,
                                NativePkFileManagerRuntime* packages = nullptr,
                                IPlayerManagerOperationsRuntime* loaded_operations = nullptr);

    std::uint32_t Log(std::uint32_t line) override;
    void ProcessScripts() override;
    std::uint32_t GameDB() override;
    std::uint32_t FindRole(std::uint32_t role_table,
                           std::uint32_t index) override;
    void CopySlotData(std::uint32_t slot, std::uint32_t record) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void CallLoadActor(std::uint32_t slot, std::uint32_t virtual_slot,
                       std::uint32_t name, std::uint32_t model,
                       std::uint32_t enabled) override;
    void CallVectorMethod(std::uint32_t slot, std::uint32_t virtual_slot,
                          PlayerManagerVector& vector,
                          std::uint32_t option) override;
    void SetStateFlag(std::uint32_t slot, std::uint32_t value) override;
    void CallScriptIndex(std::uint32_t script, std::uint32_t virtual_slot,
                         std::uint32_t index) override;
    void ProcessLoadedData(std::uint32_t manager) override;
    void CleanupScripts() override;
    void RollbackActors() noexcept;
    void CommitActors() noexcept;

private:
    const NativeRoleRecordCatalog& roles_;
    IPlayerManagerLoadRuntime& host_;
    IPlayerManagerSlotCopyRuntime& copier_;
    IPlayerActorAssetLoader* actors_;
    IGameResourceManagerRuntime* transforms_;
    NativePkFileManagerRuntime* packages_;
    IPlayerManagerOperationsRuntime* loaded_operations_;
    bool primary_script_active_{};
    bool host_script_active_{};
    std::uint32_t last_database_{};
    std::array<std::uint32_t, 4> loaded_actors_{};
    std::size_t loaded_actor_count_{};
};

}  // namespace pal4
