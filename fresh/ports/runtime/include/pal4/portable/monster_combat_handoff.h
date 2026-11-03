#pragma once
#include "pal4/portable/monster_scene_actor.h"
#include <optional>

namespace pal4::portable {
// 场景/战斗系统拥有参数文档、名称集合及状态切换；回调必须同步消费数据。
class IMonsterCombatHandoffServices {
public:
    virtual ~IMonsterCombatHandoffServices()=default;
    virtual std::optional<std::int32_t> CurrentStateId()=0;
    virtual void CreateDocument(std::string_view root)=0;
    virtual std::string ResolveArenaPath(std::string_view arena)=0;
    virtual void ClearStateNames()=0;
    virtual void AddStateName(std::string_view name)=0;
    virtual std::size_t StateNameCount()=0;
    virtual void SetCombatWorld(std::string_view node,std::string_view path,std::uint32_t mode,std::int32_t count)=0;
    virtual void SetCombatMonster(std::uint32_t id,std::uint32_t value,std::uint32_t group)=0;
    virtual void EnterState(std::uint32_t id)=0;
    virtual std::int32_t DestroyDocument() noexcept=0;
};
class MonsterCombatHandoff final {
public:
    explicit MonsterCombatHandoff(IMonsterCombatHandoffServices&);
    ~MonsterCombatHandoff();
    MonsterCombatHandoff(const MonsterCombatHandoff&)=delete;
    MonsterCombatHandoff& operator=(const MonsterCombatHandoff&)=delete;
    std::int32_t Process(MonsterSceneActor&,std::int32_t mode,std::string_view arena);
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
