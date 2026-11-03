#pragma once

#include "pal4/game_data_field_descriptor.h"

#include <cstdint>
#include <vector>

namespace pal4 {

class IGameDataEstRegistryRuntime {
public:
    virtual ~IGameDataEstRegistryRuntime() = default;
    virtual void InitializeGlobalStorage(std::uint32_t address) = 0;
    virtual void RegisterGlobalCleanup(std::uint32_t callback) = 0;
    virtual void CleanupGlobalStorage(std::uint32_t address) = 0;
    virtual void AppendField(std::vector<GameDataFieldDescriptor>& fields,
                             GameDataFieldDescriptor field) = 0;
    virtual void Sort(std::vector<GameDataFieldDescriptor>& fields) = 0;
    [[nodiscard]] virtual std::uint8_t ProcessCollection(
        std::uint32_t collection,
        const std::vector<GameDataFieldDescriptor>& fields) = 0;
};

class GameDataEstRegistry final {
public:
    static constexpr std::uint32_t kCollectionAddress = 0x8ef0f8;
    static constexpr std::uint32_t kCleanupCallback = 0x4e98d0;

    // IDA 0x4e8500: initialize and sort the 32 numeric descriptors.
    [[nodiscard]] const std::vector<GameDataFieldDescriptor>& Initialize(
        IGameDataEstRegistryRuntime& runtime);
    // IDA 0x4e98d0: release the global descriptor collection.
    void Cleanup(IGameDataEstRegistryRuntime& runtime);
    // IDA 0x4e98e0: initialize then process an external collection.
    [[nodiscard]] std::uint8_t Process(
        std::uint32_t collection, IGameDataEstRegistryRuntime& runtime);

private:
    bool globals_initialized_ = false;
    bool entries_initialized_ = false;
    std::vector<GameDataFieldDescriptor> fields_;
};

}  // namespace pal4
