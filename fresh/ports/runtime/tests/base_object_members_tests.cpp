#include "pal4/base_object_member_runtime.h"
#include "pal4/portable/object_memory.h"
#include "pal4/portable/surface_motion.h"
#include "pal4/portable/monster_base_motion.h"
#include "pal4/portable/actor_playback.h"
#include "pal4/portable/borrowed_actor_base_members.h"
#include "pal4/collision_actor.h"
#include "../../../tests/support/rw_dict_schema_fake.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <bit>
namespace {
void Check(bool b){if(!b)throw std::runtime_error("base member contract failed");}
struct Schema final:pal4::test::RwDictSchemaFake {
    unsigned resets{};std::uint32_t SetCurrentDict(std::uint32_t id) override{Check(id==0);++resets;return 0;}
};
struct Animation final:pal4::IGameObjectEmbeddedAnimationRuntime {
    unsigned initialized{},cleaned{};
    void InitializeAnimationData(pal4::GameObjectEmbeddedAnimationState&) override{++initialized;}
    void CleanupAnimationData(pal4::GameObjectEmbeddedAnimationState&) override{++cleaned;}
    void CleanupAnimationDataRange(pal4::GameObjectEmbeddedAnimationState& s) override{Check(s.words[21]==0 && s.words[22]==0 && s.words[23]==0);}
    std::uint32_t AnimationArrayCount(std::uint32_t) override{throw std::logic_error("unexpected animation array");}
    void DestructAnimationArray(std::uint32_t,std::uint32_t,std::uint32_t) override{throw std::logic_error("unexpected animation data");}
    std::uint32_t FreeMemory(std::uint32_t id) override{Check(id==0);return 0;}
    void FreeWrappedMemory(std::uint32_t) override{throw std::logic_error("unexpected animation buffer");}
    std::uint32_t FindHierarchy(std::uint32_t) override{throw std::logic_error("construction cannot sample animation");}
    void ProcessAnimationData(pal4::GameObjectEmbeddedAnimationState&,std::uint32_t,std::int32_t) override{throw std::logic_error("construction cannot update animation");}
};
void ConstructAndDestroy(){
    pal4::portable::ObjectMemory memory;Schema schema;Animation animation;
    pal4::BaseObjectState a{},b{};a.words.fill(0xdeadbeef);b.words.fill(0xdeadbeef);
    pal4::BaseObjectMemberRuntime members(memory,schema,animation,memory,memory.Path(),[&](auto& s){return &s==&a?0x20000u:0x30000u;});
    for(auto* state:{&a,&b}){
        const auto token=state==&a?0x20000u:0x30000u;
        Check(pal4::BaseObjectLifecycle::Construct(*state,token,members)==token);
        for(auto off:{0u,0x218u,0x388u}){Check(state->Read(off)==(off?0x84b8c8u:0x8433d8u));Check(state->Read(off+39*4)==0x3f800000);}
        Check(state->Read(0x20c)==token && state->Read(0x214)==token);
        for(auto off:{0x544u,0x554u}){Check(memory.ReadByte(state->Read(off))==0 && state->Read(off+4)==0 && state->Read(off+8)==32);}
        const auto sentinel=state->Read(0x564+21*4);Check(memory.ReadDword(sentinel)==sentinel && memory.ReadDword(sentinel+4)==sentinel);
        Check((state->Read(0x170+80)&0xff)==(((token+0x170+80)>>8)&0xff));
        Check(state->Read(0x170+12*4)==0x3f800000 && state->Read(0x564+29*4)==0x40a00000);
        for(auto off:{0x520u,0x528u,0x534u,0x53cu})Check(state->Read(off)==0xffffffff && state->Read(off+4)==0);
    }
    Check(memory.AllocationCount()==6 && animation.initialized==2);const auto b_before=b.words;
    {
        using namespace pal4::portable;
        const std::vector<CollisionTriangle> floor{{{Point3{0,0,0},Point3{10,0,0},Point3{0,0,10}},0,0,0},{{Point3{10,0,0},Point3{10,0,10},Point3{0,0,10}},0,0,1}};
        CollisionSurface surface(floor);surface.EnableNavigation();
        SurfaceMotion motion(surface,{1,0,1},1,[](auto,auto){return false;},0x40000,&memory);
        Check(memory.AllocationCount()==7);Check(motion.Start({1,0,1},{9,0,9},2));motion.SetSpeed(5);
        for(unsigned frame=0;motion.Active() && frame<1000;++frame)motion.Advance(.05f);
        Check(!motion.Active());motion.Close();Check(memory.AllocationCount()==6 && b.words==b_before);
        const auto collision_offset=0x564/4;SurfaceMotionImage member;
        std::copy_n(a.words.begin()+collision_offset,member.size(),member.begin());const auto sentinel=member[21];
        {
            SurfaceMotion borrowed(surface,{1,0,1},1,[](auto,auto){return false;},0x20564,&memory,member);
            Check(memory.AllocationCount()==6 && borrowed.Image()[21]==sentinel);
            Check(borrowed.Start({1,0,1},{9,0,9},2));borrowed.SetSpeed(5);
            for(unsigned frame=0;borrowed.Active() && frame<1000;++frame)borrowed.Advance(.05f);
            Check(!borrowed.Active() && borrowed.Position()==Point3{9,0,9});borrowed.Close();borrowed.Close();
            member=borrowed.Image();std::copy(member.begin(),member.end(),a.words.begin()+collision_offset);
            Check(member[21]==sentinel && member[1]==0 && memory.AllocationCount()>6 && b.words==b_before);
        }
        // 借用退出仍保留原路径；下面真正的BaseObject析构统一释放。
        const auto retained_allocations=memory.AllocationCount();CollisionSurface empty({});bool threw{};
        try{SurfaceMotion invalid(empty,{0,0,0},1,[](auto,auto){return false;},0x50000,&memory);}catch(const std::logic_error&){threw=true;}
        Check(threw && memory.AllocationCount()==retained_allocations);
    }
    pal4::BaseObjectLifecycle::Destroy(a,members);Check(memory.AllocationCount()==3 && b.words==b_before);
    pal4::BaseObjectLifecycle::Destroy(b,members);Check((memory.AllocationCount()==0) && animation.cleaned==2 && schema.resets==6);
}
void AllocationFailures(){
    for(unsigned budget:{16u,32u,64u}){
        pal4::portable::ObjectMemory memory({0x10000000,0x70000000,budget,100});
        Schema schema;Animation animation;pal4::BaseObjectState state{};
        pal4::BaseObjectMemberRuntime members(memory,schema,animation,memory,memory.Path(),[](auto&){return 0x20000u;});bool threw{};
        try{(void)pal4::BaseObjectLifecycle::Construct(state,0x20000,members);}catch(const std::length_error&){threw=true;}
        Check(threw && memory.AllocationCount()==0 && memory.AllocatedBytes()==0 && schema.resets==3 && animation.cleaned==1);
    }
}
void InvalidIdentities(){
    pal4::portable::ObjectMemory memory;Schema schema;Animation animation;pal4::BaseObjectState state{};
    pal4::BaseObjectMemberRuntime valid(memory,schema,animation,memory,memory.Path(),[](auto&){return 0x20000u;});
    for(auto offset:{1u,pal4::BaseObjectState::kSize-4}){bool threw{};try{valid.ConstructText(state,offset);}catch(const std::invalid_argument&){threw=true;}Check(threw && (memory.AllocationCount()==0));}
    for(auto id:{0u,1u,0xfffffffcu}){
        pal4::BaseObjectMemberRuntime members(memory,schema,animation,memory,memory.Path(),[=](auto&){return id;});bool threw{};
        try{members.ConstructText(state,0x544);}catch(const std::invalid_argument&){threw=true;}
        Check(threw && (memory.AllocationCount()==0));
    }
}
template<class F>void Reject(F f){bool threw{};try{f();}catch(const std::exception&){threw=true;}Check(threw);}
void PlaybackAndMotionLifetime(){
    using namespace pal4::portable;
    ObjectMemory memory;pal4::BaseObjectState base{};
    BorrowedActorBaseMembers members(memory,[](auto&){return 0x20000u;});
    (void)members.Construct(base,0x20000);Check(memory.AllocationCount()==3);
    std::array<std::uint32_t,31> original_animation;std::copy_n(base.words.begin()+0x170/4,31,original_animation.begin());
    Actor model;const Matrix identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};model.frames.push_back({-1,identity});model.palettes.push_back({0,identity});
    model.mesh.vertices.resize(3);model.influences.resize(3,{{0,0,0,0},{1,0,0,0}});model.atomic_spheres.push_back({0,{0,0,0,1}});
    model.clips.push_back({"Z01",2,{{0,{{0,{0,0,0,1},{0,0,0}},{2,{0,0,0,1},{2,0,0}}}}}});
    SceneMonster record;record.name="first";record.resource="401";record.position={1,0,1};
    SceneMonsterInstances actors(1,std::span<const SceneMonster>(&record,1),[&](auto){return model;},[](auto)->std::optional<std::vector<std::uint8_t>>{return {};});
    const auto actor=actors.Handle(0);Check(actors.SelectAction(actor,"Z01"));
    const std::vector<CollisionTriangle> geometry{{{Point3{0,0,0},Point3{10,0,0},Point3{0,0,10}},0,0,0},{{Point3{10,0,0},Point3{10,0,10},Point3{0,0,10}},0,0,1}};
    CollisionSurface floor(geometry);floor.EnableNavigation();pal4::MonsterBehaviorManagerState state{};
    std::copy(base.words.begin(),base.words.end(),state.words.begin());state.words[5]=0x30000;state.words[43]=state.words[45]=std::bit_cast<std::uint32_t>(1.f);state.words[0x1f0/4]=1;
    const auto sentinel=state.words[0x564/4+21];
    {
        MonsterBaseMotion motion(actors,actor,state,0x20000,floor,1,[](auto,auto){return false;},[](auto&,float){},&memory);
        Check(motion.Start({1,0,1},{9,0,9},2));motion.SetSpeed(5);(void)motion.Update(.1f,1);
        Check(state.words[0x170/4+2]!=0 && state.words[0x564/4+21]==sentinel && memory.AllocationCount()>3);
        std::copy_n(state.words.begin(),base.words.size(),base.words.begin());const auto running=base.words;const auto allocated=memory.AllocationCount();
        Reject([&]{members.Destroy(base);});Check(base.words==running && memory.AllocationCount()==allocated);
        Check(motion.Close() && motion.Close());
        Check(std::equal(original_animation.begin(),original_animation.end(),state.words.begin()+0x170/4));
        Reject([&]{(void)motion.Update(0,1);});Reject([&]{motion.SetSpeed(1);});Reject([&]{motion.Stop();});
    }
    Check(!actors.TransformBound(actor));std::copy_n(state.words.begin(),base.words.size(),base.words.begin());base.words[5]=0; // 结束借用的主体身份。
    members.Destroy(base);Check(memory.AllocationCount()==0);
    // 原对象已销毁，播放器继续独立运行，不再访问旧图像。
    actors.AdvanceAnimation(actor,.1f);Check(actors.AnimationTime(actor)>.1f);
}
void BorrowedBaseMemberBoundaries(){
    using namespace pal4::portable;ObjectMemory memory;pal4::BaseObjectState first{},second{};
    std::uint32_t second_token=0x20000u;BorrowedActorBaseMembers members(memory,[&](auto& state){return &state==&first?0x20000u:second_token;});
    Reject([&]{members.Destroy(first);});Reject([&]{(void)members.Construct(first,0);});Check(memory.AllocationCount()==0);
    (void)members.Construct(first,0x20000);const auto constructed=first.words;
    Reject([&]{(void)members.Construct(first,0x20000);});Reject([&]{(void)members.Construct(second,0x20000);});Check(first.words==constructed && memory.AllocationCount()==3);
    second_token=0x20004;Reject([&]{(void)members.Construct(second,second_token);});Check(memory.AllocationCount()==3);
    second_token=0x30000;(void)members.Construct(second,second_token);Check(memory.AllocationCount()==6);members.Destroy(second);Check(memory.AllocationCount()==3);
    for(unsigned offset:{37*4u,38*4u,11*4u,0x218u+12*4u,0x388u+13*4u,0x218u+37*4u,0x388u+38*4u,0x170u+2*4u,0x170u+7*4u,0x170u+21*4u}){
        first.Write(offset,1);const auto invalid=first.words;Reject([&]{members.Destroy(first);});Check(first.words==invalid && memory.AllocationCount()==3);first.words=constructed;
    }
    members.Destroy(first);Check(memory.AllocationCount()==0);Reject([&]{members.Destroy(first);});
    first={};(void)members.Construct(first,0x20000);members.Destroy(first);Check(memory.AllocationCount()==0);
    ObjectMemory limited({0x10000000,0x70000000,32,100});BorrowedActorBaseMembers failure(limited,[](auto&){return 0x20000u;});
    for(unsigned attempt=0;attempt<2;++attempt){pal4::BaseObjectState image{};bool failed{};try{(void)failure.Construct(image,0x20000);}catch(const std::length_error&){failed=true;}Check(failed && limited.AllocationCount()==0);}
}
void BorrowedFailures(){
    using namespace pal4::portable;ObjectMemory memory;pal4::CollisionActorState member{};
    (void)pal4::CollisionActor::Construct(member,0,memory.Path());const auto original=member.words;
    const std::vector<CollisionTriangle> floor{{{Point3{0,0,0},Point3{10,0,0},Point3{0,0,10}},0,0,0}};
    CollisionSurface surface(floor),empty({});surface.EnableNavigation();
    for(unsigned field:{0u,1u,2u,10u,21u,22u}){
        auto invalid=original;invalid[field]=field==21?0:1;
        Reject([&]{SurfaceMotion motion(surface,{1,0,1},1,[](auto,auto){return false;},0x20564,&memory,invalid);});
        Check(member.words==original && memory.AllocationCount()==1);
    }
    Reject([&]{SurfaceMotion motion(empty,{1,0,1},1,[](auto,auto){return false;},0x20564,&memory,original);});
    Reject([&]{SurfaceMotion motion(surface,{1,0,1},1,[](auto,auto){return false;},0x20564,nullptr,original);});
    ObjectMemory foreign;Reject([&]{SurfaceMotion motion(surface,{1,0,1},1,[](auto,auto){return false;},0x20564,&foreign,original);});
    Check(member.words==original && memory.AllocationCount()==1 && foreign.AllocationCount()==0);
    (void)pal4::CollisionActor::Destroy(member,memory.Path());Check(memory.AllocationCount()==0);
}
void StorageBoundaries(){
    using pal4::portable::ObjectMemory;
    ObjectMemory memory({0x10000000,0x10000100,32,2});
    const auto first=memory.Allocate(8),second=memory.Allocate(0);Check(second==first+8 && memory.AllocatedBytes()==9);
    memory.WriteDword(first,0x12345678);Check(memory.ReadByte(first)==0x78 && memory.ReadDword(first)==0x12345678);
    memory.WriteByte(first+3,0xab);Check(memory.ReadDword(first)==0xab345678);
    Reject([&]{(void)memory.ReadDword(first+1);});Reject([&]{memory.WriteDword(first+6,0);});
    Reject([&]{(void)memory.ReadByte(second+1);});Reject([&]{(void)memory.Allocate(1);});Reject([&]{(void)memory.Free(first+4);});
    Check(memory.Free(first)==1 && memory.Free(0)==0);Reject([&]{(void)memory.ReadByte(first);});Reject([&]{(void)memory.Free(first);});
    const auto third=memory.Allocate(8);Check(third>second && memory.ReadDword(third)==0);
    (void)memory.Free(second);(void)memory.Free(third);Check(memory.AllocatedBytes()==0);
    Reject([&]{(void)memory.Allocate(33);});
    ObjectMemory tiny({0x10000000,0x10000008,1024,100});const auto full=tiny.Allocate(8);(void)tiny.Free(full);Reject([&]{(void)tiny.Allocate(1);});
    Reject([]{ObjectMemory bad({1,0x100,1024,100});});
}

}
int main()try{StorageBoundaries();ConstructAndDestroy();AllocationFailures();InvalidIdentities();BorrowedFailures();PlaybackAndMotionLifetime();BorrowedBaseMemberBoundaries();std::cout<<"shared object storage, base member lifetime, path movement and allocation rollback contracts passed; no gameplay executed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
