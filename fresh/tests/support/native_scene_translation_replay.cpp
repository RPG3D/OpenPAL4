#include "native_scene_translation_replay.h"
#include "pal4/actor_control_discrete_movement.h"
#include "pal4/native_scene_movement_asset.h"
#include "pal4/native_scene_player_movement.h"
#include <rwcore.h>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
namespace {
void Require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
std::uint32_t Address(const void* p){return reinterpret_cast<std::uint32_t>(p);}
bool ResourceIdentifier(const std::string& name){
    if(name.empty() || name.size()>31u)return false;
    for(const unsigned char c:name)if(!((c>='A' && c<='Z') || (c>='a' && c<='z') ||
        (c>='0' && c<='9') || c=='_'))return false;
    return true;
}
float Float(std::uint32_t bits){const auto v=std::bit_cast<float>(bits);Require(std::isfinite(v),"finite translation fixture word");return v;}
struct Runtime final:ActorControlDiscreteMovementRuntimeStub {
    explicit Runtime(NativeScenePlayerMovement& movement):movement(movement){}
    NativeScenePlayerMovement& movement;
    std::array<std::uint32_t,3> before{},forward{};
    std::uint32_t height{},tag{};
    ActorControlVec3 actual{};
    bool assigned{};
    std::uint32_t CameraSystem()override{return 0x6000u;}
    bool CameraActive(std::uint32_t p)override{Require(p==0x6000u,"fixture camera");return true;}
    std::uint32_t ReadWord(std::uint32_t p)override{
        if(p==0x1014u)return 0x2000u;if(p==0x2004u)return 0x3000u;
        if(p>=0x10acu && p<=0x10b4u && p%4u==0u)return before[(p-0x10acu)/4u];
        if(p>=0x4020u && p<=0x4028u && p%4u==0u)return forward[(p-0x4020u)/4u];
        if(p==0x5024u)return tag;
        throw std::runtime_error("unexpected translation fixture memory read");
    }
    std::uint32_t ObjectTransform(std::uint32_t p)override{Require(p==0x3000u,"fixture transform");return 0x4000u;}
    double ActorProperty(std::uint32_t p)override{Require(p==0x1000u,"fixture actor");return double(Float(height))*2.0;}
    std::uint32_t Controller()override{return 0x5000u;}
    void Normalize(ActorControlVec3& v)override{
        static_assert(sizeof(v)==sizeof(RwV3d));auto* p=reinterpret_cast<RwV3d*>(&v);(void)RwV3dNormalize(p,p);
    }
    std::string AnimationName(std::uint32_t)override{return "C02";}
    std::uint32_t MovementAdapter()override{return movement.Address();}
    void PreprocessMovement(std::uint32_t adapter,ActorControlMovementGlobals& g,
                            ActorControlVec3& v,std::uint32_t object,
                            std::uint32_t a,std::uint32_t b,std::uint32_t c)override{
        Require(adapter==movement.Address() && object==0x2000u && a==1u && b==1u && c==1u,"fixture collision forwarding");
        ConfigCollisionPosition lower{},upper{};CollisionPoint3 velocity{};
        std::memcpy(lower.data(),&g.lower,sizeof(lower));std::memcpy(upper.data(),&g.upper,sizeof(upper));
        std::memcpy(velocity.data(),&v,sizeof(velocity));
        // No actor clump is registered in this static-resource probe. There is
        // no self clump to exclude; dynamic objects/events are outside its scope.
        movement.ResolveCollision(lower,upper,velocity,0u);
        std::memcpy(&g.lower,lower.data(),sizeof(lower));std::memcpy(&g.upper,upper.data(),sizeof(upper));
        std::memcpy(&v,velocity.data(),sizeof(velocity));
    }
    void UpdateMovement(std::uint32_t adapter,ActorControlVec3& p,float above,float below,std::uint32_t enabled)override{
        Require(adapter==movement.Address() && enabled==1u,"fixture floor forwarding");
        movement.SelectAndUpdate(Address(&p),above,below,enabled);
    }
    bool CanMove(std::uint32_t adapter,ActorControlVec3& p,float above,float below)override{
        Require(adapter==movement.Address(),"fixture floor acceptance");
        return movement.CheckActive(Address(&p),above,below,Address(&p));
    }
    void SetActorPosition(std::uint32_t p,const ActorControlVec3& value)override{
        Require(p==0x1000u,"fixture position output");actual=value;assigned=true;
    }
    void RotateActor(std::uint32_t,const ActorControlVec3&,std::uint32_t)override{
        throw std::runtime_error("translation fixture unexpectedly rotates actor");
    }
};
}
void ReplayNativeSceneTranslation(const std::filesystem::path& root,const std::filesystem::path& fixture){
    std::ifstream input(fixture);std::string magic,world="M02",scene="1";unsigned count{};
    Require(bool(input>>magic),"translation fixture header");
    if(magic=="PAL4_SCENE_TRANSLATION_2"){
        Require(bool(input>>world>>scene),"translation fixture resource identifiers");
        Require(ResourceIdentifier(world) && ResourceIdentifier(scene),"bounded translation resource identifiers");
    }else Require(magic=="PAL4_SCENE_TRANSLATION_1","supported translation fixture version");
    Require(bool(input>>count) && count>0u && count<=10000u,"bounded actual translation fixture");
    NativeSceneMovementAsset assets;
    if(!assets.Load(root,world,scene) || !assets.Floor())throw std::runtime_error(world+"/"+scene+" movement resource: "+assets.LastError());
    std::array<unsigned,2> cases{},mismatches{};double worst{};
    {
        NativeScenePlayerMovement movement(assets);Runtime runtime(movement);
        for(unsigned index=0;index<count;++index){
            unsigned side{},source{},command{};std::uint32_t elapsed{},speed{};
            Require(bool(input>>side>>source>>command>>elapsed>>speed>>runtime.height>>runtime.tag) && side<2u && (command==1u || command==2u),"actual translation row header");
            std::array<std::uint32_t,3> expected{};
            for(auto& v:runtime.forward)Require(bool(input>>v),"complete actual forward vector");
            for(auto& v:runtime.before)Require(bool(input>>v),"complete actual before position");
            for(auto& v:expected)Require(bool(input>>v),"complete actual expected position");
            for(const auto* a:{&runtime.forward,&runtime.before,&expected})for(auto v:*a)(void)Float(v);
            Require(Float(elapsed)>0.f && Float(speed)>0.f && Float(runtime.height)>0.f && Float(runtime.tag)>0.f,
                    "positive actual translation parameters");
            runtime.actual={Float(runtime.before[0]),Float(runtime.before[1]),Float(runtime.before[2])};runtime.assigned=false;
            ActorControlInputComponentState state{};state.words[1]=speed;state.words[2]=0x42fdcccdu;state.words[10]=1u;
            ActorControlMovementGlobals globals{};
            Require(ActorControlInputComponent::DispatchDiscreteMovement(state,globals,0x1000u,command,Float(elapsed),runtime)==1u,
                    "actual translation uses production discrete dispatcher");
            const std::array<std::uint32_t,3> result{std::bit_cast<std::uint32_t>(runtime.actual.x),
                std::bit_cast<std::uint32_t>(runtime.actual.y),std::bit_cast<std::uint32_t>(runtime.actual.z)};
            const bool match=result==expected;++cases[side];if(!match)++mismatches[side];
            double squared{};for(unsigned i=0;i<3u;++i){const auto delta=double(Float(result[i]))-Float(expected[i]);squared+=delta*delta;}
            const auto error=std::sqrt(squared);if(error>worst)worst=error;
            std::cout<<"{\"side\":"<<side<<",\"source_row\":"<<source<<",\"assigned\":"<<(runtime.assigned?"true":"false")
                <<",\"match\":"<<(match?"true":"false")<<",\"actual_bits\":["<<result[0]<<','<<result[1]<<','<<result[2]
                <<"],\"expected_bits\":["<<expected[0]<<','<<expected[1]<<','<<expected[2]<<"],\"absolute_error\":"<<error<<"}\n";
        }
        movement.DetachScene();
    }
    std::string extra;Require(!(input>>extra),"translation fixture has no trailing data");assets.Unload();
    std::cout<<"{\"summary\":true,\"cases\":["<<cases[0]<<','<<cases[1]<<"],\"mismatches\":["<<mismatches[0]<<','<<mismatches[1]
        <<"],\"worst_absolute_error\":"<<worst<<"}\n";
    Require(cases[0] && cases[1] && !mismatches[0] && !mismatches[1],"static scene translation does not match observed positions; inspect fixture context before attributing a game defect");
}
void ProbeNativeSceneMovement(const std::filesystem::path& root,const std::filesystem::path& fixture){
    std::ifstream input(fixture);std::string magic,world,scene;unsigned count{};
    Require(bool(input>>magic>>world>>scene>>count) && magic=="PAL4_SCENE_PROBE_1" &&
        ResourceIdentifier(world) && ResourceIdentifier(scene) && count>0u && count<=10000u,
        "bounded static movement probe header");
    NativeSceneMovementAsset assets;
    if(!assets.Load(root,world,scene) || !assets.Floor())throw std::runtime_error("probe movement resources: "+assets.LastError());
    {
        NativeScenePlayerMovement movement(assets);Runtime runtime(movement);
        for(unsigned index=0;index<count;++index){
            unsigned side{},source{},command{},steps{};std::uint32_t elapsed{},speed{};
            Require(bool(input>>side>>source>>command>>elapsed>>steps>>speed>>runtime.height>>runtime.tag) &&
                side<2u && (command==1u || command==2u) && steps>0u && steps<=600u,"bounded static movement probe row");
            for(auto& v:runtime.forward)Require(bool(input>>v),"probe forward vector");
            for(auto& v:runtime.before)Require(bool(input>>v),"probe before vector");
            for(const auto* values:{&runtime.forward,&runtime.before})for(auto v:*values)(void)Float(v);
            Require(Float(elapsed)>0.f && Float(elapsed)<=0.1f && Float(speed)>0.f &&
                Float(runtime.height)>0.f && Float(runtime.tag)>0.f,"positive bounded probe parameters");
            ActorControlInputComponentState state{};state.words[1]=speed;state.words[2]=0x42fdcccdu;state.words[10]=1u;
            for(unsigned step=0;step<steps;++step){
                runtime.actual={Float(runtime.before[0]),Float(runtime.before[1]),Float(runtime.before[2])};runtime.assigned=false;
                ActorControlMovementGlobals globals{};
                Require(ActorControlInputComponent::DispatchDiscreteMovement(state,globals,0x1000u,command,Float(elapsed),runtime)==1u,"probe production dispatcher");
                runtime.before={std::bit_cast<std::uint32_t>(runtime.actual.x),std::bit_cast<std::uint32_t>(runtime.actual.y),std::bit_cast<std::uint32_t>(runtime.actual.z)};
            }
            std::cout<<"{\"probe_only\":true,\"side\":"<<side<<",\"source_row\":"<<source
                <<",\"actual_bits\":["<<runtime.before[0]<<','<<runtime.before[1]<<','<<runtime.before[2]<<"]}\n";
        }
        movement.DetachScene();
    }
    std::string extra;Require(!(input>>extra),"probe rejects trailing data");assets.Unload();
    std::cout<<"Static movement probe completed: planning output only, no equivalence assertion\n";
}

}
