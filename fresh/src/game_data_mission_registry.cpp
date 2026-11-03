#include "pal4/game_data_mission_registry.h"

#include <array>
#include <string_view>
#include <utility>

namespace pal4 {
namespace {

struct LiteralField {
    std::uint32_t offset;
    std::string_view name;
    GameDataFieldKind kind;
    std::uint32_t vtable;
};

constexpr std::array<LiteralField, 8> kFields{{
    {0, "DEPENDED_ID", GameDataFieldKind::numeric, 0x843368},
    {8, "TRUNK", GameDataFieldKind::numeric, 0x843368},
    {4, "QUEST_ID", GameDataFieldKind::numeric, 0x843368},
    {16, "STORY_SHOW", GameDataFieldKind::numeric, 0x843368},
    {12, "STORY_PER", GameDataFieldKind::hit_extra, 0x8444dc},
    {20, "NAME", GameDataFieldKind::text, 0x844574},
    {52, "DESCRIPTION", GameDataFieldKind::text, 0x844574},
    {36, "PICTURE", GameDataFieldKind::text, 0x844574},
}};

}  // namespace

const std::vector<GameDataFieldDescriptor>& GameDataMissionRegistry::Initialize(
    IGameDataMissionRegistryRuntime& runtime) {
    if (!globals_initialized_) {
        globals_initialized_ = true;
        runtime.InitializeGlobalStorage();
        runtime.RegisterGlobalCleanup();
    }
    if (entries_initialized_) return fields_;
    entries_initialized_ = true;
    for (const auto& field : kFields) {
        runtime.AppendField(fields_,
                            {field.offset,
                             std::string(field.name) + runtime.ErrorSuffix(),
                             field.kind, field.vtable, 24, 0, 0});
    }
    runtime.Sort(fields_);
    return fields_;
}

void GameDataMissionRegistry::Cleanup(IGameDataMissionRegistryRuntime& runtime) {
    runtime.CleanupGlobalStorage();
    fields_.clear();
}

std::uint8_t GameDataMissionRegistry::Process(
    std::uint32_t object, IGameDataMissionRegistryRuntime& runtime) {
    return runtime.ProcessCollection(object, Initialize(runtime));
}

}  // namespace pal4
