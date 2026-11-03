#pragma once
#include "pal4/player_manager_leader_transition.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace pal4 {
class NativeGameDatabase;
struct PlayerManagerObjectState;
// Live persistent player properties plus existing original Scene/Upgrade catalogs.
// The player manager/database outlive this adapter. No UI or scene actor ownership.
class NativeRoleStateData final : public IPlayerManagerPriorityRuntime {
public:
    NativeRoleStateData(PlayerManagerObjectState&,const NativeGameDatabase&);
    ~NativeRoleStateData();
    std::uint32_t PlayerManager() const noexcept;
    std::uint32_t PlayerByIndex(std::uint32_t manager,std::uint32_t index) const;
    std::uint32_t ActiveCount(std::uint32_t manager) const;
    void Prepare(std::uint32_t script,std::uint32_t order);
    std::int32_t Element(std::uint32_t script,std::uint32_t index) const;
    std::int32_t Maximum(std::uint32_t script,std::uint32_t index) const;
    std::int32_t Current(std::uint32_t script,std::uint32_t index) const;
    std::int32_t GetPriority(std::uint32_t script) override;
    void Sort(std::vector<std::uint32_t>& players);
    std::uint32_t TriggerText(std::uint32_t id) const;
    std::int32_t NextLevelThreshold(std::uint32_t role,std::int32_t next_level) const;
    std::uint32_t SceneRecords() const noexcept;
    std::uint32_t SceneCount() const noexcept;
    bool SceneMatches(std::uint32_t entry,std::string world,std::string area) const;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
