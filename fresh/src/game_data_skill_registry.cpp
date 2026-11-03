#include "pal4/game_data_skill_registry.h"

#include <array>
#include <string_view>
#include <utility>

namespace pal4 {
namespace {

struct LiteralField {
    std::uint32_t offset;
    std::string_view name;
    GameDataSkillFieldKind kind;
    std::uint32_t vtable;
    std::uint32_t object_size;
    std::uint32_t source_address;
    std::uint32_t source_count;
};

constexpr std::array<LiteralField, 7> kLeadingFields{{
    {0, "ID", GameDataSkillFieldKind::numeric, 0x843368, 24, 0, 0},
    {128, "SKILL_DEPENDED", GameDataSkillFieldKind::numeric, 0x843368, 24, 0, 0},
    {4, "AI_CMD_TYPE", GameDataSkillFieldKind::numeric, 0x843368, 24, 0, 0},
    {112, "ATTACHED_SKILL", GameDataSkillFieldKind::numeric, 0x843368, 24, 0, 0},
    {116, "CONSUMED_RAGE", GameDataSkillFieldKind::numeric, 0x843368, 24, 0, 0},
    {120, "CONSUMED_MONEY", GameDataSkillFieldKind::numeric, 0x843368, 24, 0, 0},
    {124, "NEED_PROP_ID", GameDataSkillFieldKind::numeric, 0x843368, 24, 0, 0},
}};

constexpr std::array<LiteralField, 10> kTrailingFields{{
    {8, "NAME", GameDataSkillFieldKind::text, 0x844574, 24, 0, 0},
    {24, "DESCRIPTION", GameDataSkillFieldKind::text, 0x844574, 24, 0, 0},
    {40, "PROP", GameDataSkillFieldKind::text, 0x844574, 24, 0, 0},
    {132, "ACTTYPE", GameDataSkillFieldKind::action_type, 0x844494, 32, 0x8ef5a8, 3},
    {136, "TARGET", GameDataSkillFieldKind::numeric, 0x843368, 24, 0, 0},
    {140, "HIT_EXTRA", GameDataSkillFieldKind::hit_extra, 0x8444dc, 24, 0, 0},
    {144, "ANIMATION", GameDataSkillFieldKind::text, 0x844574, 24, 0, 0},
    {160, "EFFECT", GameDataSkillFieldKind::text, 0x844574, 24, 0, 0},
    {176, "TARGET_EF", GameDataSkillFieldKind::text, 0x844574, 24, 0, 0},
    {192, "TARGET_BIND", GameDataSkillFieldKind::text, 0x844574, 24, 0, 0},
}};

void AppendLiteral(const LiteralField& item, const std::string& suffix,
                   std::vector<GameDataSkillField>& fields,
                   IGameDataSkillRegistryRuntime& runtime) {
    runtime.AppendField(fields, {item.offset, std::string(item.name) + suffix,
                                 item.kind, item.vtable, item.object_size,
                                 item.source_address, item.source_count});
}

void AppendSource(std::uint32_t offset, std::uint32_t source,
                  std::string_view suffix,
                  std::vector<GameDataSkillField>& fields,
                  IGameDataSkillRegistryRuntime& runtime) {
    runtime.AppendField(fields,
                        {offset, runtime.SourceName(source) + std::string(suffix),
                         GameDataSkillFieldKind::numeric, 0x843368, 24,
                         source, 0});
}

}  // namespace

const std::vector<GameDataSkillField>& GameDataSkillRegistry::Initialize(
    IGameDataSkillRegistryRuntime& runtime) {
    if (!globals_initialized_) {
        globals_initialized_ = true;
        runtime.InitializeGlobalStorage();
        runtime.RegisterGlobalCleanup();
    }
    if (entries_initialized_) return fields_;
    entries_initialized_ = true;

    for (const auto& field : kLeadingFields)
        AppendLiteral(field, runtime.ErrorSuffix(), fields_, runtime);
    for (std::uint32_t i = 0; i < 4; ++i)
        AppendSource(56 + i * 4, 0x8ef9c8 + i * 16,
                     "_LV_LMT", fields_, runtime);
    for (std::uint32_t i = 0; i < 5; ++i)
        AppendSource(72 + i * 4, 0x8eeb90 + i * 16,
                     runtime.ErrorSuffix(), fields_, runtime);
    for (std::uint32_t i = 0; i < 5; ++i)
        AppendSource(92 + i * 4, 0x8eeb90 + i * 16,
                     "_NUMBER", fields_, runtime);
    for (const auto& field : kTrailingFields)
        AppendLiteral(field, runtime.ErrorSuffix(), fields_, runtime);
    runtime.Sort(fields_);
    return fields_;
}

void GameDataSkillRegistry::Cleanup(IGameDataSkillRegistryRuntime& runtime) {
    runtime.CleanupGlobalStorage();
    fields_.clear();
}

std::uint8_t GameDataSkillRegistry::Process(
    std::uint32_t object, IGameDataSkillRegistryRuntime& runtime) {
    return runtime.ProcessCollection(object, Initialize(runtime));
}

}  // namespace pal4
