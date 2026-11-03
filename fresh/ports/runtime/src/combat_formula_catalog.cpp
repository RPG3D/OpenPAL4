#include "pal4/portable/combat_formula_catalog.h"

#include "pal4/game_data_ai_record_sorter.h"
#include "pal4/portable/original_database.h"

#include <algorithm>
#include <stdexcept>

namespace pal4::portable {
namespace {

class FormulaTableRuntime final : public IGameDataCombatFormulaRuntime {
public:
    explicit FormulaTableRuntime(const std::optional<OriginalTable>& table)
        : table_(table ? &*table : nullptr) {}

    bool OpenTable(std::string_view name) override {
        if (name != "CombatFormulaConfig" || !table_) return false;
        if (table_->fields.size() < 2u || table_->fields[0].type != 1u ||
            table_->fields[1].type != 3u)
            throw std::runtime_error("CombatFormulaConfig key/float schema mismatch");

        keys_.clear();
        keys_.reserve(table_->rows.size());
        for (std::size_t index = 0u; index < table_->rows.size(); ++index) {
            const auto& row = table_->rows[index];
            if (row.size() < 2u ||
                !std::holds_alternative<std::uint32_t>(row[0]) ||
                !std::holds_alternative<float>(row[1]))
                throw std::runtime_error("CombatFormulaConfig key/float row mismatch");
            keys_.push_back({std::get<std::uint32_t>(row[0]),
                             static_cast<std::uint32_t>(index)});
        }
        std::sort(keys_.begin(), keys_.end(),
                  [](const auto& first, const auto& second) {
                      return first.key < second.key;
                  });
        for (std::size_t index = 1u; index < keys_.size(); ++index)
            if (keys_[index - 1u].key == keys_[index].key)
                throw std::runtime_error("CombatFormulaConfig duplicate key");
        opened_ = true;
        return true;
    }

    std::span<const GameDataAiKeyEntry> Keys() override { return keys_; }

    bool HasRow(std::uint32_t row_index) override {
        return opened_ && table_ && row_index < table_->rows.size();
    }

    float ReadFloatProperty(std::uint32_t row_index,
                            std::uint32_t property_index) override {
        if (!HasRow(row_index) || property_index != 1u)
            throw std::out_of_range("CombatFormulaConfig float property");
        return std::get<float>(table_->rows[row_index][property_index]);
    }

    void LogError(std::string_view message) override {
        errors_.emplace_back(message);
    }

    int LogRangeWarning(std::uint32_t source_line,
                        std::string_view detail) override {
        warnings_.push_back(std::to_string(source_line) + ": " +
                            std::string(detail));
        return 0;
    }

    const std::vector<std::string>& Warnings() const noexcept {
        return warnings_;
    }

private:
    const OriginalTable* table_{};
    std::vector<GameDataAiKeyEntry> keys_;
    std::vector<std::string> errors_, warnings_;
    bool opened_{};
};

struct LoadedFormulaTable final {
    GameDataDatabaseHeader header;
    std::vector<std::string> warnings;
};

LoadedFormulaTable LoadFormulaCatalog(const OriginalDatabaseCatalogs& database) {
    LoadedFormulaTable result;
    (void)result.header.Reset();
    FormulaTableRuntime runtime(database.combat_formula_config);
    (void)result.header.LoadCombatFormulas(runtime);
    result.warnings = runtime.Warnings();
    return result;
}

} // namespace

CombatFormulaCatalog CombatFormulaCatalog::ReadFromGameData(
    const std::filesystem::path& game_data) {
    auto loaded = LoadFormulaCatalog(ReadOriginalDatabasePackage(game_data));
    CombatFormulaCatalog result;
    result.header_ = std::move(loaded.header);
    result.warnings_ = std::move(loaded.warnings);
    return result;
}

CombatFormulaCatalog CombatFormulaCatalog::ReadDatabase(
    std::span<const std::uint8_t> database) {
    auto loaded = LoadFormulaCatalog(ReadOriginalDatabase(database));
    CombatFormulaCatalog result;
    result.header_ = std::move(loaded.header);
    result.warnings_ = std::move(loaded.warnings);
    return result;
}

float CombatFormulaCatalog::FloatAt(std::int32_t index) const {
    return static_cast<float>(header_.FloatAt(index));
}

const std::vector<std::string>& CombatFormulaCatalog::Warnings() const noexcept {
    return warnings_;
}

} // namespace pal4::portable
