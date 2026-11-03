#pragma once
#include "pal4/portable/script_stack.h"
#include "pal4/portable/world.h"
#include <memory>
namespace pal4::portable {
class SceneNpcs;
struct CollisionMesh;
struct NpcWalkCompletion {ScriptToken token;std::string error;};
// 借用场景NPC；复用原脚本行走处理器与公共地面运动，不推进动画。
class NpcWalks {
public:
    NpcWalks(SceneNpcs&,const CollisionMesh&,const Scene&);
    ~NpcWalks();
    ScriptResponse Start(ScriptToken,std::string_view name,Point3 target,bool run,bool wait);
    ScriptResponse End(ScriptToken,std::string_view name);
    std::vector<NpcWalkCompletion> Advance(float elapsed);
    bool Requested() const;
    void WriteReport(std::ostream&) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
