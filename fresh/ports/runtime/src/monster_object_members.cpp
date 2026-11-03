#include "pal4/portable/monster_object_members.h"
#include "pal4/portable/borrowed_actor_base_members.h"
#include "pal4/monster_behavior_construction.h"
#include "pal4/monster_behavior_component.h"
#include "monster_behavior_scheduler.h"
#include "monster_component_source.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace pal4::portable {
namespace {
void Require(bool value,const char* message){if(!value)throw std::logic_error(message);}
bool SameDefinition(const MonsterBehaviorDefinition& a,const MonsterBehaviorDefinition& b){
    if(a.fuzzy.has_value()!=b.fuzzy.has_value())return false;
    if(!a.fuzzy)return true;
    if(a.fuzzy->mode!=b.fuzzy->mode || a.fuzzy->choices.size()!=b.fuzzy->choices.size())return false;
    for(std::size_t i=0;i<a.fuzzy->choices.size();++i){const auto& x=a.fuzzy->choices[i];const auto& y=b.fuzzy->choices[i];if(x.class_id!=y.class_id || x.proportion!=y.proportion || x.wait_mode!=y.wait_mode || x.wait_time!=y.wait_time || x.expect_time!=y.expect_time)return false;}
    return true;
}
}
struct MonsterObjectMembers::Impl final:IMonsterBehaviorConstructionRuntime {
    MonsterBehaviorManagerState& state;std::uint32_t token;ObjectMemory& storage;IMonsterBehaviorActionFactory& factory;
    MonsterBehaviorDefinition definition;BinaryXmlNode& memory;std::function<std::uint32_t()> random;
    BaseObjectState base{};BorrowedActorBaseMembers base_members;
    std::unique_ptr<detail::MonsterComponentSource> component;
    std::unique_ptr<detail::MonsterBehaviorScheduler> scheduler;std::unique_ptr<MonsterBehaviorTasks> queue;
    std::uint64_t container_binding{};bool registry_alive{},alive{},closed{},component_loading{},component_loaded{},component_failed{};
    Impl(MonsterBehaviorManagerState& image,std::uint32_t id,ObjectMemory& heap,IMonsterBehaviorActionFactory& actions,
         const MonsterBehaviorDefinition& config,BinaryXmlNode& remembered,std::function<std::uint32_t()> rng)
        :state(image),token(id),storage(heap),factory(actions),definition(config),memory(remembered),random(std::move(rng)),
         base_members(storage,[this](auto& value){Require(&value==&base,"foreign monster base image");return token;}){
        Require(token && token%4==0 && token<=std::numeric_limits<std::uint32_t>::max()-sizeof(state) && bool(random),"invalid monster construction identity or random service");
        Require(std::all_of(state.words.begin(),state.words.end(),[](auto value){return value==0;}),"monster construction requires an empty image");
        const auto before=state.words;const auto previous_name=memory.name;
        try{(void)MonsterBehaviorManager::ConstructMembers(state,token,*this);alive=true;}
        catch(...){state.words=before;memory.name=previous_name;throw;}
    }
    ~Impl() override {if(alive && !closed){try{Close();}catch(...){}}}
    void Match(MonsterBehaviorManagerState& image,std::uint32_t address,std::uint32_t offset){Require(&image==&state && address==token+offset,"foreign monster construction component");}
    void LoadBase(){std::copy_n(state.words.begin(),base.words.size(),base.words.begin());}
    void StoreBase(){std::copy(base.words.begin(),base.words.end(),state.words.begin());}
    void ConstructBase(MonsterBehaviorManagerState& image,std::uint32_t a) override {Match(image,a,0);(void)base_members.Construct(base,token);StoreBase();}
    void DestructBase(MonsterBehaviorManagerState& image,std::uint32_t a) override {Match(image,a,0);LoadBase();base.words[5]=0;base_members.Destroy(base);StoreBase();}
    void ConstructProcessor(MonsterBehaviorManagerState& image,std::uint32_t a) override {
        Match(image,a,MonsterBehaviorManager::kProcessorOffset);
        scheduler=std::make_unique<detail::MonsterBehaviorScheduler>(definition,detail::MonsterSchedulerBindings{
            [this]{Require(bool(queue),"monster queue is not constructed");return queue->Ready();},
            [this]{Require(bool(queue),"monster queue is not constructed");return queue->Start(definition);},
            [this](float elapsed){Require(bool(queue),"monster queue is not constructed");return queue->Advance(elapsed);},
            [this]{return random();}},memory,&storage,false);
        scheduler->CopyProcessorTo(state);
    }
    void DestructProcessor(MonsterBehaviorManagerState& image,std::uint32_t a) override {Match(image,a,MonsterBehaviorManager::kProcessorOffset);if(scheduler){scheduler->Close();scheduler->CopyProcessorTo(state);scheduler.reset();}}
    void ConstructScript(MonsterBehaviorManagerState& image,std::uint32_t a) override {
        Match(image,a,MonsterBehaviorManager::kScriptOffset);
        queue=std::unique_ptr<MonsterBehaviorTasks>(new MonsterBehaviorTasks(factory,[this]{return random();},false));
        container_binding=queue->BindContainerImage(std::span<std::uint32_t,4>(state.words.data()+MonsterBehaviorManager::kScriptOffset/4,4),a);
    }
    void DestructScript(MonsterBehaviorManagerState& image,std::uint32_t a) override {
        Match(image,a,MonsterBehaviorManager::kScriptOffset);
        if(queue){if(queue->OwnsContainerImage(container_binding))queue->UnbindContainerImage(container_binding);queue.reset();container_binding=0;}
        const auto offset=MonsterBehaviorManager::kScriptOffset/4;state.words[offset+1]=state.words[offset+2]=0;state.words[offset+3]=(state.words[offset+3]&0xffffff00u)|1;
    }
    MonsterBehaviorTreeBytes TreeInputBytes() override {return {};}
    void InitializeTree(MonsterBehaviorManagerState& image,std::uint32_t a) override {Match(image,a,MonsterBehaviorManager::kTreeOffset);Require(queue && !registry_alive && queue->RegisteredFactoryCount()==0,"monster registry is already constructed");registry_alive=true;state.words[0x6a0/4]=state.words[0x6a8/4]=0;}
    void CleanupTree(MonsterBehaviorManagerState& image,std::uint32_t a) override {Match(image,a,0x698);if(registry_alive){queue->ClearFactories();registry_alive=false;}state.words[0x6a0/4]=state.words[0x6a8/4]=0;}
    void InsertBehavior(std::uint32_t a,std::uint32_t id,std::uint32_t function) override {Require(a==token+MonsterBehaviorManager::kTreeOffset && registry_alive && queue,"unbound monster registry");queue->RegisterFactory(id,function);state.words[0x6a8/4]=static_cast<std::uint32_t>(queue->RegisteredFactoryCount());}
    void DestroyAuxiliary(MonsterBehaviorManagerState& image,std::uint32_t a) override {Match(image,a,MonsterBehaviorManager::kAuxiliaryOffset);Require(state.words[MonsterBehaviorManager::kAuxiliaryOffset/4+1]==0,"monster callback still owns data");state.words[MonsterBehaviorManager::kAuxiliaryOffset/4]=0x84932c;}
    void CleanupWordArray(MonsterBehaviorManagerState& image,std::uint32_t a) override {Match(image,a,0x710);for(unsigned offset:{0x714u,0x718u,0x71cu})Require(!state.words[offset/4],"monster combat positions are not returned");}
    void CleanupHistory(MonsterBehaviorManagerState& image,std::uint32_t a) override {Match(image,a,0x720);for(unsigned offset:{0x724u,0x728u,0x72cu})Require(!state.words[offset/4],"monster history records are not returned");}
    void Close(){
        if(closed){return;}
        Require(alive && !component_loading,"monster members are not constructed or loading a component");
        LoadBase();base_members.ValidateDestroy(base);CleanupHistory(state,token+0x720);CleanupWordArray(state,token+0x710);
        Require(!state.words[MonsterBehaviorManager::kAuxiliaryOffset/4+1],"monster callback still owns data");
        queue->Cancel();DestroyAuxiliary(state,token+MonsterBehaviorManager::kAuxiliaryOffset);CleanupTree(state,token+0x698);
        DestructScript(state,token+MonsterBehaviorManager::kScriptOffset);DestructProcessor(state,token+MonsterBehaviorManager::kProcessorOffset);DestructBase(state,token);
        if(component){component->Close();component.reset();state.words[0x654/4]=state.words[0x6d0/4]=0;}component_loaded=false;closed=true;
    }
    void InitializeComponent(const SceneMonster& record,ComponentTransform dispatch){
        Require(alive && !closed && !component_loading && !component_loaded && !component_failed && !component,"monster component is already initialized or failed");
        Require(!scheduler->Prepared() && !scheduler->Closed() && queue->Ready() && queue->TaskCount()==0 && !state.words[0x654/4] && !state.words[0x6d0/4],"monster component requires a fresh processor and source");
        Require(SameDefinition(definition,ReadMonsterBehaviorDefinition(record.behavior)),"monster component behavior differs from its actual factory definition");
        CleanupWordArray(state,token+0x710);CleanupHistory(state,token+0x720);
        component_loading=true;struct Guard{bool& flag;~Guard(){flag=false;}} guard{component_loading};
        try{
            component=std::make_unique<detail::MonsterComponentSource>(storage,record);
            struct Runtime final:IMonsterBehaviorComponentRuntime {
                Impl& owner;ComponentTransform& dispatch;
                Runtime(Impl& value,ComponentTransform& transform):owner(value),dispatch(transform){}
                std::uint32_t SourceDocument(std::uint32_t source) override {return owner.component->Document(source);}
                void ClearEventsAndTasks(MonsterBehaviorManagerState&,std::uint32_t) override {throw std::logic_error("monster component replacement is not bound");}
                std::uint32_t XmlChildByIndex(std::uint32_t document,std::uint32_t index) override {return owner.component->FirstChild(document,index);}
                void LoadFuzzyBehaviorConfig(std::uint32_t manager,std::uint32_t child) override {
                    Require(manager==owner.token && child==owner.component->ChildToken(),"foreign monster behavior component");owner.scheduler->PrepareComponent(owner.state);
                }
                void DispatchComponentField(std::uint32_t manager,std::uint32_t slot,std::uint32_t field) override {
                    Require(manager==owner.token && bool(dispatch),"monster component transform is not bound");const auto source=owner.component->Token();const auto& record=owner.component->Record();
                    if(slot==4 && field==source+0x18)dispatch(slot,record.position);
                    else if(slot==8 && field==source+0x24)dispatch(slot,record.rotation);
                    else throw std::logic_error("foreign monster component transform field");
                }
                void ClearHistoryRecord(std::uint32_t) override {throw std::logic_error("monster component history is not empty");}
            } runtime(*this,dispatch);
            (void)MonsterBehaviorManager::SetComponentMembers(state,token,component->Token(),bool(dispatch),false,runtime);component_loaded=true;
        }catch(...){component_failed=true;scheduler->CopyProcessorTo(state);throw;}
    }
};
MonsterObjectMembers::MonsterObjectMembers(MonsterBehaviorManagerState& s,std::uint32_t token,ObjectMemory& memory,IMonsterBehaviorActionFactory& factory,const MonsterBehaviorDefinition& definition,BinaryXmlNode& remembered,std::function<std::uint32_t()> random)
    :impl_(std::make_unique<Impl>(s,token,memory,factory,definition,remembered,std::move(random))){}
