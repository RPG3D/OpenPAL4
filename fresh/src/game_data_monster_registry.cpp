#include "pal4/game_data_monster_registry.h"

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
    std::uint32_t object_size;
    std::uint32_t source_address;
    std::uint32_t source_count;
};

constexpr std::array<LiteralField, 4> kLeadingFields{{
    {248, "IS_BOSS", GameDataFieldKind::numeric, 0x843368, 24, 0, 0},
    {68, "COUNT", GameDataFieldKind::numeric, 0x843368, 24, 0, 0},
    {252, "PHYSICAL_ATK_TARGETT", GameDataFieldKind::action_type,
     0x844494, 32, 0x8ef438, 2},
    {256, "PHYSICAL_ATK_RANGE", GameDataFieldKind::action_type,
     0x844494, 32, 0x8ef418, 2},
}};

void AppendSource(std::uint32_t offset, std::uint32_t source,
                  std::string_view suffix,
                  std::vector<GameDataFieldDescriptor>& fields,
                  IGameDataMonsterRegistryRuntime& runtime,
                  GameDataFieldKind kind = GameDataFieldKind::numeric,
                  std::uint32_t vtable = 0x843368) {
    runtime.AppendField(fields,
                        {offset, runtime.SourceName(source) +
                                     std::string(suffix),
                         kind, vtable, 24, source, 0});
}

}  // namespace

const std::vector<GameDataFieldDescriptor>& GameDataMonsterRegistry::Initialize(
    IGameDataMonsterRegistryRuntime& runtime) {
    if (!globals_initialized_) {
        globals_initialized_ = true;
        runtime.InitializeGlobalStorage();
        runtime.RegisterGlobalCleanup();
    }
    if (entries_initialized_) return fields_;
    entries_initialized_ = true;

    for (const auto& item : kLeadingFields)
        runtime.AppendField(fields_,
                            {item.offset,
                             std::string(item.name) + runtime.ErrorSuffix(),
                             item.kind, item.vtable, item.object_size,
                             item.source_address, item.source_count});
    for (std::uint32_t i = 0; i < 5; ++i)
        AppendSource(i * 4, 0x8eed50 + i * 16,
                     runtime.ErrorSuffix(), fields_, runtime);
    for (std::uint32_t i = 0; i < 6; ++i)
        AppendSource(72 + i * 16, 0x8eecf0 + i * 16,
                     runtime.ErrorSuffix(), fields_, runtime,
                     GameDataFieldKind::text, 0x844574);
    for (std::uint32_t i = 0; i < 5; ++i)
        AppendSource(168 + i * 16, 0x8efa58 + i * 16,
                     runtime.ErrorSuffix(), fields_, runtime,
                     GameDataFieldKind::text, 0x844574);
    for (std::uint32_t i = 0; i < 3; ++i)
        AppendSource(20 + i * 4, 0x8eecc0 + i * 16,
                     runtime.ErrorSuffix(), fields_, runtime);
    for (std::uint32_t i = 0; i < 4; ++i)
        AppendSource(32 + i * 4, 0x8eec80 + i * 16,
                     "_ID", fields_, runtime);
    for (std::uint32_t i = 0; i < 4; ++i)
        AppendSource(48 + i * 4, 0x8eec80 + i * 16,
                     "_RATE", fields_, runtime,
                     GameDataFieldKind::hit_extra, 0x8444dc);
    runtime.Sort(fields_);
    return fields_;
}

void GameDataMonsterRegistry::Cleanup(
    IGameDataMonsterRegistryRuntime& runtime) {
    runtime.CleanupGlobalStorage();
    fields_.clear();
}

}  // namespace pal4
