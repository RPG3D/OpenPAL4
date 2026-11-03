#include "pal4/portable/scene_interaction.h"
#include "pal4/pal_actor_control_interaction.h"
#include <bit>
#include <cmath>
#include <ostream>
#include <stdexcept>
namespace pal4::portable {
namespace {
void Require(bool ok,const char* error){if(!ok)throw std::logic_error(error);}
}
struct SceneInteraction::Impl final:pal4::IActorControlInteractionRuntime {
    static constexpr std::uint32_t actor=0x10000,npc_base=0x10000000,record_base=0x20000000,
        gob_base=0x30000000,event_base=0x50000000,action_base=0x60000000,script_base=0x70000000;
    const ArenaResources& arena;SceneNpcs& npcs;SceneObjects& objects;SceneEvents& events;
    ISceneEventScripts& scripts;pal4::IActorControlGameEventRuntime& hints;
    SceneNpcs::InteractionEligibility npc_eligibility;
    pal4::PalActorControlState state;pal4::ActorInteractionInput input;
    Point3 position{};float bind_radius{},world_radius{};bool previous{};
    unsigned frames{},npc_requests{},gob_requests{},event_requests{};
    Impl(const ArenaResources& a,SceneNpcs& n,SceneObjects& o,SceneEvents& e,ISceneEventScripts& s,
         pal4::IActorControlGameEventRuntime& h,SceneNpcs::InteractionEligibility eligibility)
        :arena(a),npcs(n),objects(o),events(e),scripts(s),hints(h),npc_eligibility(std::move(eligibility)) {
        Require(a.catalog.npcs.size()<65536 && a.catalog.gobs.size()<65536 && a.catalog.events.size()<65536,
                "interaction handle capacity exceeded");
        Require(bool(npc_eligibility),"NPC interaction requires original Atomic eligibility");
        state.words[1]=actor;
    }
    static std::uint32_t Handle(std::uint32_t base,std::size_t i,std::uint32_t stride){return base+static_cast<std::uint32_t>(i)*stride;}
    std::size_t Index(std::uint32_t address,std::uint32_t base,std::uint32_t stride,std::size_t count) const {
        Require(address>=base && (address-base)/stride<count,"interaction handle outside collection");
        return (address-base)/stride;
    }
    std::size_t Gob(std::uint32_t handle) const{return Index(handle,gob_base,4096,objects.States().size());}
    pal4::ActorInteractionInput Input() override{return input;}
    std::uint32_t NpcByPosition(std::uint32_t address) override {
        Require(address==actor+172,"NPC interaction query address mismatch");
        const auto i=npcs.FindInteraction(position,npc_eligibility);return i?Handle(npc_base,*i,4096):0;
    }
    std::uint32_t DialogByPosition(std::uint32_t address,float radius) override {
        Require(address==actor+172,"GOB interaction query address mismatch");
        const auto i=objects.FindInteraction(position,radius);return i?Handle(gob_base,*i,4096):0;
    }
    std::vector<std::uint32_t> EffectCandidates() override {
        std::vector<std::uint32_t> result;for(std::size_t i=0;i<events.States().size();++i)result.push_back(Handle(event_base,i,256));return result;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if(address>=npc_base && address<record_base){const auto i=Index(address,npc_base,4096,npcs.States().size());
            switch((address-npc_base)%4096){case 0x1f0:return npcs.States()[i].active;case 0x654:return Handle(record_base,i,256);default:break;}}
        if(address>=record_base && address<gob_base){const auto i=Index(address,record_base,256,npcs.States().size());
            switch((address-record_base)%256){case 64:return npcs.InteractionEnabled(i);case 28:return static_cast<std::uint32_t>(npcs.InteractionScript(i).size());case 24:return Handle(script_base,i,16);default:break;}}
        if(address>=event_base && address<action_base){const auto i=Index(address,event_base,256,events.States().size());const auto& e=arena.catalog.events[i];
            switch((address-event_base)%256){case 4:return events.States()[i].type;case 76:return events.States()[i].active;case 64:return Handle(action_base,i,256);
                case 84:return e.properties.empty()?0:256;case 88:return 256+static_cast<std::uint32_t>(e.properties.size())*80;default:return 0;}}
        throw std::logic_error("unsupported interaction handle read");
    }
    float ReadFloat(std::uint32_t address) override {
        if(address>=actor+172 && address<=actor+180 && address%4==0)return position[(address-actor-172)/4];
        if(address==actor+236)return world_radius;
        throw std::logic_error("unsupported interaction float read");
    }
    double ActorProperty(std::uint32_t handle) override{Require(handle==actor,"interaction actor mismatch");return bind_radius*2.;}
    pal4::ActorControlEffectPoint EffectVertex(std::uint32_t handle,std::int32_t vertex) override {
        const auto i=Index(handle,event_base,256,events.States().size());const auto& p=arena.catalog.events[i].properties.at(static_cast<std::size_t>(vertex));
        return {std::bit_cast<float>(p[0]),std::bit_cast<float>(p[1]),std::bit_cast<float>(p[2])};
    }
    void ExecuteScript(std::uint32_t handle) override {
        const auto i=Index(handle,script_base,16,npcs.States().size());const auto name=npcs.InteractionScript(i);
        Require(!name.empty() && name.size()<=128 && name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_")==std::string_view::npos,
                "NPC interaction function is not an identifier");
        const auto declaration="void "+std::string(name)+"()";++npc_requests;
        if(!scripts.Contains(arena.description.world,declaration)){
            scripts.LoadWorld(arena.description.world);
            Require(scripts.Push(arena.description.world,declaration)==0,"NPC interaction script push failed");
        }
    }
    bool DialogEnabled(std::uint32_t handle) override{return objects.InteractionEnabled(Gob(handle));}
    bool DialogInRange(std::uint32_t handle,std::uint32_t address,float radius) override {
        Require(address==actor+172,"interaction position mismatch");return objects.InteractionInRange(Gob(handle),position,radius);
    }
    void AttachDialog(std::uint32_t handle,std::uint32_t address) override {
        Require(address==actor+172,"interaction attachment position mismatch");++gob_requests;
        (void)objects.Interact(Gob(handle),scripts);
    }
    void TriggerEffect(std::uint32_t handle) override{++event_requests;(void)events.TriggerInteraction(Index(handle,action_base,256,events.States().size()));}
    std::int32_t EventCode(std::uint32_t handle) override{return objects.InteractionEvent(Gob(handle));}
    std::uint32_t CreateHintWindow() override{return hints.CreateHintWindow();}
    std::optional<std::string> TriggerText(std::uint32_t id) override{return hints.TriggerText(id);}
    std::string ConvertToUtf8(std::string_view text) override{return hints.ConvertToUtf8(text);}
    std::string AssignHintText(std::string_view text) override{return hints.AssignHintText(text);}
    std::uint32_t EncodedSize(std::string_view text) override{return hints.EncodedSize(text);}
    std::string EncodeHintText(std::string_view text,std::uint32_t size) override{return hints.EncodeHintText(text,size);}
    void InitializeHintWindow(std::uint32_t window,std::string_view text,float x,float y,float duration,std::uint32_t zero,std::uint32_t one) override{hints.InitializeHintWindow(window,text,x,y,duration,zero,one);}
    void PlayHintSound(std::string_view path,std::uint32_t repeat) override{hints.PlayHintSound(path,repeat);}
};
SceneInteraction::SceneInteraction(const ArenaResources& a,SceneNpcs& n,SceneObjects& o,SceneEvents& e,ISceneEventScripts& s,
    pal4::IActorControlGameEventRuntime& h,SceneNpcs::InteractionEligibility eligibility)
    :impl_(std::make_unique<Impl>(a,n,o,e,s,h,std::move(eligibility))){}
SceneInteraction::~SceneInteraction()=default;
void SceneInteraction::Reset(bool pressed){auto& s=*impl_;s.state.words[20]=s.state.words[21]=s.state.words[22]=0;s.previous=pressed;s.input={};}
bool SceneInteraction::Advance(bool pressed,bool blocked,Point3 position,float bind_radius,float world_radius,std::uint32_t actor_index){
    auto& s=*impl_;++s.frames;
    if(blocked){Reset(pressed);return false;}
    for(float value:position)Require(std::isfinite(value),"invalid interaction position");
    Require(std::isfinite(bind_radius) && bind_radius>=0 && std::isfinite(world_radius) && world_radius>=0 && actor_index<4,"invalid interaction actor bounds");
    s.position=position;s.bind_radius=bind_radius;s.world_radius=world_radius;s.state.words[19]=actor_index;
    s.input={};if(pressed && !s.previous){s.input.secondary_count=1;s.input.secondary_mode=1;}
    if(!pressed && s.previous)s.input.secondary_release=1;
    s.previous=pressed;return pal4::PalActorControlInteraction::Handle(s.state,0,s);
}
void SceneInteraction::WriteReport(std::ostream& out) const{const auto& s=*impl_;out<<"{\"frames\":"<<s.frames<<",\"npc_requests\":"<<s.npc_requests<<",\"gob_requests\":"<<s.gob_requests<<",\"event_requests\":"<<s.event_requests<<",\"targets\":["<<s.state.words[20]<<','<<s.state.words[21]<<','<<s.state.words[22]<<"]}";}
}
