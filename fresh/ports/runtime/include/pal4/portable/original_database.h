#pragma once
#include "pal4/portable/inventory.h"
#include "pal4/portable/text_catalog.h"
#include "pal4/portable/role.h"
#include <filesystem>
#include <optional>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pal4::portable {
struct OriginalSceneDefinition {std::uint32_t id{},type{},earthball{};std::string world,scene,title;};
struct OriginalCombatArea {std::uint32_t id{};std::int32_t floor{};std::string world,area;};
struct OriginalIntroduction {std::uint32_t id{};std::string name,text;};
struct OriginalHelpTopic {std::uint32_t id{};std::string title;};
struct OriginalHelpTip {
    std::uint32_t id{},topic_id{};
    std::string title,text;
    std::int32_t unlock_progress{};
};
struct OriginalTable {
    std::vector<RoleField> fields;
    std::vector<std::vector<RoleValue>> rows;
};
struct OriginalDatabaseCatalogs {
    PropertyCatalog properties;
    TextCatalog tutorials;
    TextCatalog game_strings;
    std::vector<OriginalSceneDefinition> scenes;
    std::vector<OriginalCombatArea> combat_areas;
    std::vector<OriginalIntroduction> introductions;
    // Keep original enumeration/table order: HelpWindow uses it for row indices.
    std::vector<OriginalHelpTopic> help_topics;
    std::vector<OriginalHelpTip> help_tips;
    std::unordered_map<std::string,OriginalTable> player_tables;
    std::optional<OriginalTable> combat_formula_config;
};
OriginalDatabaseCatalogs ReadOriginalDatabase(std::span<const std::uint8_t> bytes);
OriginalDatabaseCatalogs ReadOriginalDatabasePackage(const std::filesystem::path& game_data);
std::string ResolveCombatArea(const std::vector<OriginalCombatArea>& areas,
    std::string_view world, std::int32_t current, std::int32_t fallback);
}
