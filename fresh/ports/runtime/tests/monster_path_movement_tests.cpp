#include "pal4/portable/monster_movement.h"
#include "pal4/behavior_helper_configuration.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
#include <source_location>
#include <stdexcept>
using namespace pal4;
using namespace pal4::portable;
namespace {
void Check(bool value,const std::source_location where=std::source_location::current()){
    if(!value)throw std::runtime_error("path frame contract failed at line "+std::to_string(where.line()));
}
template<class F>void Reject(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}Check(failed);}
struct Fixture {
    std::uint32_t sample=16384;std::size_t draws{};bool walking{};std::function<void()> on_random;
    std::shared_ptr<ScenePathways> paths;
    MonsterMovementWorld world;BehaviorHelperConfiguration config;MonsterMovement movement;
    explicit Fixture(float radius=35):paths(std::make_shared<ScenePathways>(1,std::vector<ScenePath>{{"route",1,std::bit_cast<std::uint32_t>(radius),0,{{0,0,0},{0,0,800}}}})),
        world(1,paths,[this]{if(on_random)on_random();constexpr std::uint32_t stream[]{16384,24575,8192,16384,8192,24575};const auto result=walking?stream[draws%6]:sample;++draws;return result;}),config(world),movement(world){}
    MonsterHelperHandle Create(const BehaviorVector3& position,float radius,float speed,const BehaviorVector3& heading,float max_speed=1,float max_force=4,bool random=false){
        const SceneMonsterHandle actor{1,static_cast<std::uint32_t>(world.Stats().helpers)};const auto id=static_cast<std::uint32_t>(world.Stats().helpers+1);
        const auto h=random?world.CreateRandom(actor,id):world.CreateEndpoint(actor,id);auto& helper=world.Helper(h);
        (void)config.Configure(helper,world.GridParameter(),world.FindPath("route"));
        BehaviorHelperConfiguration::SetVector(helper,4,{1,0,0});BehaviorHelperConfiguration::SetVector(helper,0xc,{0,1,0});
        BehaviorHelperConfiguration::SetVector(helper,0x14,heading);BehaviorHelperConfiguration::SetVector(helper,0x1c,position);
        BehaviorHelperConfiguration::SetScalar(helper,0x5c,radius);BehaviorHelperConfiguration::SetScalar(helper,0x68,speed);
        BehaviorHelperConfiguration::SetScalar(helper,0x74,max_force);BehaviorHelperConfiguration::SetScalar(helper,0x7c,max_speed);
        if(random){helper.words[47]=std::bit_cast<std::uint32_t>(10.f);helper.words[49]=0;
            for(unsigned axis=0;axis<3;++axis){helper.words[50+axis]=std::bit_cast<std::uint32_t>(heading[axis]);helper.words[53+axis]=std::bit_cast<std::uint32_t>(position[axis]);}
        }else{helper.words[48]=1;helper.words[49]=0x100;}world.AddElement(helper);(void)world.UpdatePosition(helper.words[42],position);return h;
    }
    void Last(MonsterHelperHandle h,const BehaviorVector3& endpoint){auto& image=world.Helper(h);for(unsigned axis=0;axis<3;++axis){image.words[50+axis]=std::bit_cast<std::uint32_t>(-100.f);image.words[53+axis]=std::bit_cast<std::uint32_t>(endpoint[axis]);}}
};
void Walking(){
    Fixture f;const auto h=f.Create({0,0,100},.2f,100,{0,0,1},100,50);f.world.Helper(h).words[49]=0x101;f.walking=true;
    const auto baseline=f.world.Stats();float minimum=100,maximum=100;bool reversed=false,returned=false;
    for(unsigned frame=0;frame<2400;++frame){
        const auto result=f.movement.Update(h,{},1.f/60);Check(result.active);
        const auto& image=f.world.Helper(h);const auto point=BehaviorHelperConfiguration::ReadVector(image,0x18);
        for(const auto value:point)Check(std::isfinite(value));Check(point[1]==0 && std::bit_cast<float>(image.words[25])<=100.001f);
        minimum=std::min(minimum,point[2]);maximum=std::max(maximum,point[2]);reversed|=image.words[48]==0xffffffffu;returned|=reversed && point[2]<100;
        const auto members=f.world.QuerySphere(point,.001f);Check(members.size()==1 && members[0].helper==h && members[0].distance_squared==0);
    }
    const auto& stats=f.movement.Stats();Check(stats.frames==2400 && stats.sphere_queries==2400 && stats.path_corrections==2400 && stats.direction_flips>=3);
    Check(maximum>765 && minimum<100 && reversed && returned);Check(f.world.Stats().mapped_bytes==baseline.mapped_bytes);
    std::cout<<"2400 frame path range "<<minimum<<".."<<maximum<<", flips "<<stats.direction_flips<<'\n';
    f.world.DestroyHelper(h);f.world.Close();Check(f.world.Stats().blocks==0);
}
void Collisions(){
    {
        Fixture f(.1f);const auto h=f.Create({0,0,0},.5f,1,{0,0,1});f.Last(h,{.4f,0,2});
        const std::array<MonsterMovementObstacle,2> obstacles{{{7,.5f,{.4f,0,3}},{9,.5f,{.4f,0,2}}}};
        const auto result=f.movement.Update(h,obstacles,0);Check(f.movement.Stats().candidate_tests==2 && f.movement.Stats().candidate_hits==2);
        Check(result.requested[0]<0 && f.world.Helper(h).words[48]==0xffffffffu && f.movement.Stats().sphere_queries==0);
    }
    {
        Fixture f(.1f);const auto h=f.Create({0,0,3},1,1,{0,0,1});const auto other=f.Create({.4f,0,3},1,0,{0,0,1});f.Last(h,{.4f,0,3});
        const auto result=f.movement.Update(h,{},0);Check(f.movement.Stats().proximity==1 && result.requested[0]<0 && f.world.Helper(h).words[48]==0xffffffffu);
        Check(f.movement.Stats().neighbor_visits==2);f.world.DestroyHelper(other);
    }
    {
        Fixture f(.1f);const auto h=f.Create({0,0,0},.2f,1,{0,0,1});const auto other=f.Create({0,0,4},.2f,1,{0,0,-1});f.Last(h,{0,0,2});
        const auto result=f.movement.Update(h,{},0);Check(f.movement.Stats().predicted==1 && result.requested[0]>0 && f.world.Helper(h).words[48]==0xffffffffu);
        // Original complex collision stores both projected points, not just force.
        Check(std::bit_cast<float>(f.world.Helper(h).words[17])==2 && std::bit_cast<float>(f.world.Helper(h).words[20])==2);f.world.DestroyHelper(other);
    }
    {
        Fixture f;const auto h=f.Create({0,0,3},.2f,1,{0,0,1});f.world.Helper(h).words[49]=1;f.sample=32767;
        const auto result=f.movement.Update(h,{},.1f);Check(result.active && result.requested[0]>0 && result.requested[1]==0);
        Check(f.movement.Stats().random_draws==4 && f.movement.Stats().drift_updates==2 && f.movement.Stats().path_corrections==1);
    }
}
void RandomMovement(){
    {
        Fixture f;f.walking=true;const auto h=f.Create({0,0,0},.2f,0,{0,0,1},5,5,true);const auto before=f.world.Stats();
        bool moved=false,turned=false;const auto direction=f.world.Helper(h).words[50];
        for(unsigned frame=0;frame<1200;++frame){
            const auto result=f.movement.Update(h,{},1.f/60);Check(result.active);const auto& image=f.world.Helper(h);const auto p=BehaviorHelperConfiguration::ReadVector(image,0x18);
            for(float v:p)Check(std::isfinite(v));for(float v:result.requested)Check(std::isfinite(v));
            Check(p[1]==0 && result.requested[1]==0 && (image.words[46]&0xff00)==0x100);moved|=p[0]!=0 || p[2]!=0;turned|=image.words[50]!=direction;
            const auto near=f.world.QuerySphere(p,.001f);Check(near.size()==1 && near.front().helper==h && near.front().distance_squared==0);
        }
        Check(moved && turned && f.movement.Stats().frames==1200 && f.movement.Stats().sphere_queries==1200 && f.movement.Stats().path_corrections==0);
        Check(f.world.Stats().mapped_bytes==before.mapped_bytes);f.world.Close();Check(f.world.Stats().blocks==0);
    }
    // Random self must use no-op notifications; a path neighbor is not rewritten.
    {
        Fixture f;const auto h=f.Create({0,0,3},1,1,{0,0,1},1,4,true),other=f.Create({.4f,0,3},1,0,{0,0,1});const auto before=f.world.Helper(other).words;
        const auto result=f.movement.Update(h,{},0);Check(f.movement.Stats().proximity==1 && result.requested[0]<0 && f.movement.Stats().neighbor_visits==2);
        Check(f.world.Helper(h).words[48]==std::bit_cast<std::uint32_t>(2.f) && f.world.Helper(other).words==before);
    }
    {
        Fixture f;const auto h=f.Create({0,0,0},.2f,1,{0,0,1},1,4,true);(void)f.Create({0,0,4},.2f,1,{0,0,-1});
        const auto result=f.movement.Update(h,{},0);Check(f.movement.Stats().predicted==1 && result.requested[0]>0);
        Check(std::bit_cast<float>(f.world.Helper(h).words[17])==2 && std::bit_cast<float>(f.world.Helper(h).words[20])==2);
    }
    {
        Fixture f;const auto h=f.Create({0,0,0},.5f,1,{0,0,1},1,4,true);const std::array<MonsterMovementObstacle,2> obstacles{{{7,.5f,{.4f,0,3}},{9,.5f,{.4f,0,2}}}};
        const auto result=f.movement.Update(h,obstacles,0);Check(result.requested[0]<0 && f.movement.Stats().candidate_tests==2 && f.movement.Stats().candidate_hits==2 && f.movement.Stats().sphere_queries==0);
    }
    // Endpoint self also accepts random neighbors through the shared typed getters.
    {
        Fixture f;const auto h=f.Create({0,0,3},1,1,{0,0,1});(void)f.Create({.4f,0,3},1,0,{0,0,1},1,4,true);f.Last(h,{.4f,0,3});
        Check(f.movement.Update(h,{},0).requested[0]<0 && f.movement.Stats().proximity==1);
    }
    {
        Fixture f;const auto h=f.Create({0,0,0},.2f,1,{0,0,1},1,4,true);auto& image=f.world.Helper(h);
        for(float range:{0.f,-1.f}){image.words[47]=std::bit_cast<std::uint32_t>(range);image.words[46]|=0x100;const auto before=image.words;const auto draws=f.draws;
            Check(!f.movement.Update(h,{},.1f).active && image.words==before && f.draws==draws);}
        for(float range:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}){image.words[47]=std::bit_cast<std::uint32_t>(range);const auto before=image.words;Reject([&]{(void)f.movement.Update(h,{},.1f);});Check(image.words==before);}
        image.words[47]=std::bit_cast<std::uint32_t>(10.f);f.on_random=[&]{(void)f.movement.Update(h,{},0);};Reject([&]{(void)f.movement.Update(h,{},.1f);});f.on_random={};Check(f.movement.Update(h,{},0).active);
    }
}
void Failures(){
    Fixture f;const auto h=f.Create({0,0,3},.2f,1,{0,0,1});const auto original=f.world.Helper(h).words;
    Reject([&]{(void)f.movement.Update(h,{},-1);});Reject([&]{(void)f.movement.Update(h,{},std::numeric_limits<float>::infinity());});
    Reject([&]{(void)f.movement.Update({2,h.token},{},.1f);});
    const std::array<MonsterMovementObstacle,1> bad{{{0,-1,{}}}};Reject([&]{(void)f.movement.Update(h,bad,.1f);});
    const std::array<MonsterMovementObstacle,1> nan{{{0,1,{0,std::numeric_limits<float>::quiet_NaN(),0}}}};Reject([&]{(void)f.movement.Update(h,nan,.1f);});
    Check(f.world.Helper(h).words==original && f.movement.Stats().frames==0 && f.movement.Stats().random_draws==0);
    const auto other=f.Create({10,0,3},.2f,1,{0,0,1});
    const auto association=f.world.Helper(h).words[42];f.world.Helper(h).words[42]=f.world.Helper(other).words[42];
    Reject([&]{(void)f.movement.Update(h,{},0);});f.world.Helper(h).words[42]=association;f.world.DestroyHelper(other);
    f.world.Helper(h).words[0]=0x842ae8u;Reject([&]{(void)f.movement.Update(h,{},0);});f.world.Helper(h).words[0]=0x842898u;
    f.on_random=[&]{(void)f.movement.Update(h,{},0);};Reject([&]{(void)f.movement.Update(h,{},.1f);});f.on_random={};
    Check(f.movement.Stats().frames==0);Check(f.movement.Update(h,{},0).active);
    auto& image=f.world.Helper(h);image.words[47]=0;image.words[46]|=0x100;const auto draws=f.draws;const auto before=image.words;
    Check(!f.movement.Update(h,{},.1f).active && f.draws==draws && (image.words[46]&0xff00)==0);
    for(unsigned i=0;i<56;++i)if(i!=46)Check(image.words[i]==before[i]);
    f.world.DestroyHelper(h);Reject([&]{(void)f.movement.Update(h,{},0);});f.world.Close();Reject([&]{(void)f.movement.Update(h,{},0);});
}
}
int main()try{Walking();Collisions();RandomMovement();Failures();std::cout<<"path movement contracts passed; generated paths and controlled obstacles, no gameplay executed\n";return 0;}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
