#pragma once
#include "pal4/portable/monster_scene_actor.h"
#include "pal4/portable/monster_scene_frame.h"

namespace pal4::portable {
// 单线程场景所有者；网格及所有借用服务必须比本对象活得更久。
class MonsterSceneActors final {
public:
    MonsterSceneActors(MonsterMovementWorld&,MonsterMovement&,MonsterSceneFrame::Obstacles,
                       std::vector<std::unique_ptr<MonsterSceneActor>> actors);
    ~MonsterSceneActors();
    MonsterSceneActors(const MonsterSceneActors&)=delete;
    MonsterSceneActors& operator=(const MonsterSceneActors&)=delete;
    std::size_t Size() const noexcept {return actors_.size();}
    MonsterSceneActor& At(std::size_t index);
    void Advance(float elapsed,std::uint32_t argument);
    MonsterSceneRenderResult Render(std::uint32_t camera,IMonsterSceneRenderServices&);
    void Close();
    bool Closed() const noexcept {return closed_;}
    bool Failed() const noexcept {return render_failed_ || scheduler_.Failed();}
private:
    bool busy_{},closed_{},closing_{},render_failed_{};
    MonsterSceneFrame scheduler_;
    std::vector<std::unique_ptr<MonsterSceneActor>> actors_;
    std::vector<MonsterPursuitFrame*> frames_;
};
}
