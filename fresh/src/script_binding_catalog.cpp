#include "pal4/script_binding_catalog.h"

#include <algorithm>
#include <array>
#include <utility>

namespace pal4 {
namespace {

struct Group {
    std::uint32_t source;
    std::uint32_t count;
    std::uint32_t stride;
    ScriptBindingNameKind name_kind;
    const char* suffix;
    std::uint32_t vtable;
    std::uint32_t object_constructor;
    std::uint32_t getter_constructor;
    std::uint32_t getter_callback;
    std::uint32_t setter_constructor;
    std::uint32_t setter_callback;
};

constexpr std::array<Group, 11> kGroups{{
    {0x8efa08, 5, 16, ScriptBindingNameKind::EnumBuffer, nullptr,
     0x848200, 0x4cbe80, 0x587fe0, 0x588c60, 0x588010, 0x4af760},
    {0x8ee3b8, 10, 16, ScriptBindingNameKind::EnumBuffer, nullptr,
     0x848200, 0x4cbe80, 0x588030, 0x588e30, 0x588050, 0x588d00},
    {0x8ee4f8, 8, 16, ScriptBindingNameKind::EnumBuffer, nullptr,
     0x848200, 0x4cbe80, 0x588070, 0x4543a0, 0x588090, 0x588cb0},
    {0x8ee4b8, 4, 16, ScriptBindingNameKind::EnumBuffer, nullptr,
     0x8481d0, 0x4c89f0, 0x5880b0, 0x588da0, 0x5880d0, 0x588c50},
    {0x8ee368, 5, 16, ScriptBindingNameKind::EnumBuffer, nullptr,
     0x8481d0, 0x4c89f0, 0x5880f0, 0x588d60, 0x588110, 0x588e80},
    {0x8ee458, 6, 16, ScriptBindingNameKind::EnumLabel, "_ADDITIONAL",
     0x848200, 0x4cbe80, 0x588130, 0x588e70, 0x588150, 0x588c40},
    {0x8ee458, 6, 16, ScriptBindingNameKind::EnumLabel, "_REACT",
     0x8481d0, 0x4c89f0, 0x588190, 0x588cf0, 0x5881b0, 0x588d50},
    {0x8ee458, 6, 16, ScriptBindingNameKind::EnumLabel, "_EXTRACT",
     0x8481d0, 0x4c89f0, 0x588190, 0x588de0, 0x5881b0, 0x588e20},
    {0x8ee2e8, 6, 16, ScriptBindingNameKind::EnumLabel, "_IMMUNITY",
     0x8481d0, 0x4c89f0, 0x5881d0, 0x588e60, 0x5881f0, 0x588ce0},
    {0x8ee278, 7, 16, ScriptBindingNameKind::EnumLabel, "_IMMUNITY",
     0x8481d0, 0x4c89f0, 0x588210, 0x588e10, 0x588230, 0x588ca0},
    {0x8ee348, 2, 16, ScriptBindingNameKind::EnumLabel, nullptr,
     0x8481d0, 0x4c89f0, 0x588250, 0x588dd0, 0x588270, 0x588c30},
}};

void Append(std::vector<ScriptBindingSpec>& entries,
            IScriptBindingCatalogRuntime& runtime,
            const ScriptBindingName& name, ScriptBindingSpec spec) {
    spec.name = runtime.ResolveName(name);
    entries.push_back(std::move(spec));
}

}  // namespace

const std::vector<ScriptBindingSpec>& ScriptBindingCatalog::Initialize(
    IScriptBindingCatalogRuntime& runtime) {
    if (!storage_initialized_) {
        storage_initialized_ = true;
        runtime.InitializeStorage(kCollectionAddress);
        runtime.InitializeStorage(0x8faa2cu);
        runtime.InitializeStorage(0x8faa3cu);
        runtime.RegisterCleanup(0x5852b0u);
    }
    if (populated_) return entries_;
    populated_ = true;

    Append(entries_, runtime,
           {ScriptBindingNameKind::Literal, 0x8b26e8, 0, 0, 0, "AI_TYPE"},
           {"", 0x24, 0x848200, 0x4cbe80, 0, 0,
            0x587f50, 0x4bb620, 0x587f70, 0x4bb630});
    Append(entries_, runtime,
           {ScriptBindingNameKind::Literal, 0x8b8a94, 0, 0, 0,
            "CombatPosition"},
           {"", 0x2c, 0x8481e8, 0x4cf2f0, 0x8ef688, 6,
            0x587f90, 0x4bb640, 0x587f90, 0x588e90});

    for (const auto& group : kGroups) {
        for (std::uint32_t index = 0; index < group.count; ++index) {
            const ScriptBindingName name{group.name_kind, group.source,
                                         index, group.stride, group.count,
                                         nullptr, group.suffix};
            Append(entries_, runtime, name,
                   {"", 0x24, group.vtable, group.object_constructor,
                    0, 0, group.getter_constructor,
                    group.getter_callback, group.setter_constructor,
                    group.setter_callback, index});
        }
    }

    const std::array<ScriptBindingSpec, 3> kStandalone{{
        {"", 0x2c, 0x8481b8, 0x4cf2f0, 0x8ee2e8, 6,
         0x587f90, 0x588d40, 0x587f90, 0x588d90},
        {"", 0x2c, 0x8481a0, 0x4cf2f0, 0x8ee278, 7,
         0x587f90, 0x588dc0, 0x587f90, 0x588e00},
        {"", 0x2c, 0x848188, 0x4cf2f0, 0x8ee208, 7,
         0x587f90, 0x588e50, 0x587f90, 0x588c20},
    }};
    constexpr std::array<std::uint32_t, 3> kStandaloneNames{{
        0x8ee018, 0x8ee028, 0x8ee038}};
    for (std::size_t index = 0; index < kStandalone.size(); ++index) {
        Append(entries_, runtime,
               {ScriptBindingNameKind::EnumBuffer, kStandaloneNames[index]},
               kStandalone[index]);
    }

    const std::array<ScriptBindingSpec, 6> kRoleFields{{
        {"", 0x2c, 0x848218, 0x585960, 0x8ee1e8, 2,
         0x587f90, 0x588c90, 0x587470, 0x588cd0},
        {"", 0x2c, 0x848230, 0x5859a0, 0x8ee1a8, 4,
         0x587530, 0x588d30, 0x5875e0, 0x588d80},
        {"", 0x2c, 0x848248, 0x5859e0, 0x8ee188, 2,
         0x5876a0, 0x588db0, 0x587750, 0x588df0},
        {"", 0x2c, 0x848260, 0x585a20, 0x8ee138, 5,
         0x587810, 0x588e40, 0x5878c0, 0x588c10},
        {"", 0x2c, 0x848278, 0x585a60, 0x8ee108, 3,
         0x587980, 0x588c80, 0x587a30, 0x588cc0},
        {"", 0x2c, 0x848290, 0x585ad0, 0x8ee0a8, 6,
         0x587af0, 0x588d20, 0x587ba0, 0x588d70},
    }};
    for (std::size_t index = 0; index < kRoleFields.size(); ++index) {
        Append(entries_, runtime,
               {ScriptBindingNameKind::EnumItem, 0x8ee018,
                static_cast<std::uint32_t>(index + 3), 0, 9},
               kRoleFields[index]);
    }
    Append(entries_, runtime,
           {ScriptBindingNameKind::Literal, 0x8b935c, 0, 0, 0,
            "ROLE_RACE"},
           {"", 0x2c, 0x8482a8, 0x585b10, 0x8ef7a8, 7,
            0x587c60, 0x4a09c0, 0x587d10, 0x588d10});
    for (std::uint32_t index = 0; index < 7; ++index) {
        Append(entries_, runtime,
               {ScriptBindingNameKind::EnumLabel, 0x8ef7a8,
                index, 16, 7, nullptr, "_RACE_ATK"},
               {"", 0x24, 0x848200, 0x5852c0, 0, 0,
                0x587dd0, 0x588c00, 0x587e90, 0x588c70, index});
    }

    std::sort(entries_.begin(), entries_.end(),
              [](const auto& left, const auto& right) {
                  return left.name < right.name;
              });
    return entries_;
}

const std::vector<ScriptBindingSpec>& ScriptBindingCatalog::InitializeForwarded(
    IScriptBindingCatalogRuntime& runtime) {
    return Initialize(runtime);
}

void ScriptBindingCatalog::Cleanup() {
    std::vector<ScriptBindingSpec>().swap(entries_);
}

}  // namespace pal4
