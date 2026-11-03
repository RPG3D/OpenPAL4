#pragma once

#include "pal4/game_data_property_collection_runtime.h"

#include <cstdint>
#include <vector>

namespace pal4 {

class IGameDataEquipmentAttributeRegistryRuntime
    : public IGameDataPropertyCollectionRuntime {
public:
    virtual void ImportBaseCollection(std::uint32_t destination) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessCollection(
        std::uint32_t object, std::uint32_t collection) = 0;
};

class GameDataEquipmentAttributeRegistry final {
public:
    static constexpr std::uint32_t kCollectionAddress = 0x8ee9b8;

    // IDA 0x4d2580: one-time storage and 25 EA property registrations.
    [[nodiscard]] std::uint32_t Initialize(
        std::uint8_t first_seed, std::uint8_t second_seed,
        IGameDataEquipmentAttributeRegistryRuntime& runtime);
    // IDA 0x4d2a30: CRT cleanup of the shared collection.
    void Cleanup(IGameDataEquipmentAttributeRegistryRuntime& runtime);
    // IDA 0x4d2a60: initialize then process the collection.
    [[nodiscard]] std::uint8_t Process(
        std::uint32_t object, std::uint8_t first_seed,
        std::uint8_t second_seed,
        IGameDataEquipmentAttributeRegistryRuntime& runtime);
    // IDA 0x4d2a80: unsigned equipment type is at most six.
    [[nodiscard]] static bool TypeInRange(
        std::uint32_t object,
        IGameDataEquipmentAttributeRegistryRuntime& runtime);
    // IDA 0x4d2aa0: map type 3..6 to 1..4, otherwise zero.
    [[nodiscard]] static std::uint32_t TypeIndex(
        std::uint32_t object,
        IGameDataEquipmentAttributeRegistryRuntime& runtime);
    [[nodiscard]] static constexpr std::uint32_t TypeIndexForType(std::uint32_t type){
        return type>=3u && type<=6u?type-2u:0u;
    }

    [[nodiscard]] const std::vector<GameDataProperty>& Records() const {
        return records_;
    }

private:
    bool storage_initialized_ = false;
    bool records_initialized_ = false;
    std::vector<GameDataProperty> records_;
};

}  // namespace pal4
