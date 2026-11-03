#include "pal4/game_data_race_registry.h"

#include <array>
#include <string_view>
#include <utility>

namespace pal4 {
namespace {

struct PropertyGroup {
    std::uint32_t first_id;
    std::uint32_t count;
    std::uint32_t source;
    std::uint32_t source_stride;
    GameDataPropertyKind kind;
    std::string_view suffix;
};

constexpr std::array<PropertyGroup, 6> kGroups{{
    {4, 8, 0x8ee4f8, 16, GameDataPropertyKind::numeric, {}},
    {36, 4, 0x8ee4b8, 16, GameDataPropertyKind::text, {}},
    {52, 6, 0x8ee458, 16, GameDataPropertyKind::numeric, "_ADDITIONAL"},
    {76, 6, 0x8ee458, 16, GameDataPropertyKind::text, "_REACT"},
    {100, 6, 0x8ee458, 16, GameDataPropertyKind::text, "_EXTRACT"},
    {124, 5, 0x8efa08, 16, GameDataPropertyKind::numeric, {}},
}};

}  // namespace

std::uint32_t GameDataRaceRegistry::Initialize(
    std::uint8_t first_seed, std::uint8_t second_seed,
    IGameDataPropertyCollectionRuntime& runtime) {
    if (!storage_initialized_) {
        storage_initialized_ = true;
        runtime.InitializeStorage(first_seed, second_seed);
        runtime.RegisterCleanup();
    }
    runtime.ResetTemporaryName(second_seed);
    if (!records_initialized_) {
        records_initialized_ = true;
        const auto append = [&](GameDataProperty record) {
            runtime.AppendRecord(record);
            records_.push_back(std::move(record));
        };
        append({0, 0x8ef7a8, GameDataPropertyKind::race,
                "RACE" + runtime.ErrorSuffix(), 7});
        for (const auto& group : kGroups) {
            for (std::uint32_t index = 0; index < group.count; ++index) {
                const auto address = group.source + index * group.source_stride;
                std::string name = runtime.SourceName(address);
                name += group.suffix.empty() ? runtime.ErrorSuffix()
                                             : std::string(group.suffix);
                append({group.first_id + index * 4, address, group.kind,
                        std::move(name), 0});
            }
        }
        runtime.Sort(records_);
    }
    runtime.ReleaseTemporaryName();
    return kCollectionAddress;
}

void GameDataRaceRegistry::Cleanup(IGameDataPropertyCollectionRuntime& runtime) {
    runtime.CleanupStorage();
    records_.clear();
}

}  // namespace pal4
