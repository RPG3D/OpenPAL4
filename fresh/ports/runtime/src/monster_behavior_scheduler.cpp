#include "monster_behavior_scheduler.h"
#include "pal4/portable/object_memory.h"
#include "pal4/script_event_processor.h"
#include "pal4/script_event_data.h"
#include "pal4/binary_xml_config.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <utility>
namespace pal4::portable::detail {
namespace {void Require(bool b,const char* s){if(!b)throw std::logic_error(s);}}
struct MonsterBehaviorScheduler::Impl final:IScriptEventSchedulerRuntime,IScriptEventDocumentRuntime {
    static constexpr std::uint32_t context=0x1000,tree=0x6000,target=0x7000;
    MonsterSchedulerBindings bindings;ScriptEventProcessorState processor{};ScriptEventDataState event{};
    ObjectMemory local_storage;ObjectMemory& storage;BinaryXmlNode& memory;std::set<std::uint32_t> owned;
    std::uint32_t memory_token{},event_token{},events{},candidates{},document_token{};
    std::optional<std::int32_t> group_mode;bool original_fuzzy{};
    std::optional<BinaryXmlNode> condition;bool allocated{},busy{},event_alive{},event_cleanup_pending{},memory_alive{},memory_cleanup_pending{},closed{},prepared{};
    std::uint64_t frames{},dispatches{},updates{},conditions{},documents{},manager_frames{},reloads{},event_deletions{};
    std::uint32_t bound_context=context,bound_tree=tree,bound_target=target,manager_token{};
    const MonsterBehaviorManagerState* manager_image{};
    std::uint32_t AllocateStorage(std::uint32_t bytes){const auto a=storage.Allocate(bytes);try{owned.insert(a);}catch(...){(void)storage.Free(a);throw;}return a;}
    std::uint32_t ReleaseStorage(std::uint32_t a){if(!a)return 0;Require(owned.contains(a),"foreign scheduler allocation release");const auto result=storage.Free(a);owned.erase(a);return result;}
    struct EventCleanup final:IScriptEventDataRuntime {
        Impl& owner;explicit EventCleanup(Impl& s):owner(s){}
        void DestroyXmlNode(std::uint32_t a) override {Require(a==owner.document_token && owner.allocated && owner.condition,"unbound scheduler event XML cleanup");owner.condition.reset();}
        std::uint32_t Free(std::uint32_t a) override {
            if(!a)return 0;
            Require(a==owner.document_token && owner.allocated && !owner.condition,"unbound scheduler event allocation cleanup");
            const auto result=owner.ReleaseStorage(a);owner.document_token=0;owner.allocated=false;return result;
        }
        std::uint8_t ReadByte(std::uint32_t a) override{return owner.storage.ReadByte(a);}
        void WriteByte(std::uint32_t a,std::uint8_t v) override{owner.storage.WriteByte(a,v);}
    } event_runtime{*this};
    struct Processor final:IScriptEventProcessorRuntime {
        Impl& owner;explicit Processor(Impl& value):owner(value){}
        std::uint32_t Allocate(std::uint32_t bytes) override {Require(bytes==60 && !owner.memory_token && !owner.memory_alive,"invalid processor memory allocation");owner.memory_token=owner.AllocateStorage(bytes);return owner.memory_token;}
        std::uint32_t InitializeDocument(std::uint32_t a,const char* name,std::uint32_t option) override {
            Require(a==owner.memory_token && !owner.memory_alive && !option && std::string_view(name)=="behAIAtomicMgr-memory","invalid processor memory document initialization");
            // 持久节点由调用方拥有；此处建立处理器视图，不复制或清空恢复后的数据。
            if(owner.memory.name.empty())owner.memory.name=name;
            owner.memory_alive=true;return a;
        }
        void CleanupDocument(std::uint32_t a) override {Require(a==owner.memory_token && owner.memory_alive && !owner.memory_cleanup_pending,"invalid processor memory document cleanup");owner.memory_cleanup_pending=true;}
        void DestroyDocument(std::uint32_t a) override {Require(a==owner.memory_token && owner.memory_alive && owner.memory_cleanup_pending,"invalid processor memory document destruction");owner.memory_alive=false;owner.memory_cleanup_pending=false;}
        std::uint32_t ReadDword(std::uint32_t address) override {Require(address==owner.events && owner.event_alive,"invalid pursuit event read");return owner.storage.ReadDword(address);}
        void CleanupEventData(std::uint32_t address) override {
            Require(address==owner.event_token && owner.event_alive && !owner.event_cleanup_pending,"invalid pursuit event cleanup");
            (void)ScriptEventData::Cleanup(owner.event,owner.event_runtime);owner.event_cleanup_pending=true;
        }
        std::uint32_t Free(std::uint32_t address) override {
            if(!address)return 0;
            if(address==owner.memory_token){Require(!owner.memory_alive,"live processor document allocation release");const auto result=owner.ReleaseStorage(address);owner.memory_token=0;return result;}
            if(address==owner.event_token){Require(owner.event_alive && owner.event_cleanup_pending,"invalid pursuit event release");const auto result=owner.ReleaseStorage(address);owner.event_token=0;owner.event_cleanup_pending=false;owner.event_alive=false;++owner.event_deletions;return result;}
            Require(address==owner.events || address==owner.candidates,"unknown processor array release");
            const auto result=owner.ReleaseStorage(address);if(address==owner.events)owner.events=0;else owner.candidates=0;return result;
        }
    } processor_runtime{*this};
    void PursuitContext(MonsterBehaviorManagerState& state,std::uint32_t token){Require(!closed && busy && manager_image==&state && manager_token==token,"pursuit requires the active owned manager frame");}
    void CreateEvent(ScriptEventProcessorState& image){
        Require(!event_alive && !event_cleanup_pending && !condition && !allocated,"event construction requires an empty owner");
        const auto token=AllocateStorage(sizeof(event));
        try{if(!events)events=AllocateStorage(4);event={};(void)ScriptEventData::Initialize(event,token);if(group_mode)event.words[4]=static_cast<std::uint32_t>(*group_mode);storage.WriteDword(events,token);}
        catch(...){(void)ReleaseStorage(token);throw;}
        event_token=token;event_alive=true;image.words[1]=events;image.words[2]=events+4;image.words[3]=events+4;
    }
    Impl(const MonsterBehaviorDefinition& definition,MonsterSchedulerBindings b,BinaryXmlNode& actual_memory,ObjectMemory* shared,bool prepare)
        :bindings(std::move(b)),storage(shared?*shared:local_storage),memory(actual_memory),group_mode(definition.fuzzy?definition.fuzzy->mode:std::nullopt),original_fuzzy(bool(definition.fuzzy)){
        Require(bool(bindings.ready) && bool(bindings.dispatch) && bool(bindings.advance) && bool(bindings.random),"incomplete monster scheduler services");
        auto previous_name=memory.name;
        try {
            (void)ScriptEventProcessor::Initialize(processor,context,0,processor_runtime);
            if(prepare)PrepareBehavior();
        }catch(...){try{(void)ScriptEventProcessor::Cleanup(processor,processor_runtime);}catch(...){}ReleaseRemaining();memory.name=std::move(previous_name);throw;}
    }
    void PrepareBehavior(){
        Require(!closed && !busy && !prepared,"monster processor behavior is already prepared or closed");
        try{
            candidates=AllocateStorage(4);processor.words[9]=candidates;processor.words[10]=candidates;processor.words[11]=candidates+4;
            if(original_fuzzy)CreateEvent(processor);
            (void)ScriptEventProcessor::SetFirstValue28(processor,0x594e00u);prepared=true;
        }catch(...){try{Close();}catch(...){ReleaseRemaining();closed=true;}throw;}
    }
    void ReleaseRemaining() noexcept {while(!owned.empty()){const auto a=*owned.begin();try{(void)storage.Free(a);}catch(...){}owned.erase(a);}}
    void Close(){if(closed)return;Require(!busy,"processor cleanup during its active frame");(void)ScriptEventProcessor::Cleanup(processor,processor_runtime);Require(owned.empty(),"processor cleanup left owned allocations");closed=true;}
    ~Impl() override {try{Close();}catch(...){ReleaseRemaining();}}
    std::uint8_t ReadByte(std::uint32_t a) override {
        Require(a==bound_context+12,"invalid scheduler queue flag token");const auto actual=bindings.ready()?1u:0u;
        if(bound_context!=context){Require(manager_image,"missing manager task image");const auto value=manager_image->words[(MonsterBehaviorManager::kScriptOffset+12)/4]&0xffu;Require(value==actual,"manager task ready byte differs from its actual queue");return static_cast<std::uint8_t>(value);}
        return static_cast<std::uint8_t>(actual);
    }
    std::uint32_t ReadDword(std::uint32_t a) override {
        Require((a==events && event_alive && processor.words[1]==events) || (a==candidates && storage.ReadDword(a)==event_token),"invalid scheduler array token");return storage.ReadDword(a);
    }
    std::int32_t Random() override {const auto v=bindings.random();Require(v<=32767,"monster scheduler random range");return static_cast<std::int32_t>(v);}
    std::uint32_t Allocate(std::uint32_t bytes) override {Require(bytes==60 && !allocated && !condition,"invalid scheduler condition allocation");document_token=AllocateStorage(bytes);allocated=true;return document_token;}
    std::uint32_t InitializeDocument(std::uint32_t a,const char* name,std::uint32_t option) override {
        Require(a==document_token && allocated && !condition && !option && std::string_view(name)=="behAIAtomic-condition-attribute","invalid scheduler condition document");
        condition.emplace();condition->name=name;++documents;return a;
    }
    void Free(std::uint32_t a) override {Require(a==document_token && allocated,"invalid scheduler condition release");condition.reset();(void)ReleaseStorage(a);allocated=false;document_token=0;}
    std::uint32_t GetConditionDocument(std::uint32_t e) override {Require(e==event_token && event_alive,"invalid scheduler event token");return ScriptEventData::GetOrCreateConditionDocument(event,*this);}
    std::uint32_t CallCondition(std::uint32_t callback,std::uint32_t d) override {
        Require(callback==0x594e00u && d==document_token && condition && condition->name=="behAIAtomic-condition-attribute","unbound monster event condition");
        ++conditions;return 1;
    }
    void InsertCandidate(ScriptEventProcessorState& state,std::uint32_t position,std::uint32_t e) override {
        Require(state.words[9]==candidates && state.words[10]==candidates && position==candidates && e==event_token,"invalid monster candidate insertion");storage.WriteDword(candidates,e);state.words[10]=candidates+4;
    }
    std::uint32_t DispatchEvent(std::uint32_t e,std::uint32_t c,std::uint32_t t,std::uint32_t o) override {
        Require(e==event_token && c==bound_context && t==bound_tree && o==bound_target,"invalid monster event dispatch");++dispatches;return bindings.dispatch()?1u:0u;
    }
    std::uint32_t UpdateContainer(std::uint32_t c,float elapsed) override {Require(c==bound_context,"invalid monster task update context");++updates;return bindings.advance(elapsed);}
};
MonsterBehaviorScheduler::MonsterBehaviorScheduler(const MonsterBehaviorDefinition& definition,MonsterSchedulerBindings bindings,BinaryXmlNode& memory,ObjectMemory* storage,bool prepare):impl_(std::make_unique<Impl>(definition,std::move(bindings),memory,storage,prepare)){}
void MonsterBehaviorScheduler::PrepareBehavior(){impl_->PrepareBehavior();}
bool MonsterBehaviorScheduler::Prepared() const{return impl_->prepared;}
void MonsterBehaviorScheduler::PrepareComponent(MonsterBehaviorManagerState& state){
    auto& s=*impl_;Require(!s.closed && !s.busy && !s.prepared,"monster component processor is already prepared or closed");
    ScriptEventProcessorState image{};std::copy_n(state.words.begin()+MonsterBehaviorManager::kProcessorOffset/4,image.words.size(),image.words.begin());
    for(unsigned i=0;i<image.words.size();++i)if(i!=6 && i!=7)Require(image.words[i]==s.processor.words[i],"monster component overwrote owned processor data");
    Require((image.words[6]&0xff)==1 && (image.words[6]&0xffffff00u)==(s.processor.words[6]&0xffffff00u) && image.words[7]==0x594e00,"monster component processor flags are not initialized");
    s.processor=image;
    try{s.PrepareBehavior();}catch(...){CopyProcessorTo(state);throw;}
    CopyProcessorTo(state);
}
void MonsterBehaviorScheduler::CopyProcessorTo(MonsterBehaviorManagerState& state) const{std::copy(impl_->processor.words.begin(),impl_->processor.words.end(),state.words.begin()+MonsterBehaviorManager::kProcessorOffset/4);}
MonsterBehaviorScheduler::~MonsterBehaviorScheduler()=default;
void MonsterBehaviorScheduler::Close(){impl_->Close();}
bool MonsterBehaviorScheduler::Closed() const{return impl_->closed;}
std::uint32_t MonsterBehaviorScheduler::MemoryToken() const{return impl_->memory_token;}
std::size_t MonsterBehaviorScheduler::AllocationCount() const{return impl_->owned.size();}
BinaryXmlNode& MonsterBehaviorScheduler::Memory(){Require(!impl_->closed && impl_->memory_alive,"closed processor memory view");return impl_->memory;}
void MonsterBehaviorScheduler::SetEnabled(bool enabled){Require(!impl_->closed && !impl_->busy,"monster scheduler is closed or reentrant");(void)ScriptEventProcessor::SetFlag24(impl_->processor,enabled?1u:0u);}
bool MonsterBehaviorScheduler::Enabled() const{return !impl_->closed && (impl_->processor.words[6]&0xff)!=0;}
std::uint32_t MonsterBehaviorScheduler::Advance(float elapsed){
    auto& s=*impl_;Require(!s.closed && !s.busy && std::isfinite(elapsed) && elapsed>=0,"invalid or reentrant monster scheduler frame");
    s.busy=true;struct Guard{bool& busy;~Guard(){busy=false;}} guard{s.busy};++s.frames;
    return ScriptEventProcessor::HandleEventProcessing(s.processor,elapsed,Impl::context,Impl::tree,Impl::target,0,s);
}
std::uint32_t MonsterBehaviorScheduler::AdvanceManager(MonsterBehaviorManagerState& state,std::uint32_t token,float elapsed,std::uint32_t argument,IMonsterBehaviorFrameRuntime& services){
    auto& s=*impl_;Require(!s.closed && !s.busy && token && token%4==0 && token<=std::numeric_limits<std::uint32_t>::max()-sizeof(state) && std::isfinite(elapsed) && elapsed>=0,"invalid or reentrant monster manager frame");
    Require(!s.manager_image || (s.manager_image==&state && s.manager_token==token),"monster manager image/token belongs to another owner");
    s.manager_image=&state;s.manager_token=token;
    // The movement owner exclusively owns the embedded processor view; callers
    // own the remaining actual manager fields and provide real engine services.
    const auto begin=state.words.begin()+MonsterBehaviorManager::kProcessorOffset/4;
    std::copy(s.processor.words.begin(),s.processor.words.end(),begin);
    s.busy=true;s.bound_context=token+MonsterBehaviorManager::kScriptOffset;s.bound_tree=token+0x698;s.bound_target=token+0x6ac;++s.manager_frames;
    struct Guard {Impl& s;MonsterBehaviorManagerState& state;~Guard(){
        std::copy_n(state.words.begin()+MonsterBehaviorManager::kProcessorOffset/4,s.processor.words.size(),s.processor.words.begin());
        s.bound_context=Impl::context;s.bound_tree=Impl::tree;s.bound_target=Impl::target;s.busy=false;
    }} guard{s,state};
    struct Frame final:IMonsterBehaviorFrameRuntime {
        Impl& owner;IMonsterBehaviorFrameRuntime& services;
        Frame(Impl& o,IMonsterBehaviorFrameRuntime& r):owner(o),services(r){}
        std::uint32_t ProcessManagerState(MonsterBehaviorManagerState& v,std::uint32_t a,float e) override {return services.ProcessManagerState(v,a,e);}
        void CheckStateAndProcessCollision(MonsterBehaviorManagerState& v,std::uint32_t a,float e) override {services.CheckStateAndProcessCollision(v,a,e);}
        IScriptEventSchedulerRuntime& SchedulerRuntime() override {return owner;}
        std::uint32_t UpdatePosition(std::uint32_t a,float e,std::uint32_t arg) override {return services.UpdatePosition(a,e,arg);}
    } frame(s,services);
    return MonsterBehaviorManager::Update(state,token,elapsed,argument,frame);
}
IScriptEventProcessorRuntime& MonsterBehaviorScheduler::PursuitProcessor(MonsterBehaviorManagerState& state,std::uint32_t token){impl_->PursuitContext(state,token);return impl_->processor_runtime;}
void MonsterBehaviorScheduler::RestoreFuzzy(MonsterBehaviorManagerState& state,std::uint32_t token,bool has_fuzzy){
    auto& s=*impl_;s.PursuitContext(state,token);const auto first=MonsterBehaviorManager::kProcessorOffset/4;
    Require(!s.event_alive && !s.event_cleanup_pending && !s.condition && !s.allocated && state.words[first+1]==state.words[first+2],"fuzzy reload requires cleared owned events");
    Require(has_fuzzy==s.original_fuzzy,"fuzzy reload does not match the actual definition");
    if(has_fuzzy){ScriptEventProcessorState image{};std::copy_n(state.words.begin()+first,image.words.size(),image.words.begin());s.CreateEvent(image);std::copy(image.words.begin(),image.words.end(),state.words.begin()+first);}++s.reloads;
}
void MonsterBehaviorScheduler::WriteReport(std::ostream& out) const {
    const auto& s=*impl_;out<<"{\"enabled\":"<<(Enabled()?"true":"false")<<",\"frames\":"<<s.frames<<",\"manager_frames\":"<<s.manager_frames<<",\"dispatches\":"<<s.dispatches<<",\"updates\":"<<s.updates<<",\"condition_calls\":"<<s.conditions<<",\"condition_documents\":"<<s.documents<<",\"event_deletions\":"<<s.event_deletions<<",\"fuzzy_reloads\":"<<s.reloads<<'}';
    Require(bool(out),"cannot write monster scheduler report");
}
}
