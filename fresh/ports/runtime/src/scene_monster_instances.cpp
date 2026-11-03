#include "pal4/portable/scene_monster_instances.h"
#include "pal4/portable/world.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <map>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <utility>

namespace pal4::portable {
namespace {
void Require(bool value,const char* message){if(!value)throw std::logic_error(message);}
void CheckSource(const SceneMonster& record){
    Require(!record.name.empty() && record.name.size()<=4096 && record.name.find('\0')==std::string::npos,"invalid monster name");
    Require(!record.resource.empty() && record.resource.size()<=64 &&
        record.resource.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-")==std::string::npos,"invalid monster model ID");
    Require(record.fields[4]<=0x7fffffffu && record.fields[4]==record.values.size(),"invalid monster record values");
    for(float f:record.position)Require(std::isfinite(f),"invalid monster position");
    for(float f:record.rotation)Require(std::isfinite(f),"invalid monster rotation");
}
}
struct SceneMonsterInstances::Impl {
    struct ModelData {Actor actor;std::array<float,4> sphere;std::optional<ActorLocalBox> box;};
    struct Instance {SceneMonster source;MonsterBehaviorDefinition behavior;SceneMonsterTransform motion;const ModelData* model{};std::unique_ptr<ActorPlayback> playback;TransformCommit commit;std::uint64_t binding{};bool committing{};};
    std::uint64_t generation,next_binding=1;
    // map元素地址稳定；实例先于模型销毁，ActorPlayback的借用不会悬空。
    std::map<std::string,ModelData,std::less<>> models;
    std::map<std::string,std::uint32_t,std::less<>> names;
    std::vector<Instance> instances;
    explicit Impl(std::uint64_t value):generation(value){}
    const Instance& Get(SceneMonsterHandle handle) const {
        Require(handle.generation==generation && handle.index<instances.size(),"stale or invalid monster handle");
        return instances[handle.index];
    }
    Instance& Mutable(SceneMonsterHandle h){(void)Get(h);return instances[h.index];}
    static void Validate(const SceneMonsterTransform& value){for(const auto& vector:{value.position,value.rotation})for(float v:vector)Require(std::isfinite(v),"non-finite monster transform");}
    void Commit(SceneMonsterHandle h,SceneMonsterTransform value,MonsterTransformChange change){
        auto& instance=Mutable(h);Require(!instance.committing,"monster transform commit is not reentrant");Validate(value);
        instance.committing=true;struct Guard{bool& busy;~Guard(){busy=false;}} guard{instance.committing};
        if(instance.commit)instance.commit(value,change);
        Validate(value);Require(change==MonsterTransformChange::Position?value.rotation==instance.motion.rotation:value.position==instance.motion.position,"transform commit changed an unrelated field");
        instance.motion=value;
    }
};
SceneMonsterInstances::SceneMonsterInstances(std::uint64_t generation,std::span<const SceneMonster> records,
    const ModelLoader& load,ActorMemberReader read):impl_(std::make_unique<Impl>(generation)){
    auto& s=*impl_;
    Require(generation!=0 && records.size()<=65535 && bool(load) && bool(read),"invalid monster instance resources");
    for(std::size_t i=0;i<records.size();++i){
        CheckSource(records[i]);Require(s.names.emplace(records[i].name,static_cast<std::uint32_t>(i)).second,"duplicate monster name");
    }
    s.instances.reserve(records.size());
    for(const auto& record:records){
        auto model=s.models.find(record.resource);
        if(model==s.models.end()){
            auto actor=load(record.resource);
            Require(!actor.frames.empty() && !actor.mesh.vertices.empty() && !actor.clips.empty(),"incomplete monster model");
            const auto sphere=ActorBindSphere(actor);Require(sphere.has_value(),"monster model has no original bounds");
            const auto box=ActorBindBox(actor);model=s.models.emplace(record.resource,Impl::ModelData{std::move(actor),*sphere,box}).first;
        }
        Impl::Instance instance;instance.source=record;instance.behavior=ReadMonsterBehaviorDefinition(record.behavior);instance.model=&model->second;
        instance.motion={record.position,record.rotation};
        instance.playback=std::make_unique<ActorPlayback>(model->second.actor,record.resource,read);
        // 不猜测行为选择的初始动作；尚未选择时沿用ActorPlayback原绑定姿态。
        s.instances.push_back(std::move(instance));
    }
}
SceneMonsterInstances::~SceneMonsterInstances()=default;
std::size_t SceneMonsterInstances::Size() const{return impl_->instances.size();}
std::size_t SceneMonsterInstances::ModelCount() const{return impl_->models.size();}
std::vector<std::size_t> SceneMonsterInstances::OriginalTraversalOrder() const {
    std::vector<std::size_t> order(Size());
    for(std::size_t i=0;i<order.size();++i)order[i]=i;
    std::sort(order.begin(),order.end(),[this](std::size_t left,std::size_t right){
        const auto& lhs=impl_->instances[left].source.name;
        const auto& rhs=impl_->instances[right].source.name;
        const auto common=std::min(lhs.size(),rhs.size());
        const int compared=std::memcmp(lhs.data(),rhs.data(),common);
        return compared<0 || (compared==0 && lhs.size()<rhs.size());
    });
    return order;
}
SceneMonsterHandle SceneMonsterInstances::Handle(std::size_t index) const {
    Require(index<Size(),"monster instance index out of range");return {impl_->generation,static_cast<std::uint32_t>(index)};
}
std::optional<SceneMonsterHandle> SceneMonsterInstances::Find(std::string_view name) const {
    const auto found=impl_->names.find(name);if(found==impl_->names.end())return {};
    return SceneMonsterHandle{impl_->generation,found->second};
}
const SceneMonster& SceneMonsterInstances::Source(SceneMonsterHandle h) const{return impl_->Get(h).source;}
const MonsterBehaviorDefinition& SceneMonsterInstances::Behavior(SceneMonsterHandle h) const{return impl_->Get(h).behavior;}
const Actor& SceneMonsterInstances::Model(SceneMonsterHandle h) const{return impl_->Get(h).model->actor;}
std::array<float,4> SceneMonsterInstances::LocalSphere(SceneMonsterHandle h) const{return impl_->Get(h).model->sphere;}
std::optional<ActorLocalBox> SceneMonsterInstances::LocalBox(SceneMonsterHandle h) const{return impl_->Get(h).model->box;}
Matrix SceneMonsterInstances::InstanceTransform(SceneMonsterHandle h) const {
    const auto& motion=impl_->Get(h).motion;return InstanceMatrix(motion.position,motion.rotation);
}
SceneMonsterTransform SceneMonsterInstances::MotionTransform(SceneMonsterHandle h) const{return impl_->Get(h).motion;}
std::uint64_t SceneMonsterInstances::BindTransform(SceneMonsterHandle h,const SceneMonsterTransform& initial,TransformCommit commit){
    auto& s=*impl_;auto& instance=s.Mutable(h);Require(!instance.committing && !instance.commit && bool(commit),"monster transform resource owner is already bound or invalid");Impl::Validate(initial);
    Require(s.next_binding<std::numeric_limits<std::uint64_t>::max(),"monster transform binding token exhausted");
    instance.commit=std::move(commit);instance.motion=initial;instance.binding=s.next_binding++;return instance.binding;
}
void SceneMonsterInstances::UnbindTransform(SceneMonsterHandle h,std::uint64_t binding){
    auto& instance=impl_->Mutable(h);Require(!instance.committing && binding && instance.binding==binding && bool(instance.commit),"invalid monster transform binding release");TransformCommit previous;previous.swap(instance.commit);instance.binding=0;
}
bool SceneMonsterInstances::TransformBound(SceneMonsterHandle h) const{return bool(impl_->Get(h).commit);}
bool SceneMonsterInstances::OwnsTransformBinding(SceneMonsterHandle h,std::uint64_t binding) const{return binding && impl_->Get(h).binding==binding && bool(impl_->Get(h).commit);}
void SceneMonsterInstances::SetMotionPosition(SceneMonsterHandle h,const std::array<float,3>& p){auto value=MotionTransform(h);value.position=p;impl_->Commit(h,value,MonsterTransformChange::Position);}
void SceneMonsterInstances::SetMotionRotation(SceneMonsterHandle h,const std::array<float,3>& p){auto value=MotionTransform(h);value.rotation=p;impl_->Commit(h,value,MonsterTransformChange::Rotation);}
bool SceneMonsterInstances::SelectAction(SceneMonsterHandle h,std::string_view action){return impl_->Get(h).playback->Select(action);}
void SceneMonsterInstances::AdvanceAnimation(SceneMonsterHandle h,float seconds){impl_->Get(h).playback->Advance(seconds);}
std::string_view SceneMonsterInstances::Action(SceneMonsterHandle h) const{return impl_->Get(h).playback->Action();}
float SceneMonsterInstances::AnimationTime(SceneMonsterHandle h) const{return impl_->Get(h).playback->Time();}
ActorPlayback& SceneMonsterInstances::Playback(SceneMonsterHandle h){return *impl_->Get(h).playback;}
void SceneMonsterInstances::Sample(SceneMonsterHandle h,std::vector<Vertex>& output){impl_->Get(h).playback->Sample(output);}
void SceneMonsterInstances::WriteReport(std::ostream& out) const {
    out<<std::setprecision(9)<<"{\"generation\":"<<impl_->generation<<",\"models\":"<<ModelCount()
        <<",\"world_registration_bound\":false,\"behavior_bound\":false,\"instances\":[";
    for(std::size_t i=0;i<Size();++i){
        if(i)out<<',';
        const auto h=Handle(i);const auto& r=Source(h);const auto sphere=LocalSphere(h);const auto motion=MotionTransform(h);
        out<<"{\"index\":"<<i<<",\"name\":"<<std::quoted(r.name)<<",\"model\":"<<std::quoted(r.resource)
            <<",\"source_state\":"<<r.fields[0]<<",\"position\":["<<r.position[0]<<','<<r.position[1]<<','<<r.position[2]
            <<"],\"rotation\":["<<r.rotation[0]<<','<<r.rotation[1]<<','<<r.rotation[2]
            <<"],\"motion_position\":["<<motion.position[0]<<','<<motion.position[1]<<','<<motion.position[2]
            <<"],\"motion_rotation\":["<<motion.rotation[0]<<','<<motion.rotation[1]<<','<<motion.rotation[2]
            <<"],\"local_sphere\":["<<sphere[0]<<','<<sphere[1]<<','<<sphere[2]<<','<<sphere[3]
            <<"],\"local_box\":";
        if(const auto box=LocalBox(h)){out<<'[';bool first=true;for(const auto& vector:{box->maximum,box->minimum})for(float value:vector){if(!first)out<<',';first=false;out<<value;}out<<']';}else out<<"null";
        out<<",\"action\":"<<std::quoted(std::string(Action(h)))<<",\"animation_time\":"<<AnimationTime(h)<<",\"behavior_definition\":";WriteMonsterBehaviorDefinition(out,Behavior(h));out<<'}';
    }
    out<<"]}";Require(bool(out),"cannot write monster instance report");
}
} // namespace pal4::portable
