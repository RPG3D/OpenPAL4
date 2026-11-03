#pragma once
#include "pal4/ai_action_task.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
namespace pal4 {
class NativeGameDatabase;
class NativeCombatPlayerManager;
class NativeCombatMonsterManager;
class NativeCombatActorSkillRuntime;
class INativeSkillSystemServices;
class NativeScriptEventRouter;
struct NativeCombatAiDecision {
    std::uint32_t actor{},ai_type{},config{},property{};
    AiTeamState team{AiTeamState::MissingRecord};
    AiEnemyState enemy{AiEnemyState::MissingRecord};
};
// Owns numeric AiConfig/AiProperty records; borrows actual rosters/skills/router.
// Chooses the original decision row, without creating an action or changing RNG.
class NativeCombatAiDecisionRuntime final {
public:
    NativeCombatAiDecisionRuntime(const NativeGameDatabase&,NativeCombatPlayerManager&,
        NativeCombatMonsterManager&,std::span<NativeCombatActorSkillRuntime* const>,
        INativeSkillSystemServices&,NativeScriptEventRouter&);
    ~NativeCombatAiDecisionRuntime();
    NativeCombatAiDecision Choose(std::uint32_t actor);
    std::uint32_t ReadPropertyWord(std::uint32_t address)const;
    std::span<const std::array<std::uint32_t,14>> Configs()const noexcept;
    std::span<const std::array<std::uint32_t,17>> Properties()const noexcept;
    std::size_t ActiveIterators()const noexcept;
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
