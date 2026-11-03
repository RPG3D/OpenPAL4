#include "pal4/portable/scene_npcs.h"
#include "pal4/portable/arena.h"
#include "pal4/portable/collision_surface.h"
#include "pal4/script_entity_callbacks.h"
#include "pal4/game_arena_script_character_callbacks.h"
#include "pal4/game_arena_npc_item_callbacks.h"
#include "pal4/base_object_activity.h"
#include "pal4/base_object_property_callbacks.h"
#include "pal4/collision_actor.h"
#include "pal4/npc_manager_closest_finder.h"
#include "pal4/game_vector_math.h"
#include <numeric>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cmath>
#include <iomanip>
#include <ostream>
#include <stdexcept>
namespace pal4::portable {
namespace {void Require(bool value,const char* message){if(!value)throw std::logic_error(message);}}
struct SceneNpcs::Impl final:pal4::IScriptNpcVisibilityRuntime,pal4::IGameArenaScriptCharacterRuntime,
    pal4::IGameArenaNpcItemRuntime,pal4::IBaseObjectPropertyCallbacksRuntime,pal4::ICollisionActorPlacementRuntime {
    struct Object {pal4::BaseObjectState base;pal4::CollisionActorState collision;std::unique_ptr<ActorPlayback> playback;unsigned enters{},leaves{},samples{};double pose_checksum{};float sample_time{};};
    const ArenaResources& arena;CollisionSurface surface;std::vector<SceneNpcState> states;std::vector<Object> objects;
    bool script_motion_bound{};unsigned calls{},matched{},updates{};std::size_t current{};
    static constexpr std::uint32_t world=0x2000,manager=0x3000,adapter=0x4000,tree=0x5000;
    explicit Impl(const ArenaResources& a):arena(a),surface(a.collision.floor){}
    static std::uint32_t Handle(std::size_t i){return 0x100000+static_cast<std::uint32_t>(i)*0x1000;}
    std::size_t Index(std::uint32_t value) const {Require(value>=0x100000,"invalid NPC handle");const auto i=(value-0x100000)/0x1000;Require(i<states.size(),"NPC handle outside collection");return i;}
    std::size_t Clump(std::uint32_t value) const {Require(value>=0x10000 && value-0x10000<states.size(),"invalid NPC clump");return value-0x10000;}
    std::uint32_t ReadWord(std::uint32_t address) override {
        if(address==world+48)return 1;
        if(address==world+16)return 0x6000;
        if(address==world+404)return surface.Size()?tree:0;
        if(address==adapter)return adapter+4;
        const auto i=Index(address);const auto offset=address-Handle(i);
        if(offset==0x14)return 0x10000+static_cast<std::uint32_t>(i);
        Require(offset<pal4::BaseObjectState::kSize && offset%4==0,"invalid NPC field");return objects[i].base.Read(offset);
    }
    std::uint32_t NpcManager() override{return manager;}
    std::uint32_t FindNpc(std::uint32_t owner,std::string_view name) override {
        Require(owner==manager,"invalid NPC manager");for(std::size_t i=0;i<states.size();++i)if(states[i].name==name)return Handle(i);return 0;
    }
    std::uint32_t ArenaManager() override{return world;}
    void SetStateFlag(std::uint32_t npc,std::uint32_t value) override {
        const auto i=Index(npc);(void)pal4::BaseObjectActivity::SetStateFlag(objects[i].base,value);states[i].active=value;
    }
    std::uint32_t LeaveNpcScript(std::uint32_t owner,std::uint32_t npc) override {
        Require(owner==world,"invalid NPC leave arena");++objects[Index(npc)].leaves;
        return pal4::GameArenaScriptCharacterCallbacks::LeaveWithVirtual68(owner,npc,*this);
    }
    std::uint32_t EnterNpcScript(std::uint32_t owner,std::uint32_t npc) override {
        Require(owner==world,"invalid NPC enter arena");++objects[Index(npc)].enters;
        try{return pal4::GameArenaScriptCharacterCallbacks::Enter(owner,npc,*this);}
        catch(...){(void)pal4::GameArenaScriptCharacterCallbacks::LeaveWithVirtual68(owner,npc,*this);throw;}
    }
    std::uint32_t Call(pal4::GameArenaScriptCharacterCall operation,std::initializer_list<std::uint32_t> args) override {
        using Op=pal4::GameArenaScriptCharacterCall;
        const auto at=[&](std::size_t n){Require(n<args.size(),"NPC callback argument missing");return *(args.begin()+n);};
        if(operation==Op::GetMovementAdapter)return adapter;
        if(operation==Op::GetCharacterProperty)return static_cast<std::uint32_t>(surface.Size());
        if(operation==Op::AddClump || operation==Op::RemoveClump){Require(at(0)==world,"invalid NPC world");states[Clump(at(1))].world_attached=operation==Op::AddClump;return at(1);}
        if(operation==Op::SetClumpProperties || operation==Op::SetClumpRenderCallback){Require(at(0)==adapter+4,"invalid NPC movement adapter");auto& s=states[Clump(at(1))];s.movement_registered=operation==Op::SetClumpProperties;return at(1);}
        if(operation==Op::ProcessClumpAtomics || operation==Op::ProcessAtomics || operation==Op::ProcessAtomicsFinal || operation==Op::ResetAtomicCallbacks || operation==Op::ResetMaterialCallbacks || operation==Op::ProcessClumpAtomicsWrapper){
            auto& s=states[Clump(at(0))];
            if(operation==Op::ProcessClumpAtomics)s.geometry_bound=true;
            if(operation==Op::ProcessAtomics)s.material_bound=true;
            if(operation==Op::ProcessAtomicsFinal)s.render_bound=true;
            if(operation==Op::ResetAtomicCallbacks)s.render_bound=false;
            if(operation==Op::ResetMaterialCallbacks)s.material_bound=false;
            if(operation==Op::ProcessClumpAtomicsWrapper)s.geometry_bound=false;
            return at(0);
        }
        const auto i=Index(at(0));auto& s=states[i];auto& o=objects[i];current=i;
        switch(operation){
        case Op::SetAnimationFlag:Require(at(0)==Handle(i)+0x170 && at(1)==1,"invalid NPC animation flag");if(o.playback)o.playback->SetRestart(true);return 1;
        case Op::RenderCamera:Require(at(1)==1 && at(2)==0x6000,"invalid NPC render camera");s.camera_registered=true;return 1;
        case Op::CleanupCamera:s.camera_registered=false;return 1;
        case Op::GetObjectProperty:return GetProperty(o.base,0x564);
        case Op::SetStateFlag:SetStateFlag(at(0),at(1));return at(1);
        // 未创建武器/附着特效的对象没有这些拥有者；有拥有者时禁止静默略过。
        case Op::CleanupObject:for(auto offset:{0x518u,0x51cu,0x22cu,0x39cu})Require(o.base.Read(offset)==0,"NPC weapon cleanup is not bound");return 0;
        case Op::Virtual68:Require(o.base.Read(0x534)==0,"NPC attached effect cleanup is not bound");return 0;
        case Op::Virtual76:Require(o.base.Read(0x53c)==0,"NPC rotated effect cleanup is not bound");return 0;
        case Op::Virtual72:Require(o.base.Read(0x530)==0,"NPC attached effect setup is not bound");return 0;
        case Op::InitializeProperties:Require(at(1)==tree,"invalid NPC collision tree");return pal4::BaseObjectPropertyCallbacks::Initialize(o.base,tree,*this);
        default:throw std::logic_error("NPC scene operation is not bound");
        }
    }
    std::uint32_t Call(pal4::GameArenaNpcItemCall operation,std::initializer_list<std::uint32_t> args) override {
        using Initial=pal4::GameArenaNpcItemCall;using Script=pal4::GameArenaScriptCharacterCall;
        // 两个已有回调枚举顺序相同，但显式映射以保留各自接口。
        switch(operation){
#define NPC_FORWARD(name) case Initial::name:return Call(Script::name,args)
        NPC_FORWARD(AddClump);NPC_FORWARD(GetMovementAdapter);NPC_FORWARD(SetClumpProperties);NPC_FORWARD(SetAnimationFlag);
        NPC_FORWARD(RenderCamera);NPC_FORWARD(Virtual72);NPC_FORWARD(ProcessClumpAtomics);NPC_FORWARD(ProcessAtomics);NPC_FORWARD(ProcessAtomicsFinal);
        NPC_FORWARD(GetCharacterProperty);NPC_FORWARD(InitializeProperties);
#undef NPC_FORWARD
        default:throw std::logic_error("initial NPC operation is not bound");
        }
    }
    std::uint32_t GetProperty(pal4::BaseObjectState&,std::uint32_t offset) override {Require(offset==0x564,"invalid NPC collision offset");states[current].collision_bound=false;return pal4::CollisionActor::ClearBinding(objects[current].collision);}
    void RegisterCountCallback(pal4::BaseObjectState&,std::uint32_t) override{(void)pal4::CollisionActor::SetPathCallback(objects[current].collision,0x444320);}
    void RegisterByIndexCallback(pal4::BaseObjectState&,std::uint32_t) override{(void)pal4::CollisionActor::SetMovementCallback(objects[current].collision,0x444330);}
    void RegisterByIdCallback(pal4::BaseObjectState&,std::uint32_t) override{(void)pal4::CollisionActor::SetOutputCallback(objects[current].collision,0x7e1840);}
    void RegisterActiveCallback(pal4::BaseObjectState&,std::uint32_t) override{(void)pal4::CollisionActor::SetCallback108(objects[current].collision,0x444390);}
    std::uint32_t SetInitialProperty(pal4::BaseObjectState&,std::uint32_t offset,std::uint32_t owner,std::uint32_t radius,const std::array<std::uint32_t,3>& position,std::uint32_t face,std::uint32_t property) override {
        Require(offset==0x564 && owner==tree,"invalid NPC collision binding");const auto result=pal4::CollisionActor::Place(objects[current].collision,Handle(current)+offset,owner,radius,{std::bit_cast<float>(position[0]),std::bit_cast<float>(position[1]),std::bit_cast<float>(position[2])},face,property,*this);
        states[current].collision_bound=true;return result;
    }
    std::uint32_t SelectFace(std::uint32_t owner,const pal4::D3dTriangleVector3& point) override{Require(owner==tree,"invalid NPC collision selector");return surface.Select({point.x,point.y,point.z});}
    pal4::D3dTriangleVector3 Project(std::uint32_t owner,std::uint32_t face,const pal4::D3dTriangleVector3& point) override{Require(owner==tree,"invalid NPC projection owner");const auto p=surface.Project(face,{point.x,point.y,point.z});return {p[0],p[1],p[2]};}
};
SceneNpcs::SceneNpcs(const ArenaResources& arena,ActorMemberReader read):impl_(std::make_unique<Impl>(arena)) {
    auto& s=*impl_;Require(arena.catalog.npcs.size()<=65535 && bool(read),"invalid NPC resources");s.states.resize(arena.catalog.npcs.size());s.objects.resize(s.states.size());
    for(std::size_t i=0;i<s.states.size();++i){const auto& source=arena.catalog.npcs[i];auto& state=s.states[i];auto& object=s.objects[i];
        state.name=source.name;state.resource=source.resource;state.position=source.position;state.rotation=source.rotation;
        const auto found=std::find_if(arena.actors.begin(),arena.actors.end(),[&](const auto& a){return !a.gob_type && a.name==source.name;});Require(found!=arena.actors.end(),"NPC model is missing");state.actor=static_cast<std::size_t>(found-arena.actors.begin());
        const auto bounds=ActorBindSphere(found->actor);Require(bounds.has_value(),"NPC bounds are missing");object.base.Write(0xdc,std::bit_cast<std::uint32_t>((*bounds)[3]));
        s.SetStateFlag(Impl::Handle(i),source.flags[0]);
        if(!found->actor.clips.empty()){
            object.playback=std::make_unique<ActorPlayback>(found->actor,source.resource,read);
            if(!found->animation.empty())Require(object.playback->Select(found->animation),"NPC default animation is missing");
        }
        s.current=i;(void)pal4::GameArenaNpcItemCallbacks::EnterNpc(Impl::Handle(i),Impl::world,s);
    }
}
SceneNpcs::~SceneNpcs()=default;
std::optional<std::size_t> SceneNpcs::FindInteraction(Point3 position,const InteractionEligibility& eligibility) const {
    auto& s=*impl_;for(auto value:position)Require(std::isfinite(value),"invalid NPC interaction position");
    struct Query final:pal4::NpcManagerClosestRuntimeStub {
        Impl& owner;Point3 position;const InteractionEligibility& eligibility;
        Query(Impl& o,Point3 p,const InteractionEligibility& e):owner(o),position(p),eligibility(e){}
        std::uint32_t ReadDword(std::uint32_t address) override {
            if(address>=0x7000 && address<=0x7008 && address%4==0)return std::bit_cast<std::uint32_t>(position[(address-0x7000)/4]);
            if(address>=0x40000000 && address<0x40000000+owner.states.size()*256){
                const auto index=(address-0x40000000)/256,offset=(address-0x40000000)%256;
                if(offset==64)return owner.arena.catalog.npcs[index].flags[1];
                if(offset==68)return owner.arena.catalog.npcs[index].flags[2];
            }
            const auto index=owner.Index(address);const auto offset=address-Impl::Handle(index);
            if(offset==0x654)return 0x40000000+static_cast<std::uint32_t>(index)*256;
            if(offset==0x1f0)return owner.states[index].active;
            if(offset>=0xac && offset<=0xb4 && offset%4==0)return std::bit_cast<std::uint32_t>(owner.states[index].position[(offset-0xac)/4]);
            if(offset==0x14)return 0x10000+static_cast<std::uint32_t>(index);
            throw std::logic_error("unsupported NPC selection read");
        }
        void WriteDword(std::uint32_t address,std::uint32_t value) override{const auto index=owner.Index(address);Require(address-Impl::Handle(index)==0x200 && value<=1,"invalid NPC selection mark");}
        std::uint32_t CountClumpMaterials(std::uint32_t clump) override {
            const auto index=owner.Clump(clump);const auto& actor=owner.arena.actors[owner.states[index].actor].actor;
            Require(actor.render_bindings.has_value(),"NPC selection requires original Atomic mapping");unsigned count{};
            for(std::size_t a=0;a<actor.render_bindings->atomics.size();++a)if(!eligibility || eligibility(index,a))++count;
            return count;
        }
        double ApproximateMagnitude(float x,float y,float z) override {
            static const auto tables=[]{pal4::GameMathTables result;(void)result.CalculateSquareRootTable();return result;}();
            return pal4::GameVectorMath::ApproximateMagnitude({x,y,z},tables);
        }
    } query(s,position,eligibility);
    std::vector<std::size_t> order(s.states.size());std::iota(order.begin(),order.end(),0);
    std::sort(order.begin(),order.end(),[&](auto a,auto b){return s.states[a].name<s.states[b].name;});
    std::vector<std::uint32_t> candidates;for(auto i:order)candidates.push_back(Impl::Handle(i));
    const auto selected=pal4::NpcManagerClosestFinder::FindByCoordinateList(candidates,0x7000,false,query);
    return selected?std::optional(s.Index(selected)):std::nullopt;
}
bool SceneNpcs::InteractionEnabled(std::size_t i) const{return impl_->states.at(i).active && impl_->arena.catalog.npcs[i].flags[1]!=0;}
std::string_view SceneNpcs::InteractionScript(std::size_t i) const{return impl_->arena.catalog.npcs.at(i).script;}
float SceneNpcs::InteractionRadius(std::size_t i) const{return std::bit_cast<float>(impl_->arena.catalog.npcs.at(i).flags[2]);}
bool SceneNpcs::HasAction(std::size_t index,std::string_view action) const {
    const auto& actor=impl_->arena.actors[impl_->states.at(index).actor].actor;
    return std::any_of(actor.clips.begin(),actor.clips.end(),[&](const auto& clip){return clip.name==action;});
}
ActorPlayback* SceneNpcs::Playback(std::size_t index){return impl_->objects.at(index).playback.get();}
bool SceneNpcs::SetVisible(std::string_view name,bool visible) {
    Require(name.find('\0')==std::string_view::npos,"embedded NUL in NPC name");++impl_->calls;
    if(!impl_->FindNpc(Impl::manager,name))return false;
    const std::string text(name);(void)pal4::ScriptNpcVisibility::SetVisible(text.c_str(),visible,*impl_);++impl_->matched;return true;
}
void SceneNpcs::CleanupWeaponEffects(std::size_t index){
    const auto& object=impl_->objects.at(index);
    for(auto offset:{0x518u,0x51cu,0x22cu,0x39cu})Require(object.base.Read(offset)==0,"NPC weapon cleanup is not bound");
}
float SceneNpcs::MovementRadius(std::size_t index) const{return std::bit_cast<float>(impl_->objects.at(index).base.Read(0xdc));}
void SceneNpcs::SetMotionPosition(std::size_t index,Point3 p){for(float v:p)Require(std::isfinite(v),"invalid NPC motion position");impl_->states.at(index).position=p;}
void SceneNpcs::SetMotionRotation(std::size_t index,Point3 p){for(float v:p)Require(std::isfinite(v),"invalid NPC motion rotation");impl_->states.at(index).rotation=p;}
void SceneNpcs::BindScriptMotion(){Require(!impl_->script_motion_bound,"NPC movement already has an owner");impl_->script_motion_bound=true;}
void SceneNpcs::UnbindScriptMotion() noexcept{impl_->script_motion_bound=false;}
void SceneNpcs::Advance(float seconds){Require(std::isfinite(seconds) && seconds>=0,"invalid NPC elapsed");for(std::size_t i=0;i<impl_->states.size();++i)if(impl_->states[i].active && impl_->objects[i].playback)impl_->objects[i].playback->Advance(seconds);++impl_->updates;}
std::span<const SceneNpcState> SceneNpcs::States() const{return impl_->states;}
bool SceneNpcs::Drawable(std::size_t i) const {const auto& s=impl_->states.at(i);return s.active && s.world_attached && s.geometry_bound && s.material_bound && s.render_bound;}
void SceneNpcs::Sample(std::size_t i,std::vector<Vertex>& output,std::vector<Matrix>* frames){auto& object=impl_->objects.at(i);if(object.playback)object.playback->Sample(output,frames);else SampleActorVertices(impl_->arena.actors[impl_->states.at(i).actor].actor,-1,0,output,AnimationTimeMode::loop,frames);
    object.pose_checksum=0;for(const auto& v:output)object.pose_checksum+=v.position[0]+2.0*v.position[1]+3.0*v.position[2];object.sample_time=object.playback?object.playback->Time():0;++object.samples;
}
void SceneNpcs::WriteReport(std::ostream& output,bool include_source) const {
    output<<std::setprecision(9)<<"{\"calls\":"<<impl_->calls<<",\"matched\":"<<impl_->matched<<",\"updates\":"<<impl_->updates<<",\"motion_bound\":"<<(impl_->script_motion_bound?"true":"false")<<",\"npcs\":[";
    for(std::size_t i=0;i<impl_->states.size();++i){if(i)output<<',';const auto& s=impl_->states[i];const auto& o=impl_->objects[i];
        output<<"{\"name\":"<<std::quoted(s.name)<<",\"resource\":"<<std::quoted(s.resource)<<",\"active\":"<<s.active<<",\"position\":["<<s.position[0]<<','<<s.position[1]<<','<<s.position[2]<<"],\"rotation\":["<<s.rotation[0]<<','<<s.rotation[1]<<','<<s.rotation[2]<<"],\"world\":"<<s.world_attached<<",\"movement\":"<<s.movement_registered<<",\"camera\":"<<s.camera_registered<<",\"collision_bound\":"<<s.collision_bound<<",\"drawable\":"<<Drawable(i)<<",\"enters\":"<<o.enters<<",\"leaves\":"<<o.leaves<<",\"collision_face\":"<<o.collision.words[2]<<",\"animation\":";
        if(o.playback)o.playback->WriteReport(output,include_source);else output<<"null";
        output<<",\"samples\":"<<o.samples<<",\"sample_time\":"<<o.sample_time<<",\"pose_checksum\":"<<std::setprecision(17)<<o.pose_checksum<<std::setprecision(9)<<'}';
    }output<<"]}";Require(bool(output),"cannot write NPC report");
}
}
