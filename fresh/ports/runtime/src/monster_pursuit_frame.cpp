#include "pal4/portable/monster_pursuit_frame.h"
#include <cmath>
#include <stdexcept>
namespace pal4::portable {
namespace {void Require(bool b,const char* message){if(!b)throw std::logic_error(message);}}
struct MonsterPursuitFrame::Impl final:IMonsterBehaviorFrameRuntime,IMonsterPursuitRuntime {
    SceneMonsterInstances& actors;SceneMonsterHandle actor;MonsterBehaviorManagerState& state;
    std::uint32_t token,source,child;MonsterMovementBehavior& behavior;MonsterBaseMotion& motion;IMonsterPursuitSceneServices& services;
    MonsterProximity proximity;MonsterPursuitPaths paths;bool busy{},failed{};
    Impl(SceneMonsterInstances& a,SceneMonsterHandle h,MonsterBehaviorManagerState& s,std::uint32_t id,std::uint32_t src,std::uint32_t c,MonsterMovementBehavior& b,MonsterBaseMotion& m,CollisionSurface& floor,const CollisionMesh& mesh,IMonsterPursuitSceneServices& v)
        :actors(a),actor(h),state(s),token(id),source(src),child(c),behavior(b),motion(m),services(v),
         proximity(a,h,s,id,src,mesh,[this]{return services.ControlledActor();}),
         paths(a,h,s,id,src,m,floor,[this](auto target){return services.Target(target);},[this]{return ResetPursuit(state,token);}){
        behavior.ValidateActor(actors,actor);Require(state.words[0x6d0/4]==child && (child || !actors.Behavior(actor).fuzzy),"pursuit behavior source is not bound");
    }
    void Match(std::uint32_t id) const {Require(busy && id==token,"invalid or inactive pursuit frame callback");}
    void Match(const MonsterBehaviorManagerState& s,std::uint32_t id) const {Match(id);Require(&s==&state,"pursuit manager image mismatch");}
    std::uint32_t ProcessManagerState(MonsterBehaviorManagerState& s,std::uint32_t id,float elapsed) override {Match(s,id);return MonsterBehaviorManager::UpdatePursuitState(s,id,elapsed,*this)?1u:0u;}
    void CheckStateAndProcessCollision(MonsterBehaviorManagerState& s,std::uint32_t id,float elapsed) override {Match(s,id);(void)paths.EnterReturn(elapsed);}
    IScriptEventSchedulerRuntime& SchedulerRuntime() override {throw std::logic_error("pursuit must use the movement owner's scheduler");}
    std::uint32_t UpdatePosition(std::uint32_t id,float elapsed,std::uint32_t argument) override {Match(id);return motion.Update(elapsed,argument).position_result;}
    std::string_view AnimationName(std::uint32_t animation) override {Match(animation-0x170);return actors.Action(actor);}
    std::uint32_t SelectAnimation(std::uint32_t animation,const char* name) override {Match(animation-0x170);Require(name,"invalid pursuit animation name");return actors.SelectAction(actor,name)?1u:0u;}
    IScriptEventProcessorRuntime& ProcessorRuntime() override {Match(token);return behavior.PursuitProcessor(state,token);}
    void CleanupScriptObjectCollection(std::uint32_t address) override {Match(address-0x688);behavior.ClearPursuitTasks(state,token);}
    void LoadFuzzyBehaviorConfig(std::uint32_t id,std::uint32_t config) override {Match(id);Require(config==child,"pursuit fuzzy source changed");behavior.RestorePursuitBehavior(state,token,config);}
    void DispatchComponentField(std::uint32_t id,std::uint32_t slot,std::uint32_t field,std::uint32_t argument) override {
        Match(id);Require(!argument,"invalid pursuit transform argument");const auto& record=actors.Source(actor);
        if(slot==4 && field==source+0x18)actors.SetMotionPosition(actor,record.position);
        else if(slot==8 && field==source+0x24)actors.SetMotionRotation(actor,record.rotation);
        else throw std::logic_error("invalid pursuit source transform field");
    }
    bool CheckProximity(MonsterBehaviorManagerState& s,std::uint32_t id,float) override {Match(s,id);return proximity.Check();}
    bool PlayerControlBusy() override {Match(token);return services.PlayerControlBusy();}
    bool CheckDistanceAndFlag(const MonsterBehaviorManagerState& s,std::uint32_t id,float) override {Match(s,id);return proximity.BeyondReturnThreshold();}
    bool CheckTargetReached(const MonsterBehaviorManagerState& s,std::uint32_t id,float) override {Match(s,id);return proximity.CheckReturnTargetReached();}
    void ProcessCollisionAndConfig(MonsterBehaviorManagerState& s,std::uint32_t id,float elapsed) override {Match(s,id);(void)paths.EnterReturn(elapsed);}
    bool ProcessPursuitCollisionAndSpeed(const MonsterBehaviorManagerState& s,std::uint32_t id,float elapsed,std::uint32_t target) override {Match(s,id);return paths.Process(elapsed,target);}
    bool ProcessObjectInteraction(MonsterBehaviorManagerState& s,std::uint32_t id,std::uint32_t target,float elapsed) override {Match(s,id);return services.ProcessObjectInteraction(s,id,target,elapsed);}
    void ProcessConfigAndEvents(std::uint32_t id,float elapsed) override {Match(id);services.DispatchCombatPositions(id,elapsed);}
    void InitializeCombatSystem(std::uint32_t id,std::int32_t argument) override {Match(id);services.InitializeCombat(id,argument);}
    std::uint32_t ResetPursuit(MonsterBehaviorManagerState& s,std::uint32_t id) override {Match(s,id);return MonsterPursuitReset::Reset(s,id,*this);}
    void SetRecordErrorFlag(std::uint32_t record,std::uint8_t flag) override {Match(token);Require(flag==1,"invalid pursuit history cleanup flag");services.ClearHistoryRecord(record);}
    void ProcessActionWithScript(std::uint32_t id,float elapsed) override {Match(id);services.ProcessActionWithScript(id,elapsed);}
    void VisitArenaObjects(std::uint32_t id) override {Match(id);services.VisitArenaObjects(id);}
    std::uint32_t Advance(float elapsed,std::uint32_t argument){
        Require(!busy && !failed,"pursuit frame is not ready");Require(std::isfinite(elapsed) && elapsed>=0,"invalid pursuit frame time");
        Require(state.words[0x654/4]==source && state.words[0x6d0/4]==child,"pursuit source binding changed");
        const auto first=state.words[0x724/4],last=state.words[0x728/4];Require(last>=first && (last-first)%16==0 && (last-first)/16<=100000,"invalid pursuit history range");
        busy=true;struct Guard{bool& value;~Guard(){value=false;}} guard{busy};try{return behavior.AdvanceManagerAfterGrid(state,token,elapsed,argument,*this);}catch(...){failed=true;throw;}
    }
};
MonsterPursuitFrame::MonsterPursuitFrame(SceneMonsterInstances& a,SceneMonsterHandle h,MonsterBehaviorManagerState& s,std::uint32_t id,std::uint32_t src,std::uint32_t child,MonsterMovementBehavior& b,MonsterBaseMotion& m,CollisionSurface& floor,const CollisionMesh& mesh,IMonsterPursuitSceneServices& v)
    :impl_(std::make_unique<Impl>(a,h,s,id,src,child,b,m,floor,mesh,v)){}
MonsterPursuitFrame::~MonsterPursuitFrame()=default;
std::uint32_t MonsterPursuitFrame::AdvanceAfterGrid(float elapsed,std::uint32_t argument){return impl_->Advance(elapsed,argument);}
bool MonsterPursuitFrame::Failed() const {return impl_->failed;}
bool MonsterPursuitFrame::Busy() const noexcept{return impl_->busy;}
bool MonsterPursuitFrame::UsesWorld(const MonsterMovementWorld& world) const noexcept{return impl_->behavior.UsesWorld(world);}
}