MonsterObjectMembers::~MonsterObjectMembers()=default;
void MonsterObjectMembers::Close(){impl_->Close();}
bool MonsterObjectMembers::Closed() const{return impl_->closed;}
void MonsterObjectMembers::PrepareBehavior(){Require(!impl_->closed && !impl_->component_loading && !impl_->component_loaded,"closed or initialized monster members");impl_->scheduler->PrepareBehavior();impl_->scheduler->CopyProcessorTo(impl_->state);}
void MonsterObjectMembers::InitializeComponent(const SceneMonster& record,ComponentTransform dispatch){impl_->InitializeComponent(record,std::move(dispatch));}
std::uint32_t MonsterObjectMembers::SourceToken() const{return impl_->component_loaded && !impl_->closed?impl_->component->Token():0;}
std::uint32_t MonsterObjectMembers::BehaviorChildToken() const{return impl_->component_loaded && !impl_->closed?impl_->component->ChildToken():0;}
MonsterBehaviorTasks& MonsterObjectMembers::Tasks(){Require(!impl_->closed && !impl_->component_loading,"closed or loading monster members");return *impl_->queue;}
detail::MonsterBehaviorScheduler& MonsterObjectMembers::Scheduler(){Require(!impl_->closed && !impl_->component_loading,"closed or loading monster members");return *impl_->scheduler;}
void MonsterObjectMembers::ValidateManager(const MonsterBehaviorManagerState& state,std::uint32_t token) const{
    Require(!impl_->closed && !impl_->component_loading && !impl_->component_failed && &state==&impl_->state && token==impl_->token && impl_->queue->OwnsContainerImage(impl_->container_binding),"monster members belong to another manager image or identity");
    if(impl_->component_loaded){Require(state.words[0x654/4]==impl_->component->Token() && state.words[0x6d0/4]==impl_->component->ChildToken(),"monster component binding was modified");}
}
}
