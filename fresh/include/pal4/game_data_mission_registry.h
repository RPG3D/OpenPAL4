#pragma once

#include "pal4/game_data_field_descriptor.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pal4 {

class IGameDataMissionRegistryRuntime {
public:
    virtual ~IGameDataMissionRegistryRuntime() = default;
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

class GameDataMissionRegistry final {
public:
    // IDA 0x4daa60: one-time Mission field registration.
    [[nodiscard]] const std::vector<GameDataFieldDescriptor>& Initialize(
        IGameDataMissionRegistryRuntime& runtime);
    // IDA 0x4db0f0/0x4db100: CRT cleanup and event-processing wrapper.
    void Cleanup(IGameDataMissionRegistryRuntime& runtime);
    [[nodiscard]] std::uint8_t Process(
        std::uint32_t object, IGameDataMissionRegistryRuntime& runtime);

private:
    bool globals_initialized_ = false;
    bool entries_initialized_ = false;
    std::vector<GameDataFieldDescriptor> fields_;
};

}  // namespace pal4
