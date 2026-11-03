#include "pal4/game_data_monster_table_registry.h"

#include <utility>

namespace pal4 {

const std::vector<GameDataFieldDescriptor>&
GameDataMonsterTableRegistry::Initialize(
    IGameDataMonsterTableRegistryRuntime& runtime) {
    if (!globals_initialized_) {
        globals_initialized_ = true;
        runtime.InitializeGlobalStorage();
        runtime.RegisterGlobalCleanup();
    }
    if (entries_initialized_) return fields_;
    entries_initialized_ = true;

    runtime.AppendField(fields_,
                        {400, "MONSTER_ID" + runtime.ErrorSuffix(),
                         GameDataFieldKind::numeric, 0x843368, 24, 0, 0});
    runtime.ImportStructureFields(fields_, 0);
    runtime.ImportMonsterFields(fields_, 336);
    runtime.Sort(fields_);
    return fields_;
}

void GameDataMonsterTableRegistry::Cleanup(
    IGameDataMonsterTableRegistryRuntime& runtime) {
    runtime.CleanupGlobalStorage();
    fields_.clear();
}

std::uint8_t GameDataMonsterTableRegistry::Process(
    std::uint32_t object, IGameDataMonsterTableRegistryRuntime& runtime) {
    return runtime.ProcessCollection(object, Initialize(runtime));
}

}  // namespace pal4
