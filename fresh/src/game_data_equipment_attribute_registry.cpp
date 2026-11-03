#include "pal4/game_data_equipment_attribute_registry.h"

#include <utility>

namespace pal4 {

std::uint32_t GameDataEquipmentAttributeRegistry::Initialize(
    std::uint8_t first_seed, std::uint8_t second_seed,
    IGameDataEquipmentAttributeRegistryRuntime& runtime) {
    if (!storage_initialized_) {
        storage_initialized_ = true;
        runtime.InitializeStorage(first_seed, second_seed);
        runtime.RegisterCleanup();
    }
    runtime.ResetTemporaryName(second_seed);
    if (!records_initialized_) {
        records_initialized_ = true;
        runtime.ImportBaseCollection(kCollectionAddress);
        const auto append = [&](GameDataProperty record) {
            runtime.AppendRecord(record);
            records_.push_back(std::move(record));
        };
        append({144, 0, GameDataPropertyKind::numeric,
                "ID" + runtime.ErrorSuffix(), 0});
        append({148, 0, GameDataPropertyKind::numeric,
                "TYPE" + runtime.ErrorSuffix(), 0});
        for (std::uint32_t index = 0; index < 13; ++index) {
            const auto address = 0x8ee9f0 + index * 16;
            append({152 + index * 16, address, GameDataPropertyKind::text,
                    runtime.SourceName(address) + runtime.ErrorSuffix(), 0});
        }
        for (std::uint32_t index = 0; index < 4; ++index) {
            const auto address = 0x8ef9c8 + index * 4;
            append({360 + index * 4, address, GameDataPropertyKind::numeric,
                    runtime.SourceName(address) + "_LV_LMT", 0});
        }
        for (std::uint32_t index = 0; index < 6; ++index) {
            const auto address = 0x8eeac0 + index * 16;
            append({376 + index * 4, address, GameDataPropertyKind::numeric,
                    runtime.SourceName(address) + runtime.ErrorSuffix(), 0});
        }
        runtime.Sort(records_);
    }
    runtime.ReleaseTemporaryName();
    return kCollectionAddress;
}

void GameDataEquipmentAttributeRegistry::Cleanup(
    IGameDataEquipmentAttributeRegistryRuntime& runtime) {
    runtime.CleanupStorage();
    records_.clear();
}

std::uint8_t GameDataEquipmentAttributeRegistry::Process(
    std::uint32_t object, std::uint8_t first_seed,
    std::uint8_t second_seed,
    IGameDataEquipmentAttributeRegistryRuntime& runtime) {
    const auto collection = Initialize(first_seed, second_seed, runtime);
    return static_cast<std::uint8_t>(
        runtime.ProcessCollection(object, collection));
}

bool GameDataEquipmentAttributeRegistry::TypeInRange(
    std::uint32_t object,
    IGameDataEquipmentAttributeRegistryRuntime& runtime) {
    return runtime.ReadWord(object + 148) <= 6u;
}

std::uint32_t GameDataEquipmentAttributeRegistry::TypeIndex(
    std::uint32_t object,
    IGameDataEquipmentAttributeRegistryRuntime& runtime) {
    const auto type = runtime.ReadWord(object + 148);
    return TypeIndexForType(type);
}

}  // namespace pal4
