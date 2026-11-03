#include "pal4/portable/monster_movement_behavior.h"
#include "pal4/portable/monster_object_members.h"
#include "pal4/portable/monster_base_motion.h"
#include "pal4/base_object_activity.h"
#include "monster_behavior_scheduler.h"
#include "pal4/portable/monster_random_state.h"
#include "pal4/portable/world.h"
#include <algorithm>
#include <cmath>
#include <exception>
#include <iomanip>
#include <limits>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace pal4::portable {
namespace {
void Require(bool value,const char* message){if(!value)throw std::logic_error(message);}
struct Busy {bool& value;explicit Busy(bool& v):value(v){Require(!v,"movement behavior owner is not reentrant");v=true;}~Busy(){value=false;}};
}
struct MonsterMovementBehavior::Impl final:IMonsterBehaviorActionFactory {
    MonsterMovementWorld& world;SceneMonsterInstances& actors;SceneMonsterHandle actor;
    BinaryXmlNode& memory;MonsterOrientation& orientation;Radius radius;Identifier identifier;
    MonsterMovementBehaviorStats stats;std::exception_ptr failure;bool busy{},closed{};
    MonsterBehaviorManagerState* manager_image{};std::uint32_t manager_token{};std::uint64_t container_binding{};
    void BindContainer(MonsterBehaviorManagerState& image,std::uint32_t token){
        Require(token && token%4==0 && token<=std::numeric_limits<std::uint32_t>::max()-sizeof(image),"invalid manager task container identity");
        if(object_members){object_members->ValidateManager(image,token);(void)queue->Ready();return;}
        if(manager_image){Require(manager_image==&image && manager_token==token && queue->OwnsContainerImage(container_binding),"manager task container belongs to another owner");(void)queue->Ready();return;}
        container_binding=queue->BindContainerImage(std::span<std::uint32_t,4>(image.words.data()+MonsterBehaviorManager::kScriptOffset/4,4),token+MonsterBehaviorManager::kScriptOffset);
        manager_image=&image;manager_token=token;
    }
    detail::MonsterBehaviorScheduler* scheduler{};MonsterBehaviorTasks* queue{};
    std::unique_ptr<detail::MonsterBehaviorScheduler> standalone_scheduler;
    // 拥有者在工厂字段之前销毁；两种路径各自只拥有一套队列/处理器。
    std::unique_ptr<MonsterBehaviorTasks> standalone_queue;
    std::unique_ptr<MonsterObjectMembers> object_members;
    Impl(MonsterMovementWorld& w,SceneMonsterInstances& a,SceneMonsterHandle h,BinaryXmlNode& m,MonsterOrientation& o,Radius r,Identifier id,
         MonsterBehaviorManagerState* image=nullptr,std::uint32_t token=0,ObjectMemory* storage=nullptr,MonsterBehaviorPreparation preparation=MonsterBehaviorPreparation::Immediate)
        :world(w),actors(a),actor(h),memory(m),orientation(o),radius(std::move(r)),identifier(std::move(id)){
        Require(bool(radius) && bool(identifier),"movement behavior requires actual radius and shared identifiers");(void)actors.Model(actor);
        Require(preparation==MonsterBehaviorPreparation::Immediate || preparation==MonsterBehaviorPreparation::Component,"invalid monster behavior preparation mode");
        if(image){
            Require(storage,"constructed movement owner requires shared storage");const auto before=image->words;const auto old_name=memory.name;
            try{
                object_members=std::make_unique<MonsterObjectMembers>(*image,token,*storage,*this,actors.Behavior(actor),memory,[this]{return world.Rand();});
                if(preparation==MonsterBehaviorPreparation::Immediate)object_members->PrepareBehavior();
                queue=&object_members->Tasks();scheduler=&object_members->Scheduler();manager_image=image;manager_token=token;
            }catch(...){if(object_members){object_members->Close();object_members.reset();}image->words=before;memory.name=old_name;throw;}
        }else{
            standalone_queue=std::make_unique<MonsterBehaviorTasks>(*this,[this]{return world.Rand();});queue=standalone_queue.get();
            standalone_scheduler=std::make_unique<detail::MonsterBehaviorScheduler>(actors.Behavior(actor),detail::MonsterSchedulerBindings{
                [this]{return queue->Ready();},[this]{return queue->Start(actors.Behavior(actor));},
                [this](float elapsed){return queue->Advance(elapsed);},[this]{return world.Rand();}},memory);scheduler=standalone_scheduler.get();
        }
    }
    void Error(std::exception_ptr error) noexcept {if(!failure)failure=std::move(error);++stats.failures;}
    void Check() const {Require(!closed,"movement behavior is closed");if(failure)std::rethrow_exception(failure);}
    float RadiusValue(){const auto value=radius();Require(std::isfinite(value) && value>=0,"invalid movement behavior manager radius");return value;}
    MonsterTransformRows Transform(){
        const auto matrix=actors.InstanceTransform(actor);MonsterTransformRows rows{};
        for(unsigned r=0;r<4;++r){
            for(unsigned axis=0;axis<3;++axis)rows[r][axis]=matrix[r*4+axis];
        }
        return rows;
    }
    struct Action final:IMonsterBehaviorAction {
        Impl& owner;std::int32_t kind;std::optional<MonsterHelperHandle> helper;bool entered{},released{};
        Action(Impl& o,std::int32_t k):owner(o),kind(k){helper=kind==3?owner.world.CreateRandom(owner.actor,owner.identifier()):owner.world.CreateEndpoint(owner.actor,owner.identifier());++owner.stats.constructed;++owner.stats.live_actions;}
        ~Action() override {Discard();--owner.stats.live_actions;}
        void Discard() noexcept {
            if(!helper)return;
            try{if(owner.world.Contains(*helper)){owner.world.DestroyHelper(*helper);++owner.stats.discarded;}}
            catch(...){owner.Error(std::current_exception());}
            helper.reset();
        }
        MonsterHelperHandle Handle() const {Require(helper.has_value() && owner.world.Contains(*helper),"movement action helper is no longer owned");return *helper;}
        void Enter() override {
            Require(!entered && !released,"movement action cannot be entered twice");const auto h=Handle();
            const MonsterPathRestoreInput input{owner.actors.Source(owner.actor).behavior,owner.memory,owner.Transform(),owner.RadiusValue()};
            if(kind==3)(void)RestoreMonsterRandom(owner.world.Helper(h),input,owner.actors.Playback(owner.actor),owner.world);
            else (void)RestoreMonsterPath(kind,owner.world.Helper(h),input,owner.actors.Playback(owner.actor),owner.world);
            entered=true;++owner.stats.entered;
        }
        bool Active() const override {
            if(!helper)return false;
            if(!owner.world.Contains(*helper)){
                if(!owner.failure)owner.Error(std::make_exception_ptr(std::logic_error("owned movement helper disappeared before task completion")));
                return false;
            }
            return (owner.world.Helper(*helper).words[46]&0xff)!=0;
        }
        void SetActive(bool enabled) override {auto& image=owner.world.Helper(Handle());image.words[46]=(image.words[46]&~0xffu)|(enabled?1u:0u);}
        void Update(float seconds) override {
            Require(std::isfinite(seconds) && seconds>=0,"invalid movement behavior actor frame");
            (void)owner.orientation.Update(kind,Handle(),owner.RadiusValue());++owner.stats.updates;
        }
        bool Complete() const override {throw std::logic_error("movement behavior completion uses the original task duration");}
        void Release() noexcept override {
            if(released)return;
            released=true;
            try{if(helper){
                Require(owner.world.Contains(*helper),"owned movement helper disappeared before release");
                if(owner.world.Registered(*helper)){
                    if(kind==3)(void)SaveMonsterRandom(*helper,owner.memory,owner.actors.Playback(owner.actor),owner.world);
                    else (void)SaveMonsterPath(kind,*helper,owner.memory,owner.actors.Playback(owner.actor),owner.world);
                    helper.reset();++owner.stats.saved;
                }
            }}catch(...){owner.Error(std::current_exception());}
            Discard();
        }
    };
    bool Supports(std::int32_t kind) const override {return kind==0 || kind==1 || kind==3;}
    std::unique_ptr<IMonsterBehaviorAction> Create(std::int32_t kind) override {Check();Require(Supports(kind),"monster behavior kind is not implemented by movement factory");return std::make_unique<Action>(*this,kind);}
    template<class F>decltype(auto) Run(F operation){
        Require(!world.InGridFrame(),"actor behavior cannot run during the grid phase");
        Busy scope(busy);Check();try{if constexpr(std::is_void_v<decltype(operation())>){operation();Check();return;}else{auto result=operation();Check();return result;}}
        catch(...){if(!failure)Error(std::current_exception());throw;}
    }
};
MonsterMovementBehavior::MonsterMovementBehavior(MonsterMovementWorld& w,SceneMonsterInstances& a,SceneMonsterHandle actor,BinaryXmlNode& m,MonsterOrientation& o,Radius r,Identifier id)
    :impl_(std::make_unique<Impl>(w,a,actor,m,o,std::move(r),std::move(id))){}
