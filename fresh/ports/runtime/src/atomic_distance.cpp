#include "pal4/portable/atomic_distance.h"
#include "pal4/rw_plugin_8450.h"
#include "pal4/script_builtin_forwarders.h"
#include "pal4/global_data_settings.h"
#include "pal4/native_game_vector_math.h"
#include <bit>
#include <cmath>
#include <map>
#include <ostream>
#include <stdexcept>

namespace pal4::portable {
namespace {
void Require(bool valid,const char* message){if(!valid)throw std::logic_error(message);}
constexpr pal4::RwPlugin8450State layout{16,96};
constexpr unsigned engine_base=0x1000,camera_base=0x2000,camera_matrix=0x4000,clump=0x8000;
constexpr unsigned atomic_base=0x10000,frame_base=0x200000,geometry_base=0x400000,material_base=0x600000,list_base=0x800000;
struct EngineWords {std::array<unsigned,2> words{};unsigned changes{},scale{};};
unsigned Atomic(unsigned i){return atomic_base+128*i;}
unsigned Geometry(unsigned i){return geometry_base+64*i;}
unsigned Material(unsigned i){return material_base+8*i;}
unsigned Frame(unsigned i){return frame_base+64*i;}
Matrix Multiply(const Matrix& a,const Matrix& b){Matrix m{};for(unsigned c=0;c<4;++c)for(unsigned r=0;r<4;++r)for(unsigned k=0;k<4;++k)m[c*4+r]+=a[k*4+r]*b[c*4+k];return m;}
class Runtime:public pal4::IRwPlugin8450Runtime,public pal4::IScriptCameraDistanceRuntime {
public:
    EngineWords& engine;
    const Actor* actor{};
    std::map<unsigned,unsigned> memory;
    std::vector<Matrix> transforms;
    std::array<float,3> camera{};
    std::vector<DrawAppearance> draws;
    unsigned frames{},callbacks{},culled{},faded{};
    explicit Runtime(EngineWords& state):engine(state){}
    std::int32_t RegisterEngine(unsigned,unsigned,pal4::RwPlugin8450Callback,pal4::RwPlugin8450Callback) override{throw std::logic_error("static portable plugin layout");}
    unsigned EngineBase() override{return engine_base;}
    // Registration is a native RW lifecycle operation, not an emulated engine.
    std::int32_t RegisterAtomic(unsigned,unsigned,pal4::RwPlugin8450Callback,pal4::RwPlugin8450Callback,pal4::RwPlugin8450Callback) override{throw std::logic_error("static portable plugin layout");}
    std::int32_t RegisterAtomicStream(unsigned,pal4::RwPlugin8450Callback,pal4::RwPlugin8450Callback,pal4::RwPlugin8450Callback) override{throw std::logic_error("static portable plugin layout");}
    unsigned ReadWord(unsigned address) override {
        if(address==engine_base+16 || address==engine_base+20)return engine.words[(address-engine_base-16)/4];
        if(address==engine_base)return camera_base;
        if(address==camera_base+4)return camera_matrix;
        if(address>=camera_matrix+48 && address<camera_matrix+60 && address%4==0)return std::bit_cast<unsigned>(camera[(address-camera_matrix-48)/4]);
        const auto found=memory.find(address);Require(found!=memory.end(),"unbound distance plugin read");return found->second;
    }
    void WriteWord(unsigned address,unsigned value) override {
        if(address==engine_base+16 || address==engine_base+20){engine.words[(address-engine_base-16)/4]=value;return;}
        const auto found=memory.find(address);Require(found!=memory.end(),"unbound distance plugin write");found->second=value;
    }
    unsigned SetDistanceOptimization(unsigned value) override{return pal4::RwPlugin8450::SetEngineInt(value,layout,*this);}
    unsigned RenderCallbackAddress() override{return 2;}
    unsigned DefaultRenderCallbackAddress() override{return 1;}
    unsigned ForAllAtomics(unsigned object,pal4::RwPlugin8450Callback callback,unsigned) override {
        Require(object==clump && actor,"invalid distance clump");
        for(unsigned i=0;i<actor->atomic_spheres.size();++i) {
            if(callback==pal4::RwPlugin8450Callback::ProcessAtomic)(void)pal4::RwPlugin8450::ProcessAtomic(Atomic(i),layout,*this);
            else throw std::logic_error("unbound distance traversal");
        }
        return object;
    }
    void ForAllAtomicsWithOutput(unsigned,pal4::RwPlugin8450Callback,unsigned&) override{throw std::logic_error("unbound distance query");}
    void ResyncAtomic(unsigned) override{throw std::logic_error("unbound morph interpolation");}
    unsigned ObjectTransform(unsigned object) override{return object;}
    std::array<float,3> CalculatePosition(unsigned center,unsigned transform) override {
        Require(actor && transform>=frame_base && (transform-frame_base)%64==0,"invalid distance frame");
        const auto& matrix=transforms.at((transform-frame_base)/64);std::array<float,3> result{};
        const std::array<float,3> point{std::bit_cast<float>(ReadWord(center)),std::bit_cast<float>(ReadWord(center+4)),std::bit_cast<float>(ReadWord(center+8))};
        for(unsigned r=0;r<3;++r)result[r]=matrix[r]*point[0]+matrix[4+r]*point[1]+matrix[8+r]*point[2]+matrix[12+r];
        return result;
    }
    double ApproximateMagnitude(const std::array<float,3>& delta) override{return pal4::NativeGameApproximateMagnitude(delta);}
    std::int32_t FloatToLong(double value) override{Require(std::isfinite(value) && value>=0 && value<256,"invalid distance intensity");return static_cast<std::int32_t>(value);}
    void InvokeCallback(unsigned callback,unsigned atomic) override {
        Require(callback==1 && actor,"invalid distance render callback");++callbacks;
        const auto index=(atomic-atomic_base)/128;
        const auto& bindings=*actor->render_bindings;
        for(std::size_t i=0;i<bindings.draws.size();++i)if(bindings.draws[i].atomic==index) {
            auto& draw=draws[i];draw.visible=true;
            const auto slot=bindings.draws[i].material_slot;
            const auto geometry=bindings.atomics[index].geometry;
            const auto rgba=ReadWord(Material(bindings.geometry_materials[geometry][slot])+4);
            for(unsigned c=0;c<4;++c)draw.color[c]=float((rgba>>(8*c))&255u)/255.f;
            if(draw.color[3]<1)++faded;
        }
    }
    void Bind(const Actor& value) {
        Require(value.render_bindings.has_value(),"distance plugin requires P4AR v3 render bindings");actor=&value;
        const auto& bindings=*actor->render_bindings;
        Require(bindings.atomics.size()==actor->atomic_spheres.size() && bindings.draws.size()==actor->mesh.draws.size(),"invalid distance bindings");
        draws.resize(bindings.draws.size());
        for(unsigned i=0;i<bindings.materials.size();++i)memory[Material(i)+4]=std::bit_cast<unsigned>(bindings.materials[i]);
        unsigned next_list=list_base;
        for(unsigned i=0;i<bindings.geometry_materials.size();++i) {
            memory[Geometry(i)+8]=0;memory[Geometry(i)+32]=next_list;
            memory[Geometry(i)+36]=static_cast<unsigned>(bindings.geometry_materials[i].size());
            for(auto material:bindings.geometry_materials[i]){Require(material<bindings.materials.size(),"invalid distance material");memory[next_list]=Material(material);next_list+=4;}
        }
        for(unsigned i=0;i<bindings.atomics.size();++i) {
            const auto& atomic=bindings.atomics[i];const auto& sphere=actor->atomic_spheres[i];
            Require(atomic.geometry<bindings.geometry_materials.size() && sphere.frame<actor->frames.size(),"invalid distance geometry/frame");
            memory[Atomic(i)+4]=Frame(sphere.frame);memory[Atomic(i)+24]=Geometry(atomic.geometry);
            memory[Atomic(i)+72]=1;memory[Atomic(i)+76]=0;
            for(unsigned c=0;c<4;++c)memory[Atomic(i)+28+4*c]=std::bit_cast<unsigned>(sphere.sphere[c]);
            for(unsigned c=0;c<3;++c)memory[Atomic(i)+96+4*c]=0;
            (void)pal4::RwPlugin8450::AtomicConstruct(Atomic(i),layout,*this);
        }
        for(const auto& draw:bindings.draws)Require(draw.atomic<bindings.atomics.size() && draw.material_slot<bindings.geometry_materials[bindings.atomics[draw.atomic].geometry].size(),"invalid distance draw");
        (void)pal4::RwPlugin8450::ProcessClump(clump,*this);
    }
};
}
struct DistanceOptimization::Impl {EngineWords state;Runtime runtime{state};};
DistanceOptimization::DistanceOptimization():impl_(std::make_unique<Impl>()) {
    (void)pal4::RwPlugin8450::EngineConstruct(engine_base,layout,impl_->runtime);
    SetScaleOption(pal4::GlobalDataSettings::DefaultState().words[3]);
}
DistanceOptimization::~DistanceOptimization()=default;
void DistanceOptimization::SetEnabled(bool enabled){(void)pal4::ScriptCameraDistanceOptimization::Set(enabled,impl_->runtime);++impl_->state.changes;}
void DistanceOptimization::SetScaleOption(unsigned option){impl_->state.scale=option;(void)pal4::RwPlugin8450::SetEngineFloat(pal4::GlobalDataSettings::ScaleDistance(option),layout,impl_->runtime);}
bool DistanceOptimization::Enabled() const{return impl_->state.words[1]!=0;}
float DistanceOptimization::Threshold() const{return std::bit_cast<float>(impl_->state.words[0]);}
void DistanceOptimization::WriteReport(std::ostream& out) const{out<<"{\"enabled\":"<<(Enabled()?"true":"false")<<",\"threshold\":"<<Threshold()<<",\"scale_option\":"<<impl_->state.scale<<",\"changes\":"<<impl_->state.changes<<'}';}
struct AtomicDistanceInstance::Impl {Runtime runtime;Impl(const Actor& actor,EngineWords& state):runtime(state){runtime.Bind(actor);}};
AtomicDistanceInstance::AtomicDistanceInstance(const Actor& actor,DistanceOptimization& engine):impl_(std::make_unique<Impl>(actor,engine.impl_->state)){}
AtomicDistanceInstance::~AtomicDistanceInstance()=default;
std::span<const DrawAppearance> AtomicDistanceInstance::Evaluate(std::span<const Matrix> frames,const Matrix& model,std::array<float,3> camera) {
    auto& r=impl_->runtime;Require(frames.size()==r.actor->frames.size(),"distance pose frame count mismatch");
    for(auto v:camera)Require(std::isfinite(v),"invalid distance camera");
    for(auto v:model)Require(std::isfinite(v),"invalid distance model");
    r.camera=camera;r.transforms.clear();
    for(const auto& frame:frames){for(auto v:frame)Require(std::isfinite(v),"invalid distance pose");r.transforms.push_back(Multiply(model,frame));}
    for(auto& draw:r.draws)draw.visible=false;
    r.callbacks=r.culled=r.faded=0;
    for(unsigned i=0;i<r.actor->render_bindings->atomics.size();++i)if(r.actor->render_bindings->atomics[i].flags&4u) {
        const auto before=r.callbacks;(void)pal4::RwPlugin8450::RenderAtomic(Atomic(i),layout,r);if(r.callbacks==before)++r.culled;
    }
    ++r.frames;return r.draws;
}
bool AtomicDistanceInstance::CollisionAllowed(std::size_t atomic) const {
    auto& r=impl_->runtime;Require(atomic<r.actor->atomic_spheres.size(),"invalid collision Atomic index");
    return pal4::RwPlugin8450::QueryDirect(Atomic(static_cast<unsigned>(atomic)),layout,r)!=0;
}
void AtomicDistanceInstance::WriteReport(std::ostream& out) const {
    const auto& r=impl_->runtime;out<<"{\"frames\":"<<r.frames<<",\"atomics\":"<<r.actor->atomic_spheres.size()<<",\"callbacks\":"<<r.callbacks<<",\"culled\":"<<r.culled<<",\"faded_draws\":"<<r.faded<<",\"draws\":[";
    for(std::size_t i=0;i<r.draws.size();++i){if(i)out<<',';out<<"{\"visible\":"<<(r.draws[i].visible?"true":"false")<<",\"rgba\":[";for(unsigned c=0;c<4;++c){if(c)out<<',';out<<r.draws[i].color[c];}out<<"]}";}out<<"]}";
}
}
