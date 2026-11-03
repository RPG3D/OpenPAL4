#pragma once

#include "pal4/game_data_field_descriptor.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pal4 {

class IGameDataMonsterTableRegistryRuntime {
public:
    virtual ~IGameDataMonsterTableRegistryRuntime() = default;
    virtual void InitializeGlobalStorage() = 0;
    virtual void RegisterGlobalCleanup() = 0;
    virtual void CleanupGlobalStorage() = 0;
    [[nodiscard]] virtual std::string ErrorSuffix() = 0;
    virtual void AppendField(std::vector<GameDataFieldDescriptor>& fields,
                             GameDataFieldDescriptor field) = 0;
    virtual void ImportStructureFields(
        std::vector<GameDataFieldDescriptor>& fields,
        std::uint32_t offset) = 0;
    virtual void ImportMonsterFields(
        std::vector<GameDataFieldDescriptor>& fields,
        std::uint32_t offset) = 0;
    virtual void Sort(std::vector<GameDataFieldDescriptor>& fields) = 0;
    [[nodiscard]] virtual std::uint8_t ProcessCollection(
        std::uint32_t object,
        const std::vector<GameDataFieldDescriptor>& fields) = 0;
};

class GameDataMonsterTableRegistry final {
public:
    // IDA 0x4de7e0: register MONSTER_ID and import two field collections.
    [[nodiscard]] const std::vector<GameDataFieldDescriptor>& Initialize(
        IGameDataMonsterTableRegistryRuntime& runtime);
    // IDA 0x4dea30: release the global field collection.
    void Cleanup(IGameDataMonsterTableRegistryRuntime& runtime);
    // IDA 0x4dea40: initialize and process the collection.
    [[nodiscard]] std::uint8_t Process(
        std::uint32_t object,
        IGameDataMonsterTableRegistryRuntime& runtime);

private:
    bool globals_initialized_ = false;
    bool entries_initialized_ = false;
    std::vector<GameDataFieldDescriptor> fields_;
};

}  // namespace pal4
