#include "pal4/portable/monster_scene_actor.h"
#include "pal4/portable/actor_resource_initialization.h"
#include "pal4/base_object_activity.h"
#include "pal4/game_resource_manager.h"
#include <algorithm>
#include <bit>
#include <exception>
#include <stdexcept>
#include <utility>

namespace pal4::portable {
struct MonsterSceneActor::Impl {
    Impl(MonsterMovementWorld& value,SceneMonsterInstances& instances,SceneMonsterHandle h,std::uint32_t token)
        :world(value),actors(&instances),actor(h),manager(token){}
    MonsterMovementWorld& world;
    SceneMonsterInstances* actors;
    SceneMonsterHandle actor;std::uint32_t manager;
    ObjectMemory storage;
    MonsterBehaviorManagerState state{};
    BinaryXmlNode memory;
    std::uint32_t label_token{};std::size_t label_size{};
    std::unique_ptr<MonsterMovementBehavior> behavior;
    std::unique_ptr<ActorResourceFrame> resource;
    std::unique_ptr<MonsterBaseMotion> motion;
    std::unique_ptr<MonsterPursuitFrame> frame;
    std::unique_ptr<MonsterEncounterStorage> encounter;
    const bool* scene_close_allowed{};
    bool closed{},rendering{};
    ~Impl(){try{Close();}catch(...) {}}
    void Close(){
        if(closed)return;
        if(rendering || (scene_close_allowed && !*scene_close_allowed) || world.InGridFrame() || (frame && frame->Busy()) || (motion && motion->Busy()) || (behavior && behavior->IsBusy()))
            throw std::logic_error("monster actor close during active frame");
        std::exception_ptr error;
        frame.reset();
        if(motion){
            try{if(!motion->Close())throw std::logic_error("monster animation binding was replaced");}catch(...){error=std::current_exception();}
            motion.reset();
        }
        resource.reset();
        if(encounter){try{encounter->Close();}catch(...){if(!error)error=std::current_exception();}encounter.reset();}
        if(behavior){try{behavior->Close();}catch(...){if(!error)error=std::current_exception();}behavior.reset();}
        closed=true;
        if(storage.AllocationCount() && !error)error=std::make_exception_ptr(std::logic_error("monster object allocations remain"));
        if(error)std::rethrow_exception(error);
    }
    void RequireOpen() const {if(closed || !frame)throw std::logic_error("monster scene actor is closed");}
};
MonsterSceneActor::MonsterSceneActor(MonsterMovementWorld& world,SceneMonsterInstances& actors,SceneMonsterHandle actor,
    MonsterOrientation& orientation,MonsterMovementBehavior::Radius radius,MonsterMovementBehavior::Identifier identifiers,
    PackageReader& package,std::uint32_t manager,std::uint32_t clump,std::uint32_t mode,std::uint32_t extra,
    CollisionSurface& floor,const CollisionMesh& mesh,float height,MovementCollisionQuery collision,IMonsterPursuitSceneServices& services)
    :impl_(std::make_unique<Impl>(world,actors,actor,manager)) {
    auto& s=*impl_;
    s.behavior=std::make_unique<MonsterMovementBehavior>(world,actors,actor,s.memory,orientation,std::move(radius),std::move(identifiers),s.state,manager,s.storage,MonsterBehaviorPreparation::Component);
    BindConstructedActorResource(actors,actor,s.state,manager,clump,mode,extra,package);
    const auto& name=actors.Source(actor).name;
    if(s.state.words[1])throw std::logic_error("monster label already bound");
    s.label_size=name.size();s.label_token=s.storage.Allocate(static_cast<std::uint32_t>(name.size()+1));s.state.words[1]=s.label_token;
    for(std::size_t i=0;i<name.size();++i)s.storage.WriteByte(s.label_token+static_cast<std::uint32_t>(i),static_cast<std::uint8_t>(name[i]));
    s.resource=std::make_unique<ActorResourceFrame>(actors,actor,s.state,package);
    s.motion=std::make_unique<MonsterBaseMotion>(actors,actor,s.state,manager,floor,height,std::move(collision),[&s](auto& state,float dt){(void)s.resource->Update(state,dt);},&s.storage);
    s.behavior->InitializeComponent(*s.motion);
    s.encounter=std::make_unique<MonsterEncounterStorage>(s.state,s.storage);
    s.frame=std::make_unique<MonsterPursuitFrame>(actors,actor,s.state,manager,s.behavior->SourceToken(),s.behavior->BehaviorChildToken(),*s.behavior,*s.motion,floor,mesh,services);
}
MonsterSceneActor::~MonsterSceneActor()=default;
MonsterBehaviorManagerState& MonsterSceneActor::State(){impl_->RequireOpen();return impl_->state;}
MonsterMovementBehavior& MonsterSceneActor::Behavior(){impl_->RequireOpen();return *impl_->behavior;}
MonsterBaseMotion& MonsterSceneActor::Motion(){impl_->RequireOpen();return *impl_->motion;}
MonsterPursuitFrame& MonsterSceneActor::Frame(){impl_->RequireOpen();return *impl_->frame;}
MonsterEncounterStorage& MonsterSceneActor::Encounter(){impl_->RequireOpen();return *impl_->encounter;}
std::uint32_t MonsterSceneActor::ManagerToken() const noexcept{return impl_->manager;}
SceneMonsterHandle MonsterSceneActor::Handle() const noexcept{return impl_->actor;}
bool MonsterSceneActor::UsesInstances(const SceneMonsterInstances& actors) const noexcept{return impl_->actors==&actors;}
std::string MonsterSceneActor::Label() const{
    impl_->RequireOpen();
    if(!impl_->label_token || impl_->state.words[1]!=impl_->label_token)throw std::logic_error("monster label ownership changed");
    std::string value(impl_->label_size,'\0');
    for(std::size_t i=0;i<value.size();++i)value[i]=static_cast<char>(impl_->storage.ReadByte(impl_->label_token+static_cast<std::uint32_t>(i)));
    return value;
}
MonsterSceneRenderResult MonsterSceneActor::Render(std::uint32_t camera,IMonsterSceneRenderServices& services){
    auto& s=*impl_;s.RequireOpen();
    if(!camera || s.rendering || s.world.InGridFrame() || s.frame->Busy() || s.motion->Busy() || s.behavior->IsBusy())
        throw std::logic_error("monster scene render is not ready");
    s.rendering=true;struct Guard{bool& busy;~Guard(){busy=false;}} guard{s.rendering};
    struct Runtime final:IBaseObjectActivityRuntime,GameResourceManagerRuntimeStub {
        Impl& owner;IMonsterSceneRenderServices& services;BaseObjectState base;MonsterSceneRenderResult result;
        Runtime(Impl& o,IMonsterSceneRenderServices& v):owner(o),services(v){Reload();}
        void Reload(){std::copy_n(owner.state.words.begin(),base.words.size(),base.words.begin());}
        void Publish(){std::copy(base.words.begin(),base.words.end(),owner.state.words.begin());}
        bool FrustumTestSphere(std::uint32_t camera,std::uint32_t* sphere) override{
            std::array<float,4> value{};for(unsigned i=0;i<4;++i)value[i]=std::bit_cast<float>(sphere[i]);
            return services.FrustumTestSphere(owner.actor,camera,value);
        }
        void RenderClumpRaw(std::uint32_t clump) override{
            if(clump!=owner.state.words[5])throw std::logic_error("monster render body identity changed");
            services.DrawClump(owner.actor,clump);
        }
        bool RenderClump(BaseObjectState& state,std::uint32_t camera) override{
            if(&state!=&base)throw std::logic_error("monster render image mismatch");
            ++result.active_objects;
            GameResourceManager resource;std::copy_n(base.words.begin(),resource.words.size(),resource.words.begin());
            const bool passed=resource.RenderClump(camera,*this);base.words[7]=resource.words[7];
            result.render_successes+=passed;result.visible_objects+=passed && resource.words[6]!=0;
            return passed;
        }
        void ProcessAnimation(BaseObjectState& state,std::uint32_t offset,std::uint32_t camera) override{
            if(&state!=&base || offset!=0x170)throw std::logic_error("monster render animation slot mismatch");
            Publish();try{result.animation_calls+=owner.actors->Playback(owner.actor).ProcessRenderAnimation(camera);}
            catch(...){Reload();throw;}Reload();
        }
        std::uint32_t UpdateCharacter(BaseObjectState& state,std::uint32_t offset,std::uint32_t object) override{
            if(&state!=&base || offset!=0x564 || object!=owner.manager)throw std::logic_error("monster render collision slot mismatch");
            Publish();CollisionActorState collision;std::copy_n(base.words.begin()+0x564/4,collision.words.size(),collision.words.begin());
            ++result.character_updates;
            try{const auto value=CollisionActor::DispatchCallbacks(collision,object+offset,object,services);Reload();return value;}
            catch(...){Reload();throw;}
        }
    } runtime(s,services);
    try{(void)BaseObjectActivity::UpdateState(runtime.base,s.manager,camera,runtime);runtime.Publish();return runtime.result;}
    catch(...){runtime.Publish();throw;}
}
void MonsterSceneActor::Close(){impl_->Close();}
bool MonsterSceneActor::CanAttachScene() const noexcept{return !impl_->closed && impl_->frame && !impl_->scene_close_allowed;}
void MonsterSceneActor::AttachScene(const bool* allow_close){
    impl_->RequireOpen();
    if(!allow_close || impl_->scene_close_allowed)throw std::logic_error("monster actor already belongs to a scene");
    impl_->scene_close_allowed=allow_close;
}
}
