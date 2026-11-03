#pragma once
#include "pal4/portable/monster_movement.h"
#include "pal4/portable/monster_pursuit_frame.h"
#include <functional>
#include <span>
#include <vector>

namespace pal4::portable {
// 借用同一场景代次的网格、移动服务和怪物帧；调用方负责对象生命周期。
// 异常可能已推进部分状态，失败后必须销毁整个场景并重建。
class MonsterSceneFrame final {
public:
    using Obstacles=std::function<std::vector<MonsterMovementObstacle>(MonsterHelperHandle)>;
    MonsterSceneFrame(MonsterMovementWorld&,MonsterMovement&,Obstacles);
    MonsterSceneFrame(const MonsterSceneFrame&)=delete;
    MonsterSceneFrame& operator=(const MonsterSceneFrame&)=delete;
    void Advance(float elapsed,std::uint32_t argument,std::span<MonsterPursuitFrame* const> actors);
    bool Failed() const noexcept { return failed_; }
private:
    MonsterMovementWorld& world_;
    MonsterMovement& movement_;
    Obstacles obstacles_;
    bool busy_{},failed_{};
};
}
