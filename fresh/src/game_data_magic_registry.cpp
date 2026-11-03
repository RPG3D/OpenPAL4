#include "pal4/game_data_magic_registry.h"

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

constexpr std::array<LiteralField, 17> kFields{{
    {0, "ID", GameDataFieldKind::numeric, 0x843368},
    {4, "FELEMENTS", GameDataFieldKind::numeric, 0x843368},
    {8, "AI_CMD_TYPE", GameDataFieldKind::numeric, 0x843368},
    {12, "NAME", GameDataFieldKind::text, 0x844574},
    {28, "DESCRIPTION", GameDataFieldKind::text, 0x844574},
    {44, "PROP", GameDataFieldKind::text, 0x844574},
    {60, "NEED_POINT", GameDataFieldKind::numeric, 0x843368},
    {64, "NEED_TOTAL_USED_POINT", GameDataFieldKind::numeric, 0x843368},
    {68, "PARENT_MAGIC", GameDataFieldKind::numeric, 0x843368},
    {72, "ATTACHED_SKILL", GameDataFieldKind::numeric, 0x843368},
    {76, "CONSUMED_MP", GameDataFieldKind::numeric, 0x843368},
    {80, "TARGET", GameDataFieldKind::numeric, 0x843368},
    {84, "HIT_EXTRA", GameDataFieldKind::hit_extra, 0x8444dc},
    {88, "ANIMATION", GameDataFieldKind::text, 0x844574},
    {104, "EFFECT", GameDataFieldKind::text, 0x844574},
    {120, "TARGET_EF", GameDataFieldKind::text, 0x844574},
    {136, "TARGET_BIND", GameDataFieldKind::text, 0x844574},
}};

}  // namespace

const std::vector<GameDataFieldDescriptor>& GameDataMagicRegistry::Initialize(
    IGameDataMagicRegistryRuntime& runtime) {
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

void GameDataMagicRegistry::Cleanup(IGameDataMagicRegistryRuntime& runtime) {
    runtime.CleanupGlobalStorage();
    fields_.clear();
}

std::uint8_t GameDataMagicRegistry::Process(
    std::uint32_t object, IGameDataMagicRegistryRuntime& runtime) {
    return runtime.ProcessCollection(object, Initialize(runtime));
}

}  // namespace pal4
