#pragma once

#include "pal4/game_data_database_header.h"

#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace pal4::portable {

// Copies CombatFormulaConfig from Pal4db.db through the portable typed-table
// reader and shared 32-key GameDataDatabaseHeader loader.
class CombatFormulaCatalog final {
public:
    [[nodiscard]] static CombatFormulaCatalog ReadFromGameData(
        const std::filesystem::path& game_data);
    [[nodiscard]] static CombatFormulaCatalog ReadDatabase(
        std::span<const std::uint8_t> database);

    [[nodiscard]] float FloatAt(std::int32_t index) const;
    [[nodiscard]] const std::vector<std::string>& Warnings() const noexcept;

private:
    GameDataDatabaseHeader header_;
    std::vector<std::string> warnings_;
};

} // namespace pal4::portable
