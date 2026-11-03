#include "pal4/game_data_role_registry.h"

#include <string_view>
#include <utility>

namespace pal4 {
namespace {

void AppendSource(std::uint32_t offset, std::uint32_t source,
                  std::string_view suffix, GameDataFieldKind kind,
                  std::uint32_t vtable,
                  std::vector<GameDataFieldDescriptor>& fields,
                  IGameDataRoleRegistryRuntime& runtime) {
    runtime.AppendField(fields,
                        {offset, runtime.SourceName(source) +
                                     std::string(suffix),
                         kind, vtable, 24, source, 0});
}

}  // namespace

const std::vector<GameDataFieldDescriptor>& GameDataRoleRegistry::Initialize(
    IGameDataRoleRegistryRuntime& runtime) {
    if (!globals_initialized_) {
        globals_initialized_ = true;
        runtime.InitializeGlobalStorage(kCollectionAddress);
        runtime.RegisterGlobalCleanup(0x4e7db0);
    }
    if (entries_initialized_) return fields_;
    entries_initialized_ = true;

    const auto empty = runtime.ErrorSuffix();
    runtime.AppendField(fields_,
                        {88, "PHYSICAL_ATK_TARGET" + empty,
                         GameDataFieldKind::action_type, 0x844494, 32,
                         0x8ef438, 2});
    runtime.AppendField(fields_,
                        {92, "PHYSICAL_ATK_RANGE" + empty,
                         GameDataFieldKind::action_type, 0x844494, 32,
                         0x8ef418, 2});
    runtime.AppendField(fields_,
                        {80, "PLAYER_TYPE" + empty,
                         GameDataFieldKind::action_type, 0x844494, 32,
                         0x8ef9c8, 4});

    for (std::uint32_t i = 0; i < 4; ++i)
        AppendSource(i * 16, 0x8ef1c8 + i * 16, empty,
                     GameDataFieldKind::text, 0x844574,
                     fields_, runtime);
    for (std::uint32_t i = 0; i < 4; ++i)
        AppendSource(64 + i * 4, 0x8ef9c8 + i * 16, "_FAVOR",
                     GameDataFieldKind::numeric, 0x843368,
                     fields_, runtime);
    for (std::uint32_t i = 0; i < 5; ++i)
        AppendSource(96 + i * 16, 0x8efa58 + i * 16, empty,
                     GameDataFieldKind::text, 0x844574,
                     fields_, runtime);
    runtime.Sort(fields_);
    return fields_;
}

void GameDataRoleRegistry::Cleanup(
    IGameDataRoleRegistryRuntime& runtime) {
    runtime.CleanupGlobalStorage(kCollectionAddress);
    fields_.clear();
}

const std::vector<GameDataFieldDescriptor>&
GameDataRoleMasterRegistry::Initialize(IGameDataRoleMasterRuntime& runtime) {
    if (!globals_initialized_) {
        globals_initialized_ = true;
        runtime.InitializeGlobalStorage(kCollectionAddress);
        runtime.RegisterGlobalCleanup(0x4e8090);
    }
    if (entries_initialized_) return fields_;
    entries_initialized_ = true;

    runtime.AppendField(fields_,
                        {228, "ROLE_ID" + runtime.ErrorSuffix(),
                         GameDataFieldKind::numeric, 0x843368, 24, 0, 0});
    const auto& role_fields = role_registry_.Initialize(runtime);
    runtime.MergeRoleFields(fields_, role_fields, 144);
    runtime.MergeOtherFields(fields_, 0);
    runtime.Sort(fields_);
    return fields_;
}

void GameDataRoleMasterRegistry::Cleanup(
    IGameDataRoleMasterRuntime& runtime) {
    runtime.CleanupGlobalStorage(kCollectionAddress);
    fields_.clear();
}

std::uint8_t GameDataRoleMasterRegistry::Process(
    std::uint32_t collection, IGameDataRoleMasterRuntime& runtime) {
    return runtime.ProcessCollection(collection, Initialize(runtime));
}

const std::vector<GameDataFieldDescriptor>&
GameDataRoleAttributeRegistry::Initialize(
    IGameDataRoleAttributeRuntime& runtime) {
    if (!globals_initialized_) {
        globals_initialized_ = true;
        runtime.InitializeGlobalStorage(kCollectionAddress);
        runtime.RegisterGlobalCleanup(0x4e84a0);
    }
    if (entries_initialized_) return fields_;
    entries_initialized_ = true;

    const auto empty = runtime.ErrorSuffix();
    for (const auto& [offset, name] : {
             std::pair<std::uint32_t, std::string_view>{144, "ID"},
             {148, "ROLE_ID"}, {152, "CA_LEVEL"},
             {156, "CA_EXPERIENCE"}})
        runtime.AppendField(fields_,
                            {offset, std::string(name) + empty,
                             GameDataFieldKind::numeric, 0x843368,
                             24, 0, 0});
    runtime.MergeStatsFields(fields_, 0);
    runtime.Sort(fields_);
    return fields_;
}

void GameDataRoleAttributeRegistry::Cleanup(
    IGameDataRoleAttributeRuntime& runtime) {
    runtime.CleanupGlobalStorage(kCollectionAddress);
    fields_.clear();
}

std::uint8_t GameDataRoleAttributeRegistry::Process(
    std::uint32_t collection, IGameDataRoleAttributeRuntime& runtime) {
    return runtime.ProcessCollection(collection, Initialize(runtime));
}

}  // namespace pal4
