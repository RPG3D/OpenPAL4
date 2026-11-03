#include "pal4/portable/monster_orientation.h"
#include "pal4/behavior_helper_configuration.h"
#include <bit>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <source_location>
#include <stdexcept>
using namespace pal4;
using namespace pal4::portable;
namespace {
void Check(bool value,const std::source_location where=std::source_location::current()){if(!value)throw std::runtime_error("monster orientation contract failed at "+std::to_string(where.line()));}
void Near(float a,float b){Check(std::isfinite(a) && std::abs(a-b)<.001f);}
template<class F>void Reject(F call){bool rejected=false;try{call();}catch(const std::exception&){rejected=true;}Check(rejected);}
Actor Model(){
    Actor a;const Matrix identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};a.frames.push_back({-1,identity});a.palettes.push_back({0,identity});
    a.mesh.vertices.resize(3);a.mesh.vertices[0].position[0]=1;a.influences.resize(3,{{0,0,0,0},{1,0,0,0}});a.atomic_spheres.push_back({0,{0,0,0,2}});
    a.clips.push_back({"Z01",1,{{0,{{0,{0,0,0,1},{0,0,0}},{1,{0,0,0,1},{1,0,0}}}}}});return a;
}
std::vector<SceneMonster> Records(){SceneMonster a;a.name="first";a.resource="401";a.position={1,2,3};a.rotation={11,22,33};auto b=a;b.name="second";return {a,b};}
std::vector<CollisionTriangle> Floor(float y){return {{{Point3{-100,y,-100},Point3{100,y,-100},Point3{0,y,100}},1,0,0}};}
void PrimaryFloor(){
    const auto primary=Floor(0),secondary=Floor(10);SceneMovement movement(primary,secondary);
    Check(movement.SelectAndUpdate({0,10,0},1,1) && movement.ActiveChild()==1);const auto previous=movement.LastHit();Check(previous.has_value());
    Point3 p{0,2,0};Check(movement.CheckPrimaryAndAdjust(p,1,3) && p==Point3{0,0,0});Check(movement.ActiveChild()==1 && movement.LastHit()->position==previous->position);
    p={0,20,0};const auto before=p;Check(!movement.CheckPrimaryAndAdjust(p,1,3) && p==before);
    Reject([&]{(void)movement.CheckPrimaryAndAdjust(p,-1,1);});Reject([&]{(void)movement.CheckPrimaryAndAdjust(p,1,std::numeric_limits<float>::infinity());});
    SceneMovement empty({});p={0,2,0};Check(!empty.CheckPrimaryAndAdjust(p,1,3) && p==Point3{0,2,0});
}
void Orientation(){
    const auto records=Records();SceneMonsterInstances actors(1,records,[](auto){return Model();},[](auto)->std::optional<std::vector<std::uint8_t>>{return {};});
    auto paths=std::make_shared<ScenePathways>(1,std::span<const ScenePath>{});std::size_t draws{};
    MonsterMovementWorld world(1,paths,[&]{constexpr std::uint32_t stream[]{16384,16384,24575,100};return stream[draws++%4];});BehaviorHelperConfiguration configuration(world);
    const auto primary=Floor(0),secondary=Floor(10);SceneMovement floor(primary,secondary);Check(floor.SelectAndUpdate({0,10,0},1,1));
    MonsterOrientationCaches caches;MonsterOrientation orientation(world,actors,floor,caches);
    const auto create=[&](unsigned index,Point3 p,bool random=false){
        const auto h=random?world.CreateRandom(actors.Handle(index),index+1):world.CreateEndpoint(actors.Handle(index),index+1);auto& helper=world.Helper(h);(void)configuration.Configure(helper,world.GridParameter(),0);
        BehaviorHelperConfiguration::SetVector(helper,0x1c,p);BehaviorHelperConfiguration::SetVector(helper,0x14,{-1,0,0});
        helper.words[46]|=0x100;world.AddElement(helper);(void)world.UpdatePosition(helper.words[42],p);return h;
    };
    const auto a=create(0,{0,2,0}),b=create(1,{0,20,0});const auto result=orientation.Update(0,a,1);
    Check(result.active && result.floor_hit && result.transform.position==Point3{0,0,0});Near(result.transform.rotation[1],270);
    Check(result.transform.rotation[0]==11 && result.transform.rotation[2]==33 && actors.Source(actors.Handle(0)).position==records[0].position);
    Check(BehaviorHelperConfiguration::ReadVector(world.Helper(a),0x18)==Point3{0,0,0});Check(floor.ActiveChild()==1 && floor.LastHit()->position[1]==10);
    const auto node=world.QuerySphere({0,2,0},.001f);Check(node.size()==1 && node[0].helper==a);Check(world.QuerySphere({0,0,0},.001f).empty());
    Near(caches.kinds[0].first_radius,.7f);Near(caches.kinds[0].second_radius,5.06f);
    // The second C02 actor retains the first actor's original global limits.
    const auto miss=orientation.Update(0,b,100);Check(miss.active && !miss.floor_hit && miss.transform.position==Point3{0,20,0});Near(caches.kinds[0].second_radius,5.06f);
    const auto hit=orientation.Update(1,b,100);Check(hit.floor_hit && hit.transform.position==Point3{0,0,0});Near(caches.kinds[1].second_radius,506.f);
    const auto random=create(0,{0,2,0},true),random_second=create(1,{0,20,0},true);
    const auto random_result=orientation.Update(3,random,2);Check(random_result.active && random_result.floor_hit && random_result.transform.position==Point3{0,0,0});Near(random_result.transform.rotation[1],270);
    Near(caches.random.first_radius,1.4f);Near(caches.random.second_radius,10.12f);Near(caches.kinds[0].second_radius,5.06f);
    const auto random_miss=orientation.Update(3,random_second,100);Check(random_miss.active && !random_miss.floor_hit && random_miss.transform.position==Point3{0,20,0});Near(caches.random.second_radius,10.12f);
    const auto old_node=world.QuerySphere({0,2,0},.001f);Check(std::any_of(old_node.begin(),old_node.end(),[&](const auto& n){return n.helper==random;}));
    Reject([&]{(void)orientation.Update(0,random,1);});Reject([&]{(void)orientation.Update(2,random,1);});
    world.DestroyHelper(random);world.DestroyHelper(random_second);actors.SetMotionPosition(actors.Handle(1),{0,0,0});
    world.Helper(a).words[46]&=~0xff00u;actors.SetMotionPosition(actors.Handle(0),{7,8,9});
    const auto inactive=orientation.Update(0,a,2);Check(!inactive.active && !inactive.floor_hit && inactive.transform.position==Point3{7,8,9});
    MonsterOrientationCaches idle_caches;idle_caches.kinds[0].guard=1;idle_caches.kinds[0].first_radius=77;
    MonsterOrientation idle(world,actors,floor,idle_caches);Check(!idle.Update(0,a,3).active);
    Check(idle_caches.kinds[0].guard==3 && idle_caches.kinds[0].first_radius==77);Near(idle_caches.kinds[0].second_radius,15.18f);
    Check(actors.InstanceTransform(actors.Handle(0))[13]==8);Check(actors.MotionTransform(actors.Handle(1)).position==Point3{0,0,0});
    Reject([&]{(void)orientation.Update(3,a,1);});Reject([&]{(void)orientation.Update(0,a,-1);});Reject([&]{(void)orientation.Update(0,a,std::numeric_limits<float>::quiet_NaN());});
    Reject([&]{(void)orientation.Update(0,{2,a.token},1);});
    const auto invalid_owner=world.CreateEndpoint({1,99},9);Reject([&]{(void)orientation.Update(0,invalid_owner,1);});world.DestroyHelper(invalid_owner);
    world.DestroyHelper(a);Reject([&]{(void)orientation.Update(0,a,1);});world.Close();Reject([&]{(void)orientation.Update(0,b,1);});
    // Caches are application-owned and survive replacing the scene service.
    Check(caches.kinds[0].guard==3 && caches.kinds[1].guard==3 && caches.random.guard==3);Near(caches.kinds[0].second_radius,5.06f);
}
}
int main()try{PrimaryFloor();Orientation();std::cout<<"monster primary floor, original cache/slot order and actor transform contracts passed; no gameplay executed\n";return 0;}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
