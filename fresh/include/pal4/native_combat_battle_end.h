#pragma once
#include <cstdint>
#include <cstddef>
#include <memory>
#include <functional>
#include <string_view>
namespace pal4 {
struct NativeCombatSystemImage;
class NativePalGameIVTextureStorage;
class NativeCombatConsole;
class NativeCombatTimedEvents;
class NativeCombatGameEventHandler;class NativeCombatMonsterManager;class INativeSkillSystemServices;
class NativeCombatRewards;
class NativeCombatReport;
//57A920 over real game entries, console subscriptions and member412 queue.
// Victory applies real rewards; ending and defeat presentation borrow explicit
// native services. Missing bindings fail before presenting fake completion.
// Original576440 EndSequence is dispatched later by the borrowed timed owner.
class NativeCombatBattleEnd final {
public:
    NativeCombatBattleEnd(NativeCombatSystemImage&,NativePalGameIVTextureStorage&,NativeCombatConsole&,NativeCombatTimedEvents&,
        NativeCombatGameEventHandler&,NativeCombatMonsterManager&,INativeSkillSystemServices&,NativeCombatRewards&);
    ~NativeCombatBattleEnd();
    std::uint32_t Handle(std::uint32_t payload);
    void BindEnding(std::function<void(std::uint32_t system,std::uint32_t prize)>);
    using DefeatBanner=std::function<void(std::string_view,std::string_view,float,float,float)>;
    void BindDefeat(DefeatBanner);
    // Valid only inside the synchronous ending/prize callback.
    const NativeCombatReport& BorrowPrize(std::uint32_t address)const;
    void Close();
    std::size_t PendingCallbacks() const noexcept;
    std::size_t PendingReports() const noexcept;
    std::size_t PreparedReports() const noexcept;
    std::size_t CopiedReports() const noexcept;
    std::size_t ProcessedReports() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
