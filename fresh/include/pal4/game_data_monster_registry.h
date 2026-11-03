#pragma once

#include "pal4/game_data_field_descriptor.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pal4 {

class IGameDataMonsterRegistryRuntime {
public:
    virtual ~IGameDataMonsterRegistryRuntime() = default;
    virtual void InitializeGlobalStorage() = 0;
    virtual void RegisterGlobalCleanup() = 0;
    virtual void CleanupGlobalStorage() = 0;
    [[nodiscard]] virtual std::string SourceName(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ErrorSuffix() = 0;
    virtual void AppendField(std::vector<GameDataFieldDescriptor>& fields,
                             GameDataFieldDescriptor field) = 0;
    virtual void Sort(std::vector<GameDataFieldDescriptor>& fields) = 0;
};

class GameDataMonsterRegistry final {
public:
    // IDA 0x4ddcb0: register the monster record's 31 field descriptors.
    [[nodiscard]] const std::vector<GameDataFieldDescriptor>& Initialize(
        IGameDataMonsterRegistryRuntime& runtime);
    // IDA 0x4de4e0: CRT callback for the global field collection.
    void Cleanup(IGameDataMonsterRegistryRuntime& runtime);

private:
    bool globals_initialized_ = false;
    bool entries_initialized_ = false;
    std::vector<GameDataFieldDescriptor> fields_;
};

}  // namespace pal4
