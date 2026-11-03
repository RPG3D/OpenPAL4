#include "pal4/portable/monster_behavior_tasks.h"
#include "pal4/behavior_atomic_dispatcher.h"
#include "pal4/gom_task_base.h"
#include "pal4/gom_task_container.h"
#include "pal4/script_object.h"
#include "pal4/monster_behavior_factory_registration.h"
#include "pal4/pal_behavior_tasks.h"
#include "pal4/pal_behavior_class_one.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <exception>
#include <map>
#include <limits>
#include <ostream>
#include <span>
#include <stdexcept>
#include <type_traits>

namespace pal4::portable {
namespace {
using namespace pal4;
void Require(bool value,const char* message){if(!value)throw std::logic_error(message);}
struct Busy {bool& value;explicit Busy(bool& v):value(v){Require(!v,"behavior queue is not reentrant");v=true;}~Busy(){value=false;}};
struct Image {GomTaskBaseState base;std::array<std::uint32_t,2> tail{};};
static_assert(sizeof(Image)==0x44 && std::is_trivially_copyable_v<Image>);
template<class To,class From>void Copy(To& to,const From& from,std::size_t size){
    static_assert(std::is_trivially_copyable_v<To> && std::is_trivially_copyable_v<From>);
    Require(size<=sizeof(To) && size<=sizeof(From),"behavior task copy bounds");
    auto destination=std::as_writable_bytes(std::span(&to,1));const auto source=std::as_bytes(std::span(&from,1));
    std::copy_n(source.begin(),size,destination.begin());
}
// 统一两种原接口的适配面，保持全部虚函数显式override。
struct OtherRuntime:IPalBehaviorTaskRuntime {virtual unsigned InitializeVectorObject(unsigned)=0;};
struct OneRuntime:IPalBehaviorClassOneRuntime,IPalBehaviorTaskExecutionRuntime {
    virtual void WriteByte(unsigned,std::uint8_t)=0;
    virtual unsigned ConstructVectorObject(unsigned)=0;
    virtual unsigned ConstructExtendedObject(unsigned)=0;
    virtual unsigned ConstructSmallObject(unsigned)=0;
    virtual unsigned ConstructRandomObject(unsigned)=0;
    virtual unsigned IsOwnedComplete(unsigned,unsigned)=0;
    virtual bool IsDurationComplete(PalBehaviorTaskState&)=0;
};
template<int K>using State=std::conditional_t<K==1,PalBehaviorClassOneState,PalBehaviorTaskState>;
template<int K>using Shared=std::conditional_t<K==1,PalBehaviorClassOne,
    PalBehaviorClass<K==0?PalBehaviorKind::ClassZero:K==2?PalBehaviorKind::ClassTwo:PalBehaviorKind::ClassThree>>;
}
struct MonsterBehaviorTasks::Impl final:IGomTaskBaseRuntime,IGomTaskContainerRuntime,IBehaviorAtomicDispatchRuntime,IScriptObjectRuntime,IScriptEventNodeRuntime,IMonsterBehaviorFactoryRegistrationRuntime {
    static constexpr unsigned task_base=0x100000,helper_base=0x20000000,memory_base=0x40000000,record_base=0x10000;
    static constexpr unsigned registry_token=0x9000,source_token=0x9004,timing_token=0x9008;
    std::map<std::int32_t,std::uint32_t> registry;
    struct DispatchScope {int kind;const char* name;BehaviorAtomicWaitOptions options;unsigned pending{};};
    DispatchScope* dispatch_scope{};
    struct Entry {Image image;int kind{};bool live{};};
    struct Helper {unsigned task{};std::unique_ptr<IMonsterBehaviorAction> action;};
    IMonsterBehaviorActionFactory& factory;std::function<unsigned()> random;
    std::vector<Entry> entries;std::map<unsigned,Helper> helpers;
    std::map<unsigned,std::vector<std::byte>> memory;
    unsigned next_memory=memory_base,next_helper=helper_base;
    MonsterBehaviorGroup group;float delta{};
    unsigned started{},completed{},deleted{},updates{};bool processed{},failed{},busy{};
    std::uint32_t container_address{2};std::span<std::uint32_t> bound_image;std::array<std::uint32_t,4> published{};
    std::uint64_t binding{},next_binding{1};
    bool ImageIntact() const{return bound_image.empty() || std::equal(bound_image.begin(),bound_image.end(),published.begin());}
    void ValidateImage() const{Require(ImageIntact(),"owned task container image was modified externally");}
    void PublishImage() noexcept {
        if(bound_image.empty())return;
        bound_image[0]=(bound_image[0]&0xffffff00u)|queue.opaque_byte;bound_image[1]=queue.sentinel;bound_image[2]=queue.count;
        bound_image[3]=(bound_image[3]&0xffffff00u)|queue.ready;std::copy(bound_image.begin(),bound_image.end(),published.begin());
    }
    void Relocate(std::uint32_t address,bool validate=true){
        if(validate){for(const auto& entry:entries)if(entry.live)Require(entry.image.base.context==container_address,"task context does not match its actual container");}
        for(auto& entry:entries)if(entry.live)entry.image.base.context=address;
        container_address=address;
    }
    template<class F>decltype(auto) Mutate(F operation){
        ValidateImage();Busy scope(busy);
        try{if constexpr(std::is_void_v<decltype(operation())>){operation();ValidateImage();PublishImage();return;}
            else{auto result=operation();ValidateImage();PublishImage();return result;}}
        catch(...){if(ImageIntact())PublishImage();throw;}
    }
    GomTaskBase base{*this};GomTaskContainer container{*this,base};GomTaskContainerState queue{};
    Impl(IMonsterBehaviorActionFactory& f,std::function<unsigned()> rng,bool register_factories):factory(f),random(std::move(rng)){
        Require(bool(random),"behavior RNG is not bound");(void)ScriptObject::Initialize(container_address,*this);
        try{if(register_factories)RegisterMonsterBehaviorFactories(registry_token,*this);}catch(...){(void)ScriptObject::Destroy(container_address,*this);throw;}
    }
    ~Impl() override {bound_image={};if(queue.sentinel)(void)ScriptObject::Destroy(container_address,*this);}
    void InitializeTaskContainer(std::uint32_t address) override {Require(address==container_address && !queue.sentinel,"invalid script task container initialization");(void)container.Initialize(queue,0);}
    std::uint32_t DestroyTaskContainer(std::uint32_t address) override {Require(address==container_address,"invalid script task container destruction");return container.Destroy(queue);}
    Entry& Task(unsigned id){Require(id>=task_base && (id-task_base)%0x100==0 && (id-task_base)/0x100<entries.size(),"invalid behavior task handle");return entries[(id-task_base)/0x100];}
    static unsigned Owner(int kind){return 0x8000+static_cast<unsigned>(kind)*0x100;}
    Helper& GetHelper(unsigned id){auto it=helpers.find(id);Require(it!=helpers.end(),"invalid behavior helper");return it->second;}
    unsigned AllocateHelper(unsigned task,unsigned bytes){
        const auto kind=Task(task).kind;Require(bytes==(kind==2?0xbcu:0xe0u) && next_helper<0x30000000,"behavior helper allocation bounds");
        const auto id=next_helper;next_helper+=0x100;helpers.emplace(id,Helper{task,{}});return id;
    }
    void ConstructHelper(unsigned task,unsigned id){
        auto& h=GetHelper(id);Require(h.task==task && !h.action,"behavior helper already constructed");
        h.action=factory.Create(Task(task).kind);Require(h.action!=nullptr,"behavior helper factory returned null");
    }
    void Accept(unsigned task,unsigned owner,unsigned id){
        auto& h=GetHelper(id);Require(h.task==task && owner==Owner(Task(task).kind) && h.action,"invalid behavior acceptance");
        try{h.action->Enter();}catch(...){helpers.erase(id);throw;}
    }
    void Release(unsigned owner,unsigned id) noexcept {
        const auto it=helpers.find(id);if(it==helpers.end())std::terminate();
        if(owner!=Owner(Task(it->second.task).kind) || !it->second.action)std::terminate();
        it->second.action->Release();helpers.erase(it);
    }
    template<int K>struct Runtime final:std::conditional_t<K==1,OneRuntime,OtherRuntime> {
        static constexpr int kind=K;
        Impl& owner;unsigned task;
        Runtime(Impl& o,unsigned t):owner(o),task(t){}
        void ConstructBase(State<K>& s) override {GomTaskBaseState b;(void)owner.base.Construct(b);Copy(s,b,sizeof b);}
        void DestructBase(State<K>& s) override {GomTaskBaseState b;Copy(b,s,sizeof b);(void)owner.base.Destroy(b);Copy(s,b,sizeof b);}
        std::uint8_t ReadByte(unsigned a) override {
            Require(a>=helper_base+0xb8 && (a-helper_base)%0x100==0xb8,"invalid behavior helper byte");
            auto& h=owner.GetHelper(a-0xb8);Require(h.action!=nullptr,"unconstructed behavior helper");return h.action->Active()?1:0;
        }
        void WriteByte(unsigned a,std::uint8_t v) override {Require(a>=helper_base+0xb8 && (a-helper_base)%0x100==0xb8 && v<=1,"invalid behavior activation byte");owner.GetHelper(a-0xb8).action->SetActive(v!=0);}
        void ReleaseOwned(unsigned target,unsigned helper) override {owner.Release(target,helper);}
        void UpdateOwned(unsigned target,unsigned helper) override {Require(target==Owner(owner.Task(task).kind),"invalid behavior update owner");owner.GetHelper(helper).action->Update(owner.delta);}
        void Free(unsigned id) override {Require(owner.helpers.erase(id)==1,"invalid behavior helper free");}
        unsigned Bind(unsigned binder,unsigned& k) override {Require(binder==1 && k==K,"invalid behavior binder");return Owner(K)+4;}
        unsigned Allocate(unsigned bytes) override {return owner.AllocateHelper(task,bytes);}
        unsigned ConstructVectorObject(unsigned id) override {Require(K==0 || K==1,"invalid vector behavior helper class");owner.ConstructHelper(task,id);return id;}
        unsigned InitializeVectorObject(unsigned id) override {return ConstructVectorObject(id);}
        unsigned ConstructExtendedObject(unsigned) override {throw std::logic_error("NPC class four helper is not a monster behavior");}
        unsigned ConstructSmallObject(unsigned id) override {Require(K==2,"invalid small behavior helper class");owner.ConstructHelper(task,id);return id;}
        unsigned ConstructRandomObject(unsigned id) override {Require(K==3,"invalid random behavior helper class");owner.ConstructHelper(task,id);return id;}
        void OnCreated(unsigned target,unsigned helper) override {owner.Accept(task,target,helper);}
        unsigned IsOwnedComplete(unsigned target,unsigned helper) override {Require(target==Owner(K),"invalid behavior completion owner");return owner.GetHelper(helper).action->Complete()?1:0;}
        State<K>& Access(unsigned) override {throw std::logic_error("behavior factory storage is owned by modern container");}
        bool IsDurationComplete(PalBehaviorTaskState& s) override {GomTaskBaseState b;Copy(b,s,sizeof b);return GomTaskBase::IsDurationComplete(b);}
    };
    template<int K,class F>void With(unsigned id,F operation){
        auto& e=Task(id);State<K> s;Copy(s,e.image,sizeof s);Runtime<K> runtime(*this,id);
        try{operation(s,runtime);}catch(...){Copy(e.image,s,sizeof s);throw;}Copy(e.image,s,sizeof s);
    }
    template<class F>void Visit(unsigned id,F operation){
        switch(Task(id).kind){case 0:With<0>(id,operation);break;case 1:With<1>(id,operation);break;case 2:With<2>(id,operation);break;case 3:With<3>(id,operation);break;default:throw std::logic_error("unsupported behavior task class");}
    }
    std::span<std::byte> Memory(unsigned a,std::size_t size){
        if(a>=task_base && a<helper_base){const auto id=task_base+((a-task_base)/0x100)*0x100;auto& e=Task(id);auto b=std::as_writable_bytes(std::span(&e.image,1));Require(a-id<=b.size() && size<=b.size()-(a-id),"behavior task field bounds");return b.subspan(a-id,size);}
        auto it=memory.upper_bound(a);Require(it!=memory.begin(),"unknown behavior queue memory");--it;const auto offset=a-it->first;Require(offset<=it->second.size() && size<=it->second.size()-offset,"behavior queue memory bounds");return std::span(it->second).subspan(offset,size);
    }
    unsigned AllocateMemory(unsigned bytes) override {Require(bytes>0 && bytes<0x1000 && next_memory<0xf0000000,"behavior queue allocation bounds");const auto id=next_memory;next_memory+=0x1000;memory.emplace(id,std::vector<std::byte>(bytes));return id;}
    unsigned FreeMemory(unsigned id) override {if(id)Require(memory.erase(id)==1,"invalid behavior queue free");return 0;}
    unsigned ReadWord(unsigned a) override {unsigned v;auto b=Memory(a,4);std::memcpy(&v,b.data(),4);return v;}
    void WriteWord(unsigned a,unsigned v) override {auto b=Memory(a,4);std::memcpy(b.data(),&v,4);}
    void CopyCString(unsigned a,const char* text,unsigned n) override {auto b=Memory(a,n);std::memcpy(b.data(),text,n);}
    GomTaskBaseState& TaskAt(unsigned id) override {return Task(id).image.base;}
    bool IsWaitComplete(unsigned id) override {return base.IsWaitComplete(TaskAt(id));}
    void BeginRun(unsigned id) override {
        try{Visit(id,[&](auto& s,auto& r){using C=Shared<std::remove_reference_t<decltype(r)>::kind>;(void)C::CreateOwned(s,r);});++started;}
        catch(...){Task(id).image.tail[1]=0;throw;}
    }
    void Tick(unsigned id,float value) override {delta=value;PalBehaviorTaskState s;Copy(s,Task(id).image,sizeof s);Runtime<0> r(*this,id);(void)PalBehaviorTaskBase::Run(s,r);++updates;}
    bool IsDurationComplete(unsigned id) override {
        bool done{};Visit(id,[&](auto& s,auto& r){constexpr auto k=std::remove_reference_t<decltype(r)>::kind;if constexpr(k==1)done=GomTaskBase::IsDurationComplete(TaskAt(id));else done=Shared<k>::IsComplete(s,r);});return done;
    }
    void Finish(unsigned id) override {PalBehaviorTaskState s;Copy(s,Task(id).image,sizeof s);Runtime<0> r(*this,id);(void)PalBehaviorTaskBase::Finish(s,r);Copy(Task(id).image,s,sizeof s);++completed;}
    void DeleteTask(unsigned id,unsigned flags) override {
        auto& e=Task(id);Require(e.live && flags==1,"invalid behavior task deletion");
        Visit(id,[&](auto& s,auto& r){using C=Shared<std::remove_reference_t<decltype(r)>::kind>;C::Destruct(s,r);});
        for(auto it=helpers.begin();it!=helpers.end();)if(it->second.task==id)it=helpers.erase(it);else ++it;
        e.live=false;++deleted;
    }
    // 原原子分派器仅看到受检查的记录/属性句柄。
    void CleanupCollection(unsigned context) override {Require(context==1,"invalid behavior event context");(void)container.CleanupAndReady(queue);}
    unsigned Rand() override {return random();}
    [[noreturn]]void DivideByZero() override {throw std::domain_error("empty behavior random selection");}
    unsigned FindAttribute(unsigned record,const char* key) override {
        Require(record>=record_base && (record-record_base)%60==0 && (record-record_base)/60<group.choices.size(),"invalid behavior atomic record");
        const auto i=(record-record_base)/60;unsigned field{};const std::string_view name(key);
        if(name=="behAIAtomic-classID")field=1;else if(name=="behAIAtomic-proportion")field=2;else if(name=="beh-vtm-waitMode")field=3;else if(name=="beh-vtm-waitTime")field=4;else if(name=="beh-vtm-expectTime")field=5;else return 0;
        return 1+i*8+field;
    }
    const MonsterBehaviorChoice& Attr(unsigned a,unsigned& field) const {Require(a>=2,"invalid behavior atomic attribute");field=(a-1)%8;Require(field>=1 && field<=5 && (a-1)/8<group.choices.size(),"invalid behavior atomic attribute");return group.choices[(a-1)/8];}
    std::int32_t ToInt(unsigned a,std::int32_t) override {unsigned f;const auto& c=Attr(a,f);if(f==1)return c.class_id;if(f==2)return c.proportion;if(f==3)return c.wait_mode;throw std::logic_error("invalid behavior integer attribute");}
    float ToFloat(unsigned a,float) override {unsigned f;const auto& c=Attr(a,f);Require(f==4 || f==5,"invalid behavior float attribute");return f==4?c.wait_time:c.expect_time;}
    void InsertBehavior(std::uint32_t address,std::uint32_t kind,std::uint32_t function) override {
        Require(address==registry_token && kind<=std::uint32_t(std::numeric_limits<std::int32_t>::max()) && function && !registry.contains(static_cast<std::int32_t>(kind)),"invalid or duplicate behavior factory registration");
        Require(std::none_of(registry.begin(),registry.end(),[&](const auto& e){return e.second==function;}),"ambiguous behavior factory identity");
        registry.emplace(static_cast<std::int32_t>(kind),function);
    }
    unsigned NilAddress() override {throw std::logic_error("legacy behavior tree layout is not bound");}
    unsigned ReadDword(unsigned) override {throw std::logic_error("legacy behavior tree read is not bound");}
    unsigned CallFactory(unsigned function) override {
        Require(dispatch_scope && !dispatch_scope->pending,"behavior factory outside its dispatch scope");const auto kind=dispatch_scope->kind;
        const auto found=registry.find(kind);Require(found!=registry.end() && found->second==function && factory.Supports(kind),"behavior factory implementation is not bound");
        Require(entries.size()<10000,"too many behavior tasks");const auto id=task_base+static_cast<unsigned>(entries.size())*0x100;
        entries.push_back({{},kind,true});dispatch_scope->pending=id;
        Visit(id,[](auto& state,auto& runtime){using C=Shared<std::remove_reference_t<decltype(runtime)>::kind>;(void)C::Construct(state,runtime);});return id;
    }
    void AddTaskAndInitBase(unsigned owner,unsigned task,unsigned source,unsigned timing) override {
        Require(dispatch_scope && dispatch_scope->pending==task && owner==container_address && source==source_token && timing==timing_token,"invalid resolved behavior submission");
        const auto& scope=*dispatch_scope;
        (void)container.AddTaskAndInitialize(queue,container_address,TaskAt(task),task,scope.name,{static_cast<unsigned>(scope.options.mode),scope.options.wait_time,scope.options.expect_time});
        dispatch_scope->pending=0;
    }
    unsigned ProcessEvent(unsigned context,std::int32_t kind,unsigned target,const char* name,const BehaviorAtomicWaitOptions& options) override {
        const auto found=registry.find(kind);Require(context==1 && target==2 && found!=registry.end() && factory.Supports(kind) && !dispatch_scope,"behavior helper implementation is not bound");
        DispatchScope scope{kind,name,options};dispatch_scope=&scope;struct Guard{DispatchScope*& slot;~Guard(){slot=nullptr;}} guard{dispatch_scope};
        try{return ScriptObject::ProcessResolvedFactory(container_address,found->second,source_token,timing_token,*this);}
        catch(...){if(scope.pending)DeleteTask(scope.pending,1);throw;}
    }
    void NotifyTask(unsigned id,unsigned binder,unsigned) override {Require(binder==1,"invalid behavior task binder");Visit(id,[](auto& s,auto& r){using C=Shared<std::remove_reference_t<decltype(r)>::kind>;(void)C::BindOwner(s,1,r);});}
    void MarkProcessed(unsigned context) override {Require(context==1,"invalid behavior processed context");processed=true;(void)container.BeginWaiting(queue);}
    void Cancel(){(void)container.CleanupAndReady(queue);}
};
MonsterBehaviorTasks::MonsterBehaviorTasks(IMonsterBehaviorActionFactory& factory,std::function<unsigned()> random):MonsterBehaviorTasks(factory,std::move(random),true){}
MonsterBehaviorTasks::MonsterBehaviorTasks(IMonsterBehaviorActionFactory& factory,std::function<unsigned()> random,bool register_factories):impl_(std::make_unique<Impl>(factory,std::move(random),register_factories)){}
void MonsterBehaviorTasks::RegisterFactory(std::uint32_t id,std::uint32_t factory){Require(!impl_->busy && !impl_->processed && impl_->entries.empty(),"factory registration after task dispatch");impl_->InsertBehavior(Impl::registry_token,id,factory);}
void MonsterBehaviorTasks::ClearFactories(){Require(!impl_->busy && impl_->queue.count==0,"factory registry cleanup with live tasks");impl_->registry.clear();}
MonsterBehaviorTasks::~MonsterBehaviorTasks()=default;
std::uint64_t MonsterBehaviorTasks::BindContainerImage(std::span<std::uint32_t,4> image,std::uint32_t address){
    auto& s=*impl_;Require(!s.busy && s.bound_image.empty() && address && address%4==0 && address<=std::numeric_limits<std::uint32_t>::max()-sizeof(s.queue),"invalid or duplicate task container binding");
    Require(s.next_binding<std::numeric_limits<std::uint64_t>::max(),"task container binding exhausted");s.Relocate(address);s.bound_image=image;s.binding=s.next_binding++;s.PublishImage();return s.binding;
}
void MonsterBehaviorTasks::UnbindContainerImage(std::uint64_t binding){auto& s=*impl_;Require(!s.busy && binding && s.binding==binding && !s.bound_image.empty(),"invalid task container binding release");s.Relocate(2,false);s.bound_image={};s.binding=0;}
bool MonsterBehaviorTasks::OwnsContainerImage(std::uint64_t binding) const{return binding && impl_->binding==binding && !impl_->bound_image.empty();}
std::size_t MonsterBehaviorTasks::RegisteredFactoryCount() const{return impl_->registry.size();}
std::optional<std::uint32_t> MonsterBehaviorTasks::FactoryAddress(std::int32_t kind) const{const auto i=impl_->registry.find(kind);return i==impl_->registry.end()?std::nullopt:std::optional<std::uint32_t>(i->second);}
bool MonsterBehaviorTasks::Start(const MonsterBehaviorDefinition& definition){
    auto& s=*impl_;return s.Mutate([&]{s.Cancel();s.entries.clear();s.processed=false;s.failed=false;
        if(!definition.fuzzy)return false;
        s.group=*definition.fuzzy;Require(s.group.mode.has_value() && s.group.choices.size()<=10000,"behavior group mode is not initialized");
        for(const auto& c:s.group.choices)Require(std::isfinite(c.wait_time) && std::isfinite(c.expect_time),"non-finite behavior timing");
        s.entries.reserve(s.group.choices.size());pal4::ScriptEventDataState event{};event.words[4]=std::bit_cast<unsigned>(*s.group.mode);event.words[6]=Impl::record_base;event.words[7]=Impl::record_base+static_cast<unsigned>(s.group.choices.size())*60;
        try{return pal4::BehaviorAtomicDispatcher::Dispatch(event,1,2,1,s)!=0;}catch(...){s.Cancel();s.failed=true;throw;}
    });
}
std::uint32_t MonsterBehaviorTasks::Advance(float seconds){auto& s=*impl_;return s.Mutate([&]{Require(std::isfinite(seconds) && seconds>=0 && !s.failed,"invalid behavior task frame");try{return s.container.Update(s.queue,seconds);}catch(...){s.Cancel();s.failed=true;throw;}});}
void MonsterBehaviorTasks::Cancel(){impl_->Mutate([&]{impl_->Cancel();});}
bool MonsterBehaviorTasks::Ready() const{impl_->ValidateImage();return impl_->queue.ready!=0;}
std::size_t MonsterBehaviorTasks::TaskCount() const{impl_->ValidateImage();return impl_->queue.count;}
std::size_t MonsterBehaviorTasks::HelperCount() const{return impl_->helpers.size();}
void MonsterBehaviorTasks::WriteReport(std::ostream& out) const {
    const auto& s=*impl_;out<<"{\"tasks\":"<<TaskCount()<<",\"helpers\":"<<HelperCount()<<",\"ready\":"<<(Ready()?"true":"false")
        <<",\"processed\":"<<(s.processed?"true":"false")<<",\"failed\":"<<(s.failed?"true":"false")
        <<",\"temporary_blocks\":"<<(s.memory.size()-1)<<",\"started\":"<<s.started<<",\"completed\":"<<s.completed<<",\"deleted\":"<<s.deleted<<",\"updates\":"<<s.updates<<'}';Require(bool(out),"cannot write behavior task report");
}
} // namespace pal4::portable
