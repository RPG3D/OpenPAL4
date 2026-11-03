#pragma once

#include "pal4/combat_system_session.h"
#include "pal4/portable/combat_translation_catalog.h"

#include <filesystem>
#include <optional>
#include <string_view>

namespace pal4::portable {

// Supplies the original CombatTranslation dependency during the shared cold
// initializer and forwards all other operations to the platform owners.
class CombatSystemColdRuntimeAdapter final : public ICombatSystemColdRuntime {
public:
    CombatSystemColdRuntimeAdapter(std::uint32_t logical_system_owner,
                                   std::filesystem::path game_data,
                                   ICombatSystemColdRuntime& platform);

    CombatSystemColdRuntimeAdapter(const CombatSystemColdRuntimeAdapter&) = delete;
    CombatSystemColdRuntimeAdapter& operator=(const CombatSystemColdRuntimeAdapter&) = delete;

    [[nodiscard]] bool TranslatorLoaded() const noexcept;
    [[nodiscard]] std::size_t TranslationCount() const;
    [[nodiscard]] std::optional<std::string_view> FindTranslation(
        std::string_view english) const;
    void ResetTranslator() noexcept;

    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void LogInitialization(bool beginning) override;
    [[nodiscard]] std::uint32_t LogInitialized() override;
    void GetSkillSystem() override;
    void LoadTranslator(std::uint32_t translator,
                        std::string_view path) override;
    [[nodiscard]] std::uint32_t GetConsole() override;
    void SubscribeConsoleEvents(std::uint32_t console) override;
    void InitializeMatFx(std::uint32_t object) override;

private:
    std::uint32_t owner_;
    std::filesystem::path game_data_;
    ICombatSystemColdRuntime& platform_;
    std::optional<CombatTranslationCatalog> translations_;
};

} // namespace pal4::portable
