#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace pal4::portable {

// Owns the original CombatTranslation table loaded from database.cpk. Text is
// retained byte-for-byte; returned views live until this catalog is destroyed.
class CombatTranslationCatalog final {
public:
    static CombatTranslationCatalog ReadFromGameData(
        const std::filesystem::path& game_data);
    static CombatTranslationCatalog ReadDatabase(
        std::span<const std::uint8_t> database);

    [[nodiscard]] std::optional<std::string_view> Find(
        std::string_view english) const noexcept;
    [[nodiscard]] std::size_t Size() const noexcept;

private:
    std::vector<std::pair<std::string, std::string>> rows_;
};

} // namespace pal4::portable
