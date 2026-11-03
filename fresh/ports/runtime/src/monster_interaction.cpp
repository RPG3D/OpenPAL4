#include "pal4/portable/monster_interaction.h"
#include "pal4/game_vector_math.h"
#include "pal4/monster_behavior_manager.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace pal4::portable {
namespace {
void Require(bool value,const char* message){if(!value)throw std::logic_error(message);}
const GameMathTables& Math(){static const auto tables=[]{GameMathTables t;(void)t.CalculateSquareRootTable();return t;}();return tables;}
}
struct MonsterInteraction::Impl final:IMonsterInteractionRuntime {
    SceneMonsterInstances& actors;SceneMonsterHandle actor;MonsterBehaviorManagerState& state;
    std::uint32_t manager,source;MonsterPlayerView& players;IMonsterInteractionRuntime& services;bool busy{};
    Impl(SceneMonsterInstances& a,SceneMonsterHandle h,MonsterBehaviorManagerState& s,std::uint32_t m,std::uint32_t src,MonsterPlayerView& p,IMonsterInteractionRuntime& v)
        :actors(a),actor(h),state(s),manager(m),source(src),players(p),services(v){
        Require(manager && manager%4==0 && manager<=std::numeric_limits<std::uint32_t>::max()-sizeof(s) && source && state.words[0x654/4]==source,"invalid monster interaction owner");(void)actors.Source(actor);
    }
    bool PlayerControlBusy() override{return services.PlayerControlBusy();}
    MonsterInteractionTarget Target(std::uint32_t id) override{const auto target=players.Target(id);Require(target.active,"monster interaction target inactive");return {target.position};}
    double PlanarDistance(float x,float z) override{return static_cast<float>(GameVectorMath::ApproximateMagnitude({x,0,z},Math()));}
    float PlayerDistanceBase() override{const auto value=services.PlayerDistanceBase();Require(std::isfinite(value) && value>=0,"invalid monster interaction distance");return value;}
    std::array<float,3> TargetForward(std::uint32_t id) override{return players.TargetForward(id);}
    std::array<float,3> ManagerForward(std::uint32_t id) override{Require(id==manager && actors.TransformBound(actor),"invalid monster interaction frame");const auto m=actors.InstanceTransform(actor);return {m[8],m[9],m[10]};}
    void NormalizeDirection(std::array<float,3>& value) override{GameVectorMath::NormalizeAndClamp(value,Math());}
    double AngleForForwardDot(float dot) override{return Math().ApproximateArccosine(dot);}
    std::string ReadScriptName(std::uint32_t id,const std::array<std::uint32_t,4>&,const char* key) override{
        Require(id==source && key && std::string_view(key)=="AAB_MST_ESSBEH_script_func","invalid monster interaction script source");
        const auto* value=actors.Source(actor).cache.Root().FindAttribute(key);if(!value)return {};
        const auto* text=std::get_if<std::string>(&value->value);Require(text,"invalid monster interaction script name");return *text;
    }
    void InitializeScripts() override{services.InitializeScripts();}
    std::string ArenaScriptPath() override{return services.ArenaScriptPath();}
    bool ScriptCached(const std::string& path) override{return services.ScriptCached(path);}
    std::uint32_t CreateInterpreter(const std::string& path,const std::string& full,const std::string& name) override{return services.CreateInterpreter(path,full,name);}
    bool ExecuteInterpreter(std::uint32_t id) override{return services.ExecuteInterpreter(id);}
    void HandleInterpreterError(std::uint32_t id) override{services.HandleInterpreterError(id);}
    void ReleaseInterpreter(std::uint32_t id) override{services.ReleaseInterpreter(id);}
    void ShowMissingScriptError() override{services.ShowMissingScriptError();}
    void CleanupScripts() override{services.CleanupScripts();}
    std::int32_t PendingStateCount(std::uint32_t id) override{Require(id==source && actors.Source(actor).values.size()<=std::numeric_limits<std::int32_t>::max(),"invalid monster interaction source");return static_cast<std::int32_t>(actors.Source(actor).values.size());}
    void ReactivateManager(std::uint32_t id,std::uint32_t flag) override{Require(id==manager,"invalid monster interaction manager");services.ReactivateManager(id,flag);}
    void RestoreRenderCallback(std::uint32_t id) override{Require(id==manager,"invalid monster interaction manager");services.RestoreRenderCallback(id);}
    bool Process(std::uint32_t target){
        Require(!busy && state.words[0x654/4]==source && actors.TransformBound(actor),"monster interaction unavailable");
        const auto pose=actors.MotionTransform(actor);
        for(unsigned axis=0;axis<3;++axis)Require(std::bit_cast<float>(state.words[43+axis])==pose.position[axis] && std::bit_cast<float>(state.words[40+axis])==pose.rotation[axis],"monster interaction transform is stale");
        busy=true;struct Guard{bool& value;~Guard(){value=false;}} guard{busy};
        try{return MonsterInteractionProcessor::Process(state,manager,target,*this);}
        catch(...){try{services.CleanupScripts();}catch(...){}throw;}
    }
};
MonsterInteraction::MonsterInteraction(SceneMonsterInstances& a,SceneMonsterHandle h,MonsterBehaviorManagerState& s,std::uint32_t m,std::uint32_t src,MonsterPlayerView& p,IMonsterInteractionRuntime& v)
    :impl_(std::make_unique<Impl>(a,h,s,m,src,p,v)){}
MonsterInteraction::~MonsterInteraction()=default;
bool MonsterInteraction::Process(std::uint32_t target){return impl_->Process(target);}
}
