#include "pal4/game_data_upgrade_registry.h"

#include <utility>

namespace pal4 {

std::uint32_t GameDataUpgradeRegistry::Initialize(
    std::uint8_t first_seed, std::uint8_t second_seed,
    IGameDataUpgradeRegistryRuntime& runtime) {
    if (!storage_initialized_) {
        storage_initialized_ = true;
        runtime.InitializeStorage(first_seed, second_seed);
        runtime.RegisterCleanup();
    }
    runtime.ResetTemporaryName(second_seed);
    if (!records_initialized_) {
        records_initialized_ = true;
        const auto append = [&](GameDataProperty record) {
            runtime.AppendRecord(record);
            records_.push_back(std::move(record));
        };
        append({88, 0, GameDataPropertyKind::numeric,
                "ID" + runtime.ErrorSuffix(), 0});
        for (std::uint32_t index = 0; index < 22; ++index) {
            const auto address = 0x8ee850 + index * 16;
            append({index * 4, address, GameDataPropertyKind::text,
                    runtime.SourceName(address) + runtime.ErrorSuffix(), 0});
        }
        runtime.Sort(records_);
    }
    runtime.ReleaseTemporaryName();
    return kCollectionAddress;
}

void GameDataUpgradeRegistry::Cleanup(
    IGameDataUpgradeRegistryRuntime& runtime) {
    runtime.CleanupStorage();
    records_.clear();
}

std::uint8_t GameDataUpgradeRegistry::Process(
    std::uint32_t object, std::uint8_t first_seed,
    std::uint8_t second_seed, IGameDataUpgradeRegistryRuntime& runtime) {
    const auto collection = Initialize(first_seed, second_seed, runtime);
    return static_cast<std::uint8_t>(
        runtime.ProcessCollection(object, collection));
}

}  // namespace pal4
