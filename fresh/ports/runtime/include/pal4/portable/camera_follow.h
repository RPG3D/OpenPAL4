#pragma once
#include "pal4/portable/game_camera.h"
#include <functional>
#include <optional>
namespace pal4::portable {
struct CameraFollowTarget {std::array<float,3> position{},rotation{};float radius{};bool auto_seek{};};
struct CameraListener {std::array<float,3> position{},forward{},up{};unsigned updates{};};
// 借用typed角色快照，不持有原对象/平台/GPU地址。
class CameraFollow {
public:
    using Target=std::function<std::optional<CameraFollowTarget>()>;
    CameraFollow(GameCameraSystem&,Target,std::function<void(bool)> sync_player);
    ~CameraFollow();
    void SetMode(unsigned mode,bool sync_player);
    void SeekToPlayer();
    void FlushTailYaw();
    void FollowPlayer();
    void Advance();
    const CameraListener& Listener() const;
    void WriteReport(std::ostream&) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
