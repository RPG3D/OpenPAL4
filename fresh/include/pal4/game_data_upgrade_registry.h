#pragma once

#include "pal4/game_data_property_collection_runtime.h"

#include <cstdint>
#include <vector>

namespace pal4 {

class IGameDataUpgradeRegistryRuntime
    : public IGameDataPropertyCollectionRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ProcessCollection(
        std::uint32_t object, std::uint32_t collection) = 0;
};

class GameDataUpgradeRegistry final {
public:
    static constexpr std::uint32_t kCollectionAddress = 0x8ee818;

    // IDA 0x4cf750: register ID and 22 upgrade parameter properties once.
    [[nodiscard]] std::uint32_t Initialize(
        std::uint8_t first_seed, std::uint8_t second_seed,
        IGameDataUpgradeRegistryRuntime& runtime);
    // IDA 0x4cfa00: CRT callback cleans this collection.
    void Cleanup(IGameDataUpgradeRegistryRuntime& runtime);
    // IDA 0x4cfa10: initialize then process through Event_ProcessCollection2.
    [[nodiscard]] std::uint8_t Process(
        std::uint32_t object, std::uint8_t first_seed,
        std::uint8_t second_seed, IGameDataUpgradeRegistryRuntime& runtime);

    [[nodiscard]] bool StorageInitialized() const { return storage_initialized_; }
    [[nodiscard]] bool RecordsInitialized() const { return records_initialized_; }
    [[nodiscard]] const std::vector<GameDataProperty>& Records() const {
        return records_;
    }

private:
    bool storage_initialized_ = false;
    bool records_initialized_ = false;
    std::vector<GameDataProperty> records_;
};

}  // namespace pal4