MonsterMovementBehavior::MonsterMovementBehavior(MonsterMovementWorld& w,SceneMonsterInstances& a,SceneMonsterHandle actor,BinaryXmlNode& m,MonsterOrientation& o,Radius r,Identifier id,MonsterBehaviorManagerState& state,std::uint32_t token,ObjectMemory& storage,MonsterBehaviorPreparation preparation)
    :impl_(std::make_unique<Impl>(w,a,actor,m,o,std::move(r),std::move(id),&state,token,&storage,preparation)){}
void MonsterMovementBehavior::InitializeComponent(MonsterBaseMotion& motion,bool dispatch_birth_transform){
    auto& s=*impl_;s.Run([&]{
        Require(s.object_members && s.manager_image && !s.scheduler->Prepared() && !s.scheduler->Closed(),"monster component requires a deferred original member owner");
        motion.ValidateComponentOwner(s.actors,s.actor,*s.manager_image,s.manager_token);const auto& record=s.actors.Source(s.actor);
        BaseObjectState base{};std::copy_n(s.manager_image->words.begin(),base.words.size(),base.words.begin());
        (void)BaseObjectActivity::SetStateFlag(base,record.fields[0]);s.manager_image->words[0x1f0/4]=base.Read(0x1f0);
        MonsterObjectMembers::ComponentTransform dispatch;
        if(dispatch_birth_transform)dispatch=[&](std::uint32_t slot,const auto& value){if(slot==4){s.actors.SetMotionPosition(s.actor,value);}else if(slot==8){s.actors.SetMotionRotation(s.actor,value);}else{throw std::logic_error("invalid monster component transform slot");}};
        s.object_members->InitializeComponent(record,std::move(dispatch));
    });
}
std::uint32_t MonsterMovementBehavior::SourceToken() const{return impl_->object_members?impl_->object_members->SourceToken():0;}
std::uint32_t MonsterMovementBehavior::BehaviorChildToken() const{return impl_->object_members?impl_->object_members->BehaviorChildToken():0;}
MonsterMovementBehavior::~MonsterMovementBehavior()=default;
bool MonsterMovementBehavior::IsBusy() const noexcept{return impl_->busy;}
void MonsterMovementBehavior::Close(){
    auto& s=*impl_;if(s.closed)return;Require(!s.busy && !s.world.InGridFrame(),"movement behavior cleanup during an active frame");std::exception_ptr error;
    if(s.object_members){
        s.object_members->Close();s.queue=nullptr;s.scheduler=nullptr;s.manager_image=nullptr;s.manager_token=0;s.closed=true;
        if(s.failure){std::rethrow_exception(s.failure);}
        return;
    }
    try{Cancel();}catch(...){error=std::current_exception();}
    try{s.scheduler->Close();}catch(...){if(!error)error=std::current_exception();s.Error(std::current_exception());}
    if(s.queue->OwnsContainerImage(s.container_binding)){s.queue->UnbindContainerImage(s.container_binding);}
    s.manager_image=nullptr;s.manager_token=0;s.container_binding=0;
    s.closed=true;if(error)std::rethrow_exception(error);
}
bool MonsterMovementBehavior::Closed() const{return impl_->closed;}
std::uint32_t MonsterMovementBehavior::ProcessorMemoryToken() const{return impl_->closed?0:impl_->scheduler->MemoryToken();}
std::size_t MonsterMovementBehavior::ProcessorAllocationCount() const{return impl_->closed?0:impl_->scheduler->AllocationCount();}
BinaryXmlNode& MonsterMovementBehavior::ProcessorMemory(){Require(!impl_->closed,"movement behavior is closed");return impl_->scheduler->Memory();}
std::optional<std::uint32_t> MonsterMovementBehavior::FactoryAddress(std::int32_t kind) const{if(impl_->closed && !impl_->queue){return {};}return impl_->queue->FactoryAddress(kind);}
bool MonsterMovementBehavior::Start(){return Start(impl_->actors.Behavior(impl_->actor));}
bool MonsterMovementBehavior::Start(const MonsterBehaviorDefinition& definition){return impl_->Run([&]{Require(!impl_->scheduler->Enabled(),"single-round start while scheduling is enabled");return impl_->queue->Start(definition);});}
void MonsterMovementBehavior::AdvanceAfterGrid(float seconds){impl_->Run([&]{Require(!impl_->scheduler->Enabled(),"single-round advance while scheduling is enabled");(void)impl_->queue->Advance(seconds);});}
void MonsterMovementBehavior::SetSchedulingEnabled(bool enabled){impl_->Run([&]{impl_->scheduler->SetEnabled(enabled);});}
bool MonsterMovementBehavior::SchedulingEnabled() const{return !impl_->closed && impl_->scheduler->Enabled();}
std::uint32_t MonsterMovementBehavior::AdvanceScheduledAfterGrid(float seconds){return impl_->Run([&]{return impl_->scheduler->Advance(seconds);});}
std::uint32_t MonsterMovementBehavior::AdvanceManagerAfterGrid(MonsterBehaviorManagerState& state,std::uint32_t token,float seconds,std::uint32_t argument,IMonsterBehaviorFrameRuntime& services){
    return impl_->Run([&]{Require(std::isfinite(seconds) && seconds>=0,"invalid manager frame time");impl_->BindContainer(state,token);return impl_->scheduler->AdvanceManager(state,token,seconds,argument,services);});
}
IScriptEventProcessorRuntime& MonsterMovementBehavior::PursuitProcessor(MonsterBehaviorManagerState& state,std::uint32_t token){Require(impl_->busy,"pursuit outside movement frame");return impl_->scheduler->PursuitProcessor(state,token);}
void MonsterMovementBehavior::ClearPursuitTasks(MonsterBehaviorManagerState& state,std::uint32_t token){(void)PursuitProcessor(state,token);impl_->queue->Cancel();impl_->Check();}
void MonsterMovementBehavior::RestorePursuitBehavior(MonsterBehaviorManagerState& state,std::uint32_t token,std::uint32_t child){
    (void)PursuitProcessor(state,token);Require(child && state.words[0x6d0/4]==child && impl_->queue->Ready(),"invalid pursuit behavior reload");
    impl_->scheduler->RestoreFuzzy(state,token,bool(impl_->actors.Behavior(impl_->actor).fuzzy));
}
void MonsterMovementBehavior::ValidateActor(SceneMonsterInstances& actors,SceneMonsterHandle actor) const {Require(&actors==&impl_->actors && actor==impl_->actor,"pursuit behavior actor mismatch");impl_->Check();}
void MonsterMovementBehavior::Cancel(){
    if(impl_->closed)return;
    Require(!impl_->world.InGridFrame(),"actor behavior cannot cancel during the grid phase");
    Busy scope(impl_->busy);try{impl_->queue->Cancel();}catch(...){impl_->Error(std::current_exception());throw;}impl_->Check();
}
bool MonsterMovementBehavior::Ready() const{return impl_->closed || impl_->queue->Ready();}
bool MonsterMovementBehavior::Failed() const{return bool(impl_->failure);}
bool MonsterMovementBehavior::UsesWorld(const MonsterMovementWorld& world) const noexcept{return &impl_->world==&world;}
std::size_t MonsterMovementBehavior::TaskCount() const{return impl_->closed?0:impl_->queue->TaskCount();}
MonsterMovementBehaviorStats MonsterMovementBehavior::Stats() const{return impl_->stats;}
void MonsterMovementBehavior::WriteReport(std::ostream& out) const {
    const auto& s=*impl_;out<<"{\"actor_index\":"<<s.actor.index<<",\"failed\":"<<(Failed()?"true":"false")<<",\"constructed\":"<<s.stats.constructed<<",\"entered\":"<<s.stats.entered<<",\"saved\":"<<s.stats.saved<<",\"discarded\":"<<s.stats.discarded<<",\"updates\":"<<s.stats.updates<<",\"live_actions\":"<<s.stats.live_actions<<",\"failures\":"<<s.stats.failures<<",\"queue\":";
    if(s.queue){s.queue->WriteReport(out);}else{out<<"null";}
    out<<",\"scheduler\":";if(s.scheduler){s.scheduler->WriteReport(out);}else{out<<"null";}
    out<<",\"error\":";
    if(!s.failure)out<<"null";else{try{std::rethrow_exception(s.failure);}catch(const std::exception& e){out<<std::quoted(e.what());}catch(...){out<<"\"non-standard exception\"";}}
    out<<'}';Require(bool(out),"cannot write movement behavior report");
}
}
