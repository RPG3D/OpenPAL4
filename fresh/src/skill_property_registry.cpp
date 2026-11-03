#include "pal4/skill_property_registry.h"

#include <array>

namespace pal4 {
namespace {

struct NamedProperty {
    SkillPropertyKind kind;
    std::string_view name;
    std::uint32_t offset;
};

constexpr std::array<NamedProperty, 12> kNamedProperties = {{
    {SkillPropertyKind::Basic, "ID", 0},
    {SkillPropertyKind::Basic, "TYPE", 4},
    {SkillPropertyKind::Basic, "TRIGER", 8},
    {SkillPropertyKind::Basic, "EXIST_TIME", 12},
    {SkillPropertyKind::Basic, "FUNCTION", 16},
    {SkillPropertyKind::Basic, "ATTACHED_SKILL", 56},
    {SkillPropertyKind::Basic, "MUTEX_ID", 60},
    {SkillPropertyKind::Basic, "STATUS_ID", 64},
    {SkillPropertyKind::Effect, "EF", 68},
    {SkillPropertyKind::Effect, "BIND", 84},
    {SkillPropertyKind::Effect, "EF1", 100},
    {SkillPropertyKind::Effect, "BIND1", 116},
}};

}  // namespace

std::uint32_t SkillPropertyRegistry::Initialize(
    ISkillPropertyRegistryRuntime& runtime) {
    if (!collection_initialized_) {
        collection_initialized_ = true;
        runtime.InitializeCollection(kCollectionAddress);
        static_cast<void>(runtime.RegisterCleanup(kCollectionAddress));
    }
    runtime.BeginTemporaryText();
    try {
        if (!properties_initialized_) {
            properties_initialized_ = true;
            for (const auto& property : kNamedProperties)
                runtime.AddNamedProperty(property.kind, property.name,
                                         property.offset);
            for (std::uint32_t index = 0; index < 9; ++index)
                runtime.AddParameterProperty(kParameterTextAddress + 16 * index,
                                             20 + 4 * index);
            runtime.Sort(kCollectionAddress);
        }
    } catch (...) {
        runtime.ReleaseTemporaryText();
        throw;
    }
    runtime.ReleaseTemporaryText();
    return kCollectionAddress;
}

void SkillPropertyRegistry::Cleanup(
    ISkillPropertyRegistryRuntime& runtime) const {
    runtime.CleanupCollection(kCollectionAddress);
}

std::uint8_t SkillPropertyRegistry::ProcessEvent(
    std::uint32_t input, ISkillPropertyRegistryRuntime& runtime) {
    const auto collection = Initialize(runtime);
    return runtime.ProcessEvent(input, collection);
}

}  // namespace pal4
