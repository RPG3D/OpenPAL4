#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <vector>
namespace pal4 {
class NativeCombatMainWindow;
class NativeCombatPlayerManager;
class NativeCombatMonsterManager;
class NativeCombatActorSkillRuntime;
class NativeCombatSkillActorRegistry;
// Real consumers for14 of the19 Main subscriptions. Does not subscribe by
// itself: the complete frame lifecycle must register/unregister all19 together.
class NativeCombatMainUiEvents final {
public:
    NativeCombatMainUiEvents(NativeCombatMainWindow&,NativeCombatPlayerManager&,
        NativeCombatMonsterManager&,std::span<NativeCombatActorSkillRuntime* const>,
        std::function<std::uint32_t()> game_state);
    // Resolve current Player skills during creation/stage events; no snapshot
    // of an empty pre-creation roster is retained.
    NativeCombatMainUiEvents(NativeCombatMainWindow&,NativeCombatPlayerManager&,
        NativeCombatMonsterManager&,NativeCombatSkillActorRegistry&,
        std::function<std::uint32_t()> game_state);
    static bool Supports(std::uint32_t callback) noexcept;
    // Unsupported domain entries return false before consuming their payload.
    bool Handle(std::uint32_t callback,std::uint32_t payload);
private:
    NativeCombatMainWindow& window_;
    NativeCombatPlayerManager& players_;
    NativeCombatMonsterManager& monsters_;
    std::vector<NativeCombatActorSkillRuntime*> player_skills_;
    NativeCombatSkillActorRegistry* registry_{};
    std::function<std::uint32_t()> game_state_;
};
}
