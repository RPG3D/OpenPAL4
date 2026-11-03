#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCombatReport;struct NativeCombatSystemImage;
class NativePalGameIVTextureStorage;class NativeCombatEventHandlers;
class NativeCombatPlayerManager;class NativeInventorySession;class NativePlayerEventRuntime;
class NativeCombatFormulaCatalog;class NativeGameDatabase;class INativeSkillSystemServices;
class NativeCombatActorSkillRuntime;
// Original5715F0 over real report, catalog, inventory, GameData, first Player
// roster, SkillSystem RNG and Player growth/learning owners. No ending UI.
class NativeCombatRewards final {
public:
    using ActorSkills=std::function<NativeCombatActorSkillRuntime&(std::uint32_t)>;
    NativeCombatRewards(NativeCombatSystemImage&,NativePalGameIVTextureStorage&,NativeCombatEventHandlers&,
        NativeCombatPlayerManager&,NativeInventorySession&,NativePlayerEventRuntime&,
        const NativeCombatFormulaCatalog&,const NativeGameDatabase&,INativeSkillSystemServices&,ActorSkills);
    ~NativeCombatRewards();
    // Takes the original by-value source ownership. Actual grants and actor
    // changes are not rolled back on failure. Output owns all maps/learned IDs.
    std::unique_ptr<NativeCombatReport> Process(std::unique_ptr<NativeCombatReport> source);
    void Close();
    std::size_t PendingReports() const noexcept;
    std::size_t ProcessedReports() const noexcept;
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
