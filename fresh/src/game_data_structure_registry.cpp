#include "pal4/game_data_structure_registry.h"

#include <array>
#include <string_view>

namespace pal4 {
namespace {

struct PropertyGroup {
    std::uint32_t first_id;
    std::uint32_t last_id;
    std::uint32_t source;
    GameDataPropertyKind kind;
    std::string_view suffix;
};

constexpr std::array<PropertyGroup, 7> kGroups{{
    {196, 200, 0x8ee348, GameDataPropertyKind::text, {}},
    {168, 192, 0x8ee278, GameDataPropertyKind::text, "_IMMUNITY"},
    {144, 164, 0x8ee2e8, GameDataPropertyKind::text, "_IMMUNITY"},
    {204, 240, 0x8ee3b8, GameDataPropertyKind::numeric, {}},
    {244, 260, 0x8ee368, GameDataPropertyKind::text, {}},
    {264, 288, 0x8ef7a8, GameDataPropertyKind::text, "_RACE_ATK"},
    {292, 324, 0x8ee018, GameDataPropertyKind::numeric, {}},
}};

}  // namespace

const std::vector<GameDataProperty>& GameDataStructureRegistry::Initialize(
    IGameDataStructureRegistryRuntime& runtime) {
    if (!globals_initialized_) {
        globals_initialized_ = true;
        runtime.InitializeGlobalStorage();
        runtime.RegisterGlobalCleanup();
    }
    if (entries_initialized_) return entries_;

    entries_initialized_ = true;
    runtime.SeedCollection(entries_);
    for (const PropertyGroup& group : kGroups) {
        for (std::uint32_t id = group.first_id, address = group.source;
             id <= group.last_id; id += 4, address += 16) {
            std::string name = runtime.SourceName(address);
            name += group.suffix.empty() ? runtime.ErrorSuffix()
                                         : std::string(group.suffix);
            entries_.push_back({id, address, group.kind, std::move(name), 0});
        }
    }
    std::string position = "COMBAT_POSITION";
    position += runtime.ErrorSuffix();
    entries_.push_back({328, 0x8ef688, GameDataPropertyKind::combat_position,
                        std::move(position), 6});
    std::string ai_type = "AI_TYPE";
    ai_type += runtime.ErrorSuffix();
    entries_.push_back({332, 0, GameDataPropertyKind::numeric,
                        std::move(ai_type), 0});
    runtime.Sort(entries_);
    return entries_;
}

void GameDataStructureRegistry::Cleanup(
    IGameDataStructureRegistryRuntime& runtime) {
    runtime.CleanupGlobalStorage();
    entries_.clear();
}

}  // namespace pal4
