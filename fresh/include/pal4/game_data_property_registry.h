#pragma once

#include "pal4/game_data_field_descriptor.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pal4 {

class IGameDataPropertyRegistryRuntime {
public:
    virtual ~IGameDataPropertyRegistryRuntime() = default;
    virtual void InitializeGlobalStorage() = 0;
    virtual void RegisterGlobalCleanup() = 0;
    virtual void CleanupGlobalStorage() = 0;
    [[nodiscard]] virtual std::string SourceName(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ErrorSuffix() = 0;
    virtual void AppendField(std::vector<GameDataFieldDescriptor>& fields,
                             GameDataFieldDescriptor field) = 0;
    virtual void Sort(std::vector<GameDataFieldDescriptor>& fields) = 0;
    [[nodiscard]] virtual std::uint8_t ProcessCollection(
        std::uint32_t collection,
        const std::vector<GameDataFieldDescriptor>& fields) = 0;
};

class GameDataPropertyRegistry final {
public:
    // IDA 0x4e4df0: initialize and sort 21 property field descriptors.
    [[nodiscard]] const std::vector<GameDataFieldDescriptor>& Initialize(
        IGameDataPropertyRegistryRuntime& runtime);
    // IDA 0x4e54a0: release the global field collection.
    void Cleanup(IGameDataPropertyRegistryRuntime& runtime);
    // IDA 0x4e54b0: initialize the registry before processing a collection.
    [[nodiscard]] std::uint8_t Process(
        std::uint32_t collection,
        IGameDataPropertyRegistryRuntime& runtime);

private:
    bool globals_initialized_ = false;
    bool entries_initialized_ = false;
    std::vector<GameDataFieldDescriptor> fields_;
};

}  // namespace pal4
