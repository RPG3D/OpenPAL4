#pragma once

#include "pal4/game_data_field_descriptor.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pal4 {

class IGameDataMagicRegistryRuntime {
public:
    virtual ~IGameDataMagicRegistryRuntime() = default;
    virtual void InitializeGlobalStorage() = 0;
    virtual void RegisterGlobalCleanup() = 0;
    virtual void CleanupGlobalStorage() = 0;
    [[nodiscard]] virtual std::string ErrorSuffix() = 0;
    virtual void AppendField(std::vector<GameDataFieldDescriptor>& fields,
                             GameDataFieldDescriptor field) = 0;
    virtual void Sort(std::vector<GameDataFieldDescriptor>& fields) = 0;
    [[nodiscard]] virtual std::uint8_t ProcessCollection(
        std::uint32_t object,
        const std::vector<GameDataFieldDescriptor>& fields) = 0;
};

class GameDataMagicRegistry final {
public:
    // IDA 0x4d9c80: one-time magic field registration.
    [[nodiscard]] const std::vector<GameDataFieldDescriptor>& Initialize(
        IGameDataMagicRegistryRuntime& runtime);
    // IDA 0x4da960/0x4da970: CRT cleanup and event-processing wrapper.
    void Cleanup(IGameDataMagicRegistryRuntime& runtime);
    [[nodiscard]] std::uint8_t Process(
        std::uint32_t object, IGameDataMagicRegistryRuntime& runtime);

private:
    bool globals_initialized_ = false;
    bool entries_initialized_ = false;
    std::vector<GameDataFieldDescriptor> fields_;
};

}  // namespace pal4
