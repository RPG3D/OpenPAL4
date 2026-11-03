#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <functional>

namespace pal4 {

class ICombatEndingWindowLevelUpRuntime {
public:
    virtual ~ICombatEndingWindowLevelUpRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string ReadText(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string GameString(std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string FormatLevelNotice(
        std::string_view format, std::string_view actor_name,
        std::int32_t level) = 0;
    [[nodiscard]] virtual std::string FormatActorNotice(
        std::string_view format, std::string_view actor_name) = 0;
    using LearnedNameVisitor=std::function<void(std::string_view)>;
    virtual void VisitLearnedNames(std::uint32_t actor_list,std::uint32_t actor,std::int32_t level,const LearnedNameVisitor&);
    virtual void SetWindowText(std::uint32_t window,
                               std::string_view text) = 0;
    virtual void SetHintFlag(std::uint32_t hint_window,
                             std::uint32_t flag) = 0;
    virtual void ShowHint(std::uint32_t hint_window,
                          std::string_view text, std::uint32_t position,float duration,
                          std::uint32_t first_flag,
                          std::uint32_t second_flag) = 0;
    [[nodiscard]] virtual std::uint32_t CombatLevelUp(
        std::uint32_t actor) = 0;
    virtual void RegisterActorEvent(std::uint32_t event) = 0;
    virtual void PlaySound(std::string_view name, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t SpawnEffect(
        std::uint32_t actor, std::string_view name,
        std::uint32_t effect_context) = 0;
    virtual void ResetEffectHandle(std::uint32_t handle) = 0;
};

class CombatEndingWindowLevelUp final {
public:
    // IDA 0x542f00.
    static void Apply(std::uint32_t score_row, std::int32_t new_level,
                      std::uint32_t actor, std::uint32_t hint_window,
                      std::uint32_t actor_list,
                      ICombatEndingWindowLevelUpRuntime& runtime);
};

}  // namespace pal4
