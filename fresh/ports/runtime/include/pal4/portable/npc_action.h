#pragma once
#include "pal4/portable/script_stack.h"
#include <memory>
#include <iosfwd>
namespace pal4::portable {
class SceneNpcs;
struct NpcActionCompletion {ScriptToken token;std::string error;};
// 借用场景NPC及其唯一播放器；只推进原脚本处理器，不重复推进动画。
class NpcActions {
public:
    explicit NpcActions(SceneNpcs& npcs);
    ~NpcActions();
    NpcActions(const NpcActions&)=delete;
    NpcActions& operator=(const NpcActions&)=delete;
    ScriptResponse Start(ScriptToken token,std::string_view name,std::string_view action,std::int32_t execute_flag,bool wait);
    ScriptResponse End(ScriptToken token,std::string_view name);
    std::vector<NpcActionCompletion> Advance(float elapsed);
    bool Requested() const;
    void WriteReport(std::ostream& output) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
