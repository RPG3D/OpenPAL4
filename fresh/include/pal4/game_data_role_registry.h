#pragma once

#include "pal4/game_data_field_descriptor.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pal4 {

class IGameDataRoleRegistryRuntime {
public:
    virtual ~IGameDataRoleRegistryRuntime() = default;
    virtual void InitializeGlobalStorage(std::uint32_t address) = 0;
    virtual void RegisterGlobalCleanup(std::uint32_t callback) = 0;
    virtual void CleanupGlobalStorage(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string SourceName(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ErrorSuffix() = 0;
    virtual void AppendField(std::vector<GameDataFieldDescriptor>& fields,
                             GameDataFieldDescriptor field) = 0;
    virtual void Sort(std::vector<GameDataFieldDescriptor>& fields) = 0;
};

class GameDataRoleRegistry final {
public:
    static constexpr std::uint32_t kCollectionAddress = 0x8ef194;
    // IDA 0x4e7860: initialize and sort 16 Role field descriptors.
    [[nodiscard]] const std::vector<GameDataFieldDescriptor>& Initialize(
        IGameDataRoleRegistryRuntime& runtime);
    // IDA 0x4e7db0: release the global Role field collection.
    void Cleanup(IGameDataRoleRegistryRuntime& runtime);

private:
    bool globals_initialized_ = false;
    bool entries_initialized_ = false;
    std::vector<GameDataFieldDescriptor> fields_;
};

class IGameDataRoleMasterRuntime : public IGameDataRoleRegistryRuntime {
public:
    virtual void MergeRoleFields(
        std::vector<GameDataFieldDescriptor>& destination,
        const std::vector<GameDataFieldDescriptor>& source,
        std::uint32_t offset) = 0;
    virtual void MergeOtherFields(
        std::vector<GameDataFieldDescriptor>& destination,
        std::uint32_t offset) = 0;
    [[nodiscard]] virtual std::uint8_t ProcessCollection(
        std::uint32_t collection,
        const std::vector<GameDataFieldDescriptor>& fields) = 0;
};

class GameDataRoleMasterRegistry final {
public:
    static constexpr std::uint32_t kCollectionAddress = 0x8ef160;
    // IDA 0x4e7e40: ROLE_ID plus two child collections.
    [[nodiscard]] const std::vector<GameDataFieldDescriptor>& Initialize(
        IGameDataRoleMasterRuntime& runtime);
    // IDA 0x4e8090: release the master Role field collection.
    void Cleanup(IGameDataRoleMasterRuntime& runtime);
    // IDA 0x4e80a0: initialize then process an external collection.
    [[nodiscard]] std::uint8_t Process(
        std::uint32_t collection, IGameDataRoleMasterRuntime& runtime);

private:
    bool globals_initialized_ = false;
    bool entries_initialized_ = false;
    GameDataRoleRegistry role_registry_;
    std::vector<GameDataFieldDescriptor> fields_;
};

class IGameDataRoleAttributeRuntime :
    public IGameDataRoleRegistryRuntime {
public:
    virtual void MergeStatsFields(
        std::vector<GameDataFieldDescriptor>& destination,
        std::uint32_t offset) = 0;
    [[nodiscard]] virtual std::uint8_t ProcessCollection(
        std::uint32_t collection,
        const std::vector<GameDataFieldDescriptor>& fields) = 0;
};

class GameDataRoleAttributeRegistry final {
public:
    static constexpr std::uint32_t kCollectionAddress = 0x8ef12c;
    // IDA 0x4e80f0: four literals plus a child stats collection.
    [[nodiscard]] const std::vector<GameDataFieldDescriptor>& Initialize(
        IGameDataRoleAttributeRuntime& runtime);
    // IDA 0x4e84a0: release this field collection.
    void Cleanup(IGameDataRoleAttributeRuntime& runtime);
    // IDA 0x4e84c0: initialize then process an external collection.
    [[nodiscard]] std::uint8_t Process(
        std::uint32_t collection, IGameDataRoleAttributeRuntime& runtime);

private:
    bool globals_initialized_ = false;
    bool entries_initialized_ = false;
    std::vector<GameDataFieldDescriptor> fields_;
};

}  // namespace pal4
