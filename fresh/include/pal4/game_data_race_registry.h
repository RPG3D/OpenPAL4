#pragma once

#include "pal4/game_data_property_collection_runtime.h"

#include <cstdint>
#include <vector>

namespace pal4 {

class GameDataRaceRegistry final {
public:
    static constexpr std::uint32_t kCollectionAddress = 0x8ee7e4;

    // IDA 0x4cebe0: one-time storage and 36 race-related properties.
    [[nodiscard]] std::uint32_t Initialize(
        std::uint8_t first_seed, std::uint8_t second_seed,
        IGameDataPropertyCollectionRuntime& runtime);
    // IDA 0x4cf1d0: CRT callback cleans the same collection.
    void Cleanup(IGameDataPropertyCollectionRuntime& runtime);

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
