#include "pal4/portable/effect_tasks.h"
#include "pal4/dialog_object.h"
#include "pal4/gom_task_container.h"
#include "pal4/effect_system_task_object.h"
#include "pal4/showobj_task.h"
#include "pal4/hideobj_task.h"
#include "pal4/scale_task.h"
#include "pal4/change_color_task.h"
#include "pal4/decress_task.h"
#include "pal4/xml_float_to_int.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <span>
#include <stdexcept>
#include <type_traits>
namespace pal4::portable {
namespace {
using namespace pal4;
void Require(bool v,const char* message){if(!v)throw std::logic_error(message);}
// 基类始终是真实C++对象；派生处理器通过memcpy副本访问其数值，不强转对象类型。
struct TaskImage {GomTaskBaseState base;std::array<std::byte,sizeof(ChangeColorTaskState)-sizeof(GomTaskBaseState)> tail{};};
static_assert(sizeof(TaskImage)==sizeof(ChangeColorTaskState) && std::is_trivially_copyable_v<TaskImage>);
template<class To,class From> void CopyPrefix(To& target,const From& source,std::size_t count){
    static_assert(std::is_trivially_copyable_v<To> && std::is_trivially_copyable_v<From>);
    auto to=std::as_writable_bytes(std::span(&target,1));const auto from=std::as_bytes(std::span(&source,1));
    Require(count<=to.size() && count<=from.size(),"effect task representation bounds");std::copy_n(from.begin(),count,to.begin());
}
}
struct EffectTasks::Impl final:IGomTaskBaseRuntime,IGomTaskContainerRuntime,IDialogObjectTaskRuntime,
    IShowobjTaskRuntime,IHideobjTaskRuntime,IHideobjFacetRuntime,IScaleTaskRuntime,IScaleTaskConfigRuntime,
    IChangeColorTaskRuntime,IChangeColorConfigRuntime,IChangeColorProgressRuntime,IChangeColorAnimationRuntime,
    IDecressTaskRuntime,IDecressTaskConfigRuntime,IDecressTaskFacetRuntime {
    static constexpr unsigned records=0x10000,xml_slots=0x20000,effects=0x30000,task_start=0x100000,memory_start=0x10000000;
    enum class Kind {show,hide,scale,color,decress};
    struct Task {TaskImage image;Kind kind{};std::string name;unsigned binding{};bool live{};};
    std::vector<EffectTaskBinding> bindings;std::function<int()> random;
    std::vector<Task> entries;std::map<unsigned,std::vector<std::byte>> allocations;
    unsigned next_memory=memory_start,started{},completed{},deleted{},frames{};float seconds{};
    GomTaskBase base{*this};EffectSystemTaskObject common{base,*this};
    GomTaskContainer container{*this,base};GomTaskContainerState queue{};DialogObjectState dialog{};
    explicit Impl(std::vector<EffectTaskBinding> b,std::function<int()> r):bindings(std::move(b)),random(std::move(r)) {
        Require(!bindings.empty() && bindings.size()<=1024,"effect task count limit");
        for(const auto& binding:bindings)Require(binding.target!=nullptr || binding.dummy,"effect task target is missing");
        entries.reserve(bindings.size());(void)container.Initialize(queue,0);
    }
    ~Impl() override {if(queue.sentinel)(void)container.Destroy(queue);for(auto& e:entries)if(e.live){(void)base.Destroy(e.image.base);e.live=false;}}
    void Initialize(){
        dialog.words[2]=1;dialog.words[24]=records;dialog.words[25]=records+static_cast<unsigned>(bindings.size())*8;
        dialog.words[28]=effects;dialog.words[32]=xml_slots;
        (void)DialogObject::InitializeTasks(dialog,1,*this);
    }
    Task& Entry(unsigned t){Require(t>=task_start && (t-task_start)%0x100==0 && (t-task_start)/0x100<entries.size(),"invalid effect task handle");return entries[(t-task_start)/0x100];}
    template<class S,class F> auto With(unsigned t,F operation){
        auto& entry=Entry(t);S state{};static_assert(std::is_trivially_copyable_v<S> && sizeof(S)<=sizeof(TaskImage));
        CopyPrefix(state,entry.image,sizeof(S));auto result=operation(state);CopyPrefix(entry.image,state,sizeof(S));return result;
    }
    std::span<std::byte> Memory(unsigned a,std::size_t n){
        if(a>=task_start && a<memory_start){const auto token=task_start+((a-task_start)/0x100)*0x100;auto& e=Entry(token);auto bytes=std::as_writable_bytes(std::span(&e.image,1));Require(a-token<=bytes.size() && n<=bytes.size()-(a-token),"effect task field bounds");return bytes.subspan(a-token,n);}
        auto it=allocations.upper_bound(a);Require(it!=allocations.begin(),"unknown effect task allocation");--it;const auto offset=a-it->first;
        Require(offset<=it->second.size() && n<=it->second.size()-offset,"effect task allocation bounds");return std::span(it->second).subspan(offset,n);
    }
    unsigned AllocateMemory(unsigned bytes) override{Require(bytes>0 && bytes<0x1000 && next_memory<0xf0000000,"effect task allocation limit");const auto t=next_memory;next_memory+=0x1000;allocations.emplace(t,std::vector<std::byte>(bytes));return t;}
    unsigned FreeMemory(unsigned a) override{if(a)Require(allocations.erase(a)==1,"invalid effect task free");return 0;}
    unsigned ReadWord(unsigned a) override{auto bytes=Memory(a,4);unsigned value;std::memcpy(&value,bytes.data(),4);return value;}
    void WriteWord(unsigned a,unsigned v) override{auto bytes=Memory(a,4);std::memcpy(bytes.data(),&v,4);}
    void CopyCString(unsigned a,const char* s,unsigned n) override{auto bytes=Memory(a,n);std::memcpy(bytes.data(),s,n);}
    GomTaskBaseState& TaskAt(unsigned t) override{return Entry(t).image.base;}
    void DeleteTask(unsigned t,unsigned flags) override{auto& e=Entry(t);Require(e.live && flags==1,"invalid effect task deletion");(void)base.Destroy(e.image.base);e.live=false;++deleted;}
    bool IsWaitComplete(unsigned t) override{return base.IsWaitComplete(TaskAt(t));}
    bool IsDurationComplete(unsigned t) override{
        switch(Entry(t).kind){
        case Kind::scale:return With<ScaleTaskState>(t,[&](auto& s){return ScaleTask::IsDurationComplete(s,*this);});
        case Kind::color:return With<ChangeColorTaskState>(t,[&](auto& s){return ChangeColorTask::IsComplete(s,*this);});
        default:return GomTaskBase::IsDurationComplete(TaskAt(t));}
    }
    void BeginRun(unsigned t) override{
        switch(Entry(t).kind){
        case Kind::show:(void)With<ShowobjTaskState>(t,[&](auto& s){return ShowobjTask::Start(s,*this);});break;
        case Kind::hide:(void)With<HideobjTaskState>(t,[&](auto& s){return HideobjTask::Start(s,*this);});break;
        case Kind::scale:(void)With<ScaleTaskState>(t,[&](auto& s){return ScaleTask::Start(s,*this);});break;
        case Kind::color:(void)With<ChangeColorTaskState>(t,[&](auto& s){return ChangeColorTask::InitializeProgress(s,*this);});break;
        case Kind::decress:(void)With<DecressTaskState>(t,[&](auto& s){return DecressTask::Start(s,*this);});break;
        }++started;
    }
    void Tick(unsigned t,float delta) override{
        if(Entry(t).kind==Kind::scale)(void)With<ScaleTaskState>(t,[&](auto& s){return ScaleTask::Update(s,delta,*this);});
        if(Entry(t).kind==Kind::color)(void)With<ChangeColorTaskState>(t,[&](auto& s){return ChangeColorTask::UpdateAnimation(s,delta,*this);});
    }
    void Finish(unsigned t) override{
        if(Entry(t).kind==Kind::scale)(void)With<ScaleTaskState>(t,[&](auto& s){return ScaleTask::Stop(s,*this);});
        if(Entry(t).kind==Kind::color)(void)With<ChangeColorTaskState>(t,[&](auto& s){return ChangeColorTask::FinishAnimation(s,*this);});
        if(Entry(t).kind==Kind::decress)(void)With<DecressTaskState>(t,[](auto& s){return DecressTask::Reset(s);});
        ++completed;
    }
    const BinaryXmlNode& Node(unsigned n) const{Require(n>0 && n<=bindings.size(),"invalid effect task XML handle");return bindings[n-1].configuration;}
    static void Numeric(const BinaryXmlAttribute& a){Require(std::holds_alternative<std::int8_t>(a.value)||std::holds_alternative<std::int32_t>(a.value)||std::holds_alternative<float>(a.value),"effect task XML text-number conversion is not bound");}
    std::optional<int> IntAttribute(unsigned n,std::string_view key,bool) override{const auto* a=Node(n).FindAttribute(key);if(!a)return {};Numeric(*a);if(const auto* f=std::get_if<float>(&a->value))return XmlFloatToInt(std::bit_cast<unsigned>(*f));return std::bit_cast<int>(a->ToInt(0));}
    std::optional<float> FloatAttribute(unsigned n,std::string_view key) override{const auto* a=Node(n).FindAttribute(key);if(!a)return {};Numeric(*a);const auto v=a->ToFloat(0);Require(std::isfinite(v),"nonfinite effect task parameter");return v;}
    std::optional<std::string_view> StringAttribute(unsigned n,std::string_view key) override{const auto* a=Node(n).FindAttribute(key);if(!a)return {};const auto* s=std::get_if<std::string>(&a->value);Require(s!=nullptr,"effect task class must be text");return *s;}
    int ReadInt(unsigned n,std::string_view key,int fallback) override{return IntAttribute(n,key,false).value_or(fallback);}
    float ReadFloat(unsigned n,std::string_view key,float fallback) override{return FloatAttribute(n,key).value_or(fallback);}
    bool EffectsEnabled() override{return false;} // Retail system byte+1: skip editor-only dummy objects.
    bool EffectIsActive(unsigned e) override{Require(e>0 && e<=bindings.size(),"invalid effect task object");return bindings[e-1].dummy;}
    unsigned ReadWord(unsigned a,unsigned offset) override{
        a+=offset;
        if(a>=records && a<records+bindings.size()*8){Require((a-records)%8==0,"invalid effect task record read");return (a-records)/8;}
        if(a>=xml_slots && a<xml_slots+bindings.size()*4){Require((a-xml_slots)%4==0,"invalid effect task XML slot");return 1+(a-xml_slots)/4;}
        if(a>=effects && a<effects+bindings.size()*20){Require((a-effects)%20==16,"invalid effect task target record");return 1+(a-effects)/20;}
        return ReadWord(a);
    }
    void WriteWord(unsigned a,unsigned offset,unsigned v) override{a+=offset;if(a>=records && a<records+bindings.size()*8){Require((a-records)%8==4,"invalid effect task record write");return;}WriteWord(a,v);}
    unsigned CreateTask(unsigned n) override{
        const auto name=StringAttribute(n,"Attr_Class_Name").value_or("");Kind kind;
        if(name=="ShowobjTask")kind=Kind::show;else if(name=="HideobjTask")kind=Kind::hide;
        else if(name=="ScaleTask")kind=Kind::scale;else if(name=="ChangeColorTask")kind=Kind::color;
        else if(name=="DecressTask"){Require(bindings.at(n-1).target->HasEmissionFacet(),"Decress target has no emission facet");kind=Kind::decress;}
        else throw std::logic_error("effect task factory is not bound: "+std::string(name));
        const auto t=task_start+static_cast<unsigned>(entries.size())*0x100;Task e{};e.kind=kind;e.name=name;e.binding=n-1;e.live=true;entries.push_back(std::move(e));
        switch(kind){
        case Kind::show:(void)With<ShowobjTaskState>(t,[&](auto& s){ShowobjTask::Construct(s,*this);return 0;});break;
        case Kind::hide:(void)With<HideobjTaskState>(t,[&](auto& s){HideobjTask::Construct(s,*this);return 0;});break;
        case Kind::scale:(void)With<ScaleTaskState>(t,[&](auto& s){ScaleTask::Construct(s,*this);return ScaleTask::LoadConfig(s,n,*this);});break;
        case Kind::color:(void)With<ChangeColorTaskState>(t,[&](auto& s){ChangeColorTask::Construct(s,*this);return ChangeColorTask::LoadConfig(s,n,*this);});break;
        case Kind::decress:(void)With<DecressTaskState>(t,[&](auto& s){DecressTask::Construct(s,*this);return DecressTask::LoadConfig(s,n,*this);});break;
        }return t;
    }
    void AddTaskAndInitialize(unsigned manager,unsigned t,std::string_view name,const DialogTaskTiming& timing) override{
        Require(manager==1 && name==Entry(t).name,"invalid effect task manager");
        Require(timing.wait_mode>=1 && timing.wait_mode<=3 && std::isfinite(timing.wait_time) && std::isfinite(timing.expected_time),"unsupported effect task timing");
        (void)container.AddTaskAndInitialize(queue,1,TaskAt(t),t,Entry(t).name.c_str(),{static_cast<unsigned>(timing.wait_mode),timing.wait_time,timing.expected_time});
    }
    void BindDialogObject(unsigned t,unsigned owner) override{Require(owner==1,"invalid effect task Dialog");WriteWord(t+68,owner);}
    void PrepareTask(unsigned t) override{(void)With<EffectSystemTaskObjectState>(t,[&](auto& s){return common.SetupState(s);});}
    void StartTask(unsigned t) override{(void)Entry(t);} // 四个原类的virtual+48均为空操作。
    template<class S> void Construct(S& s){EffectSystemTaskObjectState state{};(void)common.Construct(state);CopyPrefix(s,state,sizeof(state));}
    template<class S> void Destroy(S& s){EffectSystemTaskObjectState state{};CopyPrefix(state,s,sizeof(state));(void)common.Cleanup(state);CopyPrefix(s,state,sizeof(state));}
    void ConstructBase(ShowobjTaskState& s) override{Construct(s);}void DestroyBase(ShowobjTaskState& s) override{Destroy(s);}
    void ConstructBase(HideobjTaskState& s) override{Construct(s);}void DestroyBase(HideobjTaskState& s) override{Destroy(s);}
    void ConstructBase(ScaleTaskState& s) override{Construct(s);}void DestroyBase(ScaleTaskState& s) override{Destroy(s);}
    void ConstructBase(ChangeColorTaskState& s) override{Construct(s);}void DestroyBase(ChangeColorTaskState& s) override{Destroy(s);}
    void ConstructBase(DecressTaskState& s) override{Construct(s);}void DestroyBase(DecressTaskState& s) override{Destroy(s);}
    EffectVisualTarget& Target(unsigned f){Require(f>0 && f<=bindings.size(),"invalid effect visual facet");return *bindings[f-1].target;}
    unsigned ResolveFacet(unsigned owner,std::string_view) override{(void)Target(owner);return owner;}
    unsigned ResolveColorFacet(unsigned owner,std::string_view name) override{return ResolveFacet(owner,name);}
    void Show(unsigned f) override{Target(f).Show(true);}void Hide(unsigned f) override{Target(f).Show(false);}
    void StopFacet(unsigned f) override{Target(f).StopEmission();}
    void StartFacet(unsigned f,float duration,std::uint8_t increase) override{Target(f).SetEmissionTime(duration,increase!=0);}
    void ReadScale(unsigned f,std::array<float,3>& v) override{v=Target(f).Scale();}
    void SetScale(unsigned f,const std::array<float,3>& v,unsigned mode) override{Require(mode<=1,"invalid effect scale mode");Target(f).SetScale(v,mode!=0);}
    bool BaseDurationComplete(const ScaleTaskState& s) override{GomTaskBaseState b{};CopyPrefix(b,s,sizeof(b));return GomTaskBase::IsDurationComplete(b);}
    bool DurationComplete(const ChangeColorTaskState& s) override{GomTaskBaseState b{};CopyPrefix(b,s,sizeof(b));return GomTaskBase::IsDurationComplete(b);}
    int RandomInt() override{Require(bool(random),"effect random service is not bound");const auto r=random();Require(r>=0 && r<=32767,"invalid effect random15 value");return r;}
    void SetColorFacetEnabled(unsigned f,std::uint8_t mode) override{Target(f).SetColorMode(mode);}
    void ReadColor(unsigned f,std::array<std::uint8_t,4>& v) override{v=Target(f).Color();}
    unsigned WriteColor(unsigned f,const std::array<std::uint8_t,4>& v) override{Target(f).SetColor(v);return 1;}
};
EffectTasks::EffectTasks(std::vector<EffectTaskBinding> b,std::function<int()> r,bool waiting):impl_(std::make_unique<Impl>(std::move(b),std::move(r))){impl_->Initialize();if(waiting)BeginWaiting();}
EffectTasks::~EffectTasks()=default;
void EffectTasks::Advance(float elapsed){Require(std::isfinite(elapsed) && elapsed>=0,"invalid effect task delta");if(Finished())return;(void)impl_->container.Update(impl_->queue,elapsed);impl_->seconds+=elapsed;++impl_->frames;}
void EffectTasks::Stop(){(void)impl_->container.CleanupAndReady(impl_->queue);}
void EffectTasks::BeginWaiting(){(void)impl_->container.BeginWaiting(impl_->queue);}
bool EffectTasks::Finished() const{return impl_->queue.ready!=0;}
void EffectTasks::WriteReport(std::ostream& out) const{
    const auto& s=*impl_;out<<std::setprecision(9)<<"{\"finished\":"<<(Finished()?"true":"false")<<",\"seconds\":"<<s.seconds<<",\"frames\":"<<s.frames<<",\"started\":"<<s.started<<",\"completed\":"<<s.completed<<",\"deleted\":"<<s.deleted<<",\"tasks\":[";
    for(std::size_t i=0;i<s.entries.size();++i){if(i)out<<',';const auto& e=s.entries[i];const auto& b=e.image.base;out<<"{\"name\":"<<std::quoted(e.name)<<",\"phase\":"<<b.phase<<",\"wait_mode\":"<<b.wait_mode<<",\"wait_time\":"<<b.wait_time<<",\"expect_time\":"<<b.expect_time<<",\"run_elapsed\":"<<b.run_elapsed<<",\"finish_latch\":"<<b.finish_latch<<",\"live\":"<<(e.live?"true":"false")<<'}';}out<<"]}";
}
}
