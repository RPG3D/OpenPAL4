#include "pal4/portable/combat_system_cold_runtime.h"

#include <limits>
#include <stdexcept>
#include <utility>

namespace pal4::portable {

CombatSystemColdRuntimeAdapter::CombatSystemColdRuntimeAdapter(
    std::uint32_t logical_system_owner, std::filesystem::path game_data,
    ICombatSystemColdRuntime& platform)
    : owner_(logical_system_owner), game_data_(std::move(game_data)),
      platform_(platform) {
    if (!owner_ || owner_ > std::numeric_limits<std::uint32_t>::max() - 517u ||
        game_data_.empty())
        throw std::invalid_argument("Combat cold runtime requires an owner and game-data path");
}

bool CombatSystemColdRuntimeAdapter::TranslatorLoaded() const noexcept {
    return translations_.has_value();
}

std::size_t CombatSystemColdRuntimeAdapter::TranslationCount() const {
    if (!translations_)
        throw std::logic_error("Combat translator has not been loaded");
    return translations_->Size();
}

std::optional<std::string_view> CombatSystemColdRuntimeAdapter::FindTranslation(
    std::string_view english) const {
    if (!translations_)
        throw std::logic_error("Combat translator has not been loaded");
    return translations_->Find(english);
}

void CombatSystemColdRuntimeAdapter::ResetTranslator() noexcept {
    translations_.reset();
}

void CombatSystemColdRuntimeAdapter::WriteByte(std::uint32_t address,
                                               std::uint8_t value) {
    if (address == owner_ + 517u && value == 1u && !translations_)
        throw std::logic_error("Combat cold commit requires the original translator");
    platform_.WriteByte(address, value);
}

void CombatSystemColdRuntimeAdapter::LogInitialization(bool beginning) {
    platform_.LogInitialization(beginning);
}

std::uint32_t CombatSystemColdRuntimeAdapter::LogInitialized() {
    return platform_.LogInitialized();
}

void CombatSystemColdRuntimeAdapter::GetSkillSystem() {
    platform_.GetSkillSystem();
}

void CombatSystemColdRuntimeAdapter::LoadTranslator(
    std::uint32_t translator, std::string_view path) {
    if (translator != owner_ + 380u || path != "gamedata\\database\\combatDb.db")
        throw std::invalid_argument("Combat translator owner or original path mismatch");
    auto loaded = CombatTranslationCatalog::ReadFromGameData(game_data_);
    translations_.emplace(std::move(loaded));
}

std::uint32_t CombatSystemColdRuntimeAdapter::GetConsole() {
    return platform_.GetConsole();
}

void CombatSystemColdRuntimeAdapter::SubscribeConsoleEvents(
    std::uint32_t console) {
    platform_.SubscribeConsoleEvents(console);
}

void CombatSystemColdRuntimeAdapter::InitializeMatFx(std::uint32_t object) {
    platform_.InitializeMatFx(object);
}

} // namespace pal4::portable
