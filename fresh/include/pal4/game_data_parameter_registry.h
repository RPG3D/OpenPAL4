#pragma once

#include "pal4/game_data_field_descriptor.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pal4 {

class IGameDataParameterRegistryRuntime {
public:
    virtual ~IGameDataParameterRegistryRuntime() = default;
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

class GameDataParameterRegistry final {
public:
    // IDA 0x4e1c40: Prescription328-byte field list. Text destinations advance
    // by16, other destinations by4; equipment source names advance by16.
    [[nodiscard]] const std::vector<GameDataFieldDescriptor>& Initialize(
        IGameDataParameterRegistryRuntime& runtime);
    // IDA 0x4e2800: release the global field collection.
    void Cleanup(IGameDataParameterRegistryRuntime& runtime);
    // IDA 0x4e2810: initialize the field list and process a collection.
    [[nodiscard]] std::uint8_t Process(
        std::uint32_t collection,
        IGameDataParameterRegistryRuntime& runtime);

private:
    bool globals_initialized_ = false;
    bool entries_initialized_ = false;
    std::vector<GameDataFieldDescriptor> fields_;
};

}  // namespace pal4
