#pragma once
#include "pal4/script_gameplay_callbacks.h"
#include "pal4/native_monster_pursuit_path_runtime.h"
#include "pal4/monster_behavior_manager.h"

namespace pal4 {
// Scene-scoped script command adapter. Owns no monsters, collision trees or
// path buffers; the normal scene release must retire this borrower first.
class NativeScriptMonsterPursuitRuntime final : public IMonsterPursuitCommandRuntime,
                                               private IMonsterStopPursuitRuntime {
public:
    NativeScriptMonsterPursuitRuntime(NativeMonsterManagerOwner&,
        NativeArenaCollisionTree&,NativeBaseObjectMemory&);
    void SetStopPursuitFlag(std::uint32_t) override;
    std::uint32_t MonsterManager() override;
    std::uint32_t Iterate(std::uint32_t,Callback,std::uint32_t) override;
private:
    std::uint32_t ProcessCollisionAndConfig(std::uint32_t,float,float) override;
    NativeMonsterManagerOwner& monsters_;
    NativeMonsterPursuitPathRuntime paths_;
};
}
