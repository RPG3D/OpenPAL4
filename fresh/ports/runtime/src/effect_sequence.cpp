#include "pal4/portable/effect_sequence.h"
#include "pal4/effect_system_factory_registry.h"
#include "pal4/clump_object_config.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace pal4::portable {
namespace {
void Require(bool v,const char* message){if(!v)throw std::logic_error(message);}
bool LoadDummyClump(const BinaryXmlNode& node){
    struct Config final:pal4::ClumpObjectConfigRuntimeStub {
        const BinaryXmlNode& node;explicit Config(const BinaryXmlNode& n):node(n){}
        std::int32_t GetInt(unsigned,std::string_view key,std::int32_t fallback) override {
            const auto* a=node.FindAttribute(key);return a?std::bit_cast<std::int32_t>(a->ToInt(std::bit_cast<unsigned>(fallback))):fallback;
        }
        unsigned CreateManager(unsigned) override{throw std::logic_error("non-dummy effect ClumpObject is not bound");}
    } config(node);
    pal4::ClumpObjectState state{};
    Require(pal4::ClumpObject::LoadConfiguration(state,1,config)!=0,"effect dummy configuration failed");
    return ((state.words[1]>>8)&255)!=0;
}
}
struct EffectSequence::Impl {
    struct Object {
        std::string name;
        bool dummy{};
        std::unique_ptr<EffectParticle> particle;
        std::unique_ptr<EffectCylinder> cylinder;
        EffectVisualTarget& Target(){Require(bool(particle)||bool(cylinder),"effect object has been released");return particle?static_cast<EffectVisualTarget&>(*particle):static_cast<EffectVisualTarget&>(*cylinder);}
    };
    EffectPlan plan;ParticleRandom& random;EffectTextures& textures;ParticleCounts& particle_counts;EffectObjectCounts& cylinder_counts;std::vector<Object> objects;
    std::unique_ptr<EffectTasks> tasks; // 先销毁借用对象的任务，再销毁实际对象。
    std::string last_tasks="{\"finished\":true,\"tasks\":[]}";
    float seconds{};unsigned frames{},runs{};bool stopped{};
    Impl(EffectPlan source,EffectTextures& t,ParticleRandom& rng,ParticleCounts& pc,EffectObjectCounts& cc):plan(std::move(source)),random(rng),textures(t),particle_counts(pc),cylinder_counts(cc){
        Require(!plan.object_ids.empty() && plan.object_ids.size()<=1024,"effect object count limit");
        Require(plan.task_ids.size()==plan.task_parameters.size(),"effect task/object reference count mismatch");
        // 原激活链遍历ObjectList，不遍历模板字典。未支持但未引用的模板可留在只读文档中。
        objects.resize(plan.object_ids.size());for(std::size_t i=0;i<objects.size();++i){objects[i].name=plan.object_ids[i];Create(i);}
    }
    void Create(std::size_t i){
            auto& object=objects.at(i);if(object.particle || object.cylinder || object.dummy)return;const auto& id=object.name;
            const auto found=plan.templates.find(id);Require(found!=plan.templates.end(),"referenced effect template is missing");
            const auto& node=found->second;const auto* attribute=node.FindAttribute("Attr_Class_Name");
            const auto* name=attribute?std::get_if<std::string>(&attribute->value):nullptr;Require(name!=nullptr,"effect template class is missing");
            const auto& factories=pal4::EffectSystemFactoryRegistry::kFactories;
            const auto factory=std::find_if(factories.begin(),factories.end(),[&](const auto& f){return f.name==*name;});
            Require(factory!=factories.end(),"effect class is not registered");
            if(factory->creator==0x63e490)object.particle=std::make_unique<EffectParticle>(node,textures,random,particle_counts);
            else if(factory->creator==0x63e2b0)object.cylinder=std::make_unique<EffectCylinder>(node,textures,cylinder_counts);
            else if(*name=="ClumpObject")object.dummy=LoadDummyClump(node);
            else throw std::logic_error("portable effect object factory is not bound: "+*name);
    }
    void Begin(bool waiting){
        tasks.reset();
        std::vector<EffectTaskBinding> bindings;bindings.reserve(plan.task_ids.size());
        for(std::size_t i=0;i<plan.task_ids.size();++i){const auto target=plan.task_ids[i];Require(target>=0 && static_cast<std::size_t>(target)<objects.size(),"effect task references an invalid object");auto& object=objects[static_cast<std::size_t>(target)];bindings.push_back({plan.task_parameters[i],object.dummy?nullptr:&object.Target(),object.dummy});}
        tasks=std::make_unique<EffectTasks>(std::move(bindings),[this]{return random.Next15();},waiting);seconds=0;frames=0;stopped=false;++runs;
    }
};
EffectSequence::EffectSequence(const EffectDocument& d,EffectTextures& t,ParticleRandom& r,ParticleCounts& pc,EffectObjectCounts& cc,bool start):EffectSequence(ReadEffectPlan(d),t,r,pc,cc,start){}
EffectSequence::EffectSequence(EffectPlan p,EffectTextures& t,ParticleRandom& r,ParticleCounts& pc,EffectObjectCounts& cc,bool start):impl_(std::make_unique<Impl>(std::move(p),t,r,pc,cc)){if(start)Restart();}
EffectSequence::~EffectSequence()=default;
void EffectSequence::Advance(float elapsed){Require(std::isfinite(elapsed) && elapsed>=0,"invalid effect sequence delta");if(impl_->stopped || TasksFinished())return;AdvanceTasks(elapsed);AdvanceObjects(elapsed);}
void EffectSequence::Restart(){for(std::size_t i=0;i<ObjectCount();++i){ResetObject(i);PreRenderObject(i);}BeginTasks(true);}
void EffectSequence::Stop(){ClearTasks();for(auto& o:impl_->objects)if(!o.dummy)o.Target().Show(false);impl_->stopped=true;}
void EffectSequence::BeginTasks(bool waiting){impl_->Begin(waiting);}
void EffectSequence::BeginWaiting(){Require(HasTasks(),"effect task container is missing");impl_->tasks->BeginWaiting();}
void EffectSequence::AdvanceTasks(float elapsed){Require(HasTasks() && std::isfinite(elapsed) && elapsed>=0,"invalid effect task frame");impl_->tasks->Advance(elapsed);impl_->seconds+=elapsed;++impl_->frames;}
void EffectSequence::AdvanceObjects(double elapsed){Require(std::isfinite(elapsed) && elapsed>=0,"invalid effect object frame");for(std::size_t i=0;i<ObjectCount();++i)if(HasObject(i))AdvanceObject(i,elapsed);}
void EffectSequence::AdvanceObject(std::size_t i,double elapsed){Require(HasObject(i) && std::isfinite(elapsed) && elapsed>=0,"invalid effect object frame");auto& o=impl_->objects.at(i);if(o.dummy)return;if(o.particle)o.particle->Advance(elapsed);else o.cylinder->Advance(elapsed);}
void EffectSequence::EnsureObject(std::size_t i){impl_->Create(i);}
void EffectSequence::ReleaseObject(std::size_t i){Require(!HasTasks(),"destroy effect tasks before releasing their target");auto& o=impl_->objects.at(i);o.particle.reset();o.cylinder.reset();o.dummy=false;}
bool EffectSequence::HasObject(std::size_t i) const{const auto& o=impl_->objects.at(i);return bool(o.particle)||bool(o.cylinder)||o.dummy;}
void EffectSequence::ClearTasks(){if(impl_->tasks)impl_->tasks->Stop();}
void EffectSequence::DestroyTasks(){if(!impl_->tasks)return;std::ostringstream report;impl_->tasks->WriteReport(report);impl_->last_tasks=report.str();impl_->tasks.reset();}
bool EffectSequence::HasTasks() const{return bool(impl_->tasks);}
bool EffectSequence::TasksFinished() const{return !impl_->tasks || impl_->tasks->Finished();}
void EffectSequence::ResetObject(std::size_t i){Require(HasObject(i),"effect object has been released");auto& o=impl_->objects.at(i);if(o.dummy)return;if(o.particle)o.particle->Reset();else o.cylinder->Reset();}
void EffectSequence::PreRenderObject(std::size_t i){if(auto* p=impl_->objects.at(i).particle.get())p->PreRender();}
void EffectSequence::SetObjectMode(std::size_t i,bool mode){Require(HasObject(i),"effect object has been released");auto& o=impl_->objects.at(i);if(o.dummy)return;if(o.particle)o.particle->SetTimedMode(mode);else o.cylinder->SetTimedMode(mode);}
void EffectSequence::HideObject(std::size_t i){if(!IsDummy(i))impl_->objects.at(i).Target().Show(false);}
bool EffectSequence::IsDummy(std::size_t i) const{return impl_->objects.at(i).dummy;}
double EffectSequence::ObjectDuration(std::size_t i) const{Require(HasObject(i),"effect object has been released");const auto& o=impl_->objects.at(i);if(o.dummy)return 0;return o.particle?o.particle->Duration():o.cylinder->Duration();}
void EffectSequence::SetObjectParentTransform(std::size_t i,const std::array<float,16>& matrix){Require(HasObject(i),"effect object has been released");auto& o=impl_->objects.at(i);if(o.dummy)return;if(o.particle)o.particle->SetParentTransform(matrix);else o.cylinder->SetParentTransform(matrix);}
std::size_t EffectSequence::ObjectCount() const{return impl_->objects.size();}
std::string_view EffectSequence::ObjectName(std::size_t i) const{return impl_->objects.at(i).name;}
const EffectParticle* EffectSequence::Particle(std::size_t i) const{return impl_->objects.at(i).particle.get();}
const EffectCylinder* EffectSequence::Cylinder(std::size_t i) const{return impl_->objects.at(i).cylinder.get();}
void EffectSequence::WriteReport(std::ostream& out) const{
    const auto& s=*impl_;out<<std::setprecision(9)<<"{\"seconds\":"<<s.seconds<<",\"frames\":"<<s.frames<<",\"runs\":"<<s.runs<<",\"stopped\":"<<(s.stopped?"true":"false")<<",\"scheduler\":";if(s.tasks)s.tasks->WriteReport(out);else out<<s.last_tasks;out<<",\"objects\":[";
    for(std::size_t i=0;i<s.objects.size();++i){if(i)out<<',';const auto& o=s.objects[i];out<<"{\"name\":"<<std::quoted(o.name)<<",\"state\":";if(o.particle)o.particle->WriteReport(out);else if(o.cylinder)o.cylinder->WriteReport(out);else if(o.dummy)out<<"{\"dummy\":true}";else out<<"null";out<<'}';}out<<"]}";
}
}
