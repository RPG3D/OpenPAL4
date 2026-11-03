#include "pal4/native_behavior_random_movement.h"
#include "pal4/native_behavior_grid_elements.h"
#include "pal4/native_behavior_helper_configuration.h"
#include "pal4/native_behavior_grid_association_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_npc_manager_owner.h"
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <limits>

namespace {
void Check(bool condition,const char* message) {
    if(!condition)throw std::runtime_error(message);
}
template<class T> T& At(std::uint32_t address) { return *reinterpret_cast<T*>(address); }
std::uint32_t Address(const void* pointer) { return reinterpret_cast<std::uint32_t>(pointer); }
}
int main() {
    try {
        pal4::NativeBaseObjectMemory memory;
        pal4::NativeNpcManagerOwner manager;
        (void)manager.Construct();
        Check(manager.InitializeSceneGrid()!=0u,"actual manager grid initialized");
        pal4::NativeBehaviorGridAssociationRuntime associations(memory);
        unsigned draws=0;
        const auto random=[&] {
            constexpr std::array<std::uint32_t,3> samples{16384u,24575u,8192u};
            return samples[draws++%samples.size()];
        };
        std::uint32_t identifier=0;
        pal4::NativeBehaviorHelperStorage storage(memory,identifier,
            [&](std::uint32_t node,bool free_self){associations.DestroyAssociation(node,free_self);},random);
        pal4::NativeBehaviorHelperConfiguration configuration(memory,associations,random);
        using Vectors=pal4::NativeBehaviorHelperConfiguration;
        const auto create=[&](int kind,const pal4::BehaviorVector3& position,float radius,float speed,
                              const pal4::BehaviorVector3& heading) {
            const auto helper=memory.Allocate(pal4::NativeBehaviorHelperStorage::ImageSize(kind));
            storage.Construct(kind,helper);
            (void)configuration.Configure(helper,manager.GridWrapperAddress(),0u);
            auto& image=At<pal4::BehaviorPathEndpointState>(helper);
            Vectors::SetVector(image,0x04u,{1.f,0.f,0.f});
            Vectors::SetVector(image,0x0cu,{0.f,1.f,0.f});
            Vectors::SetVector(image,0x14u,heading);
            Vectors::SetVector(image,0x1cu,position);
            pal4::NativeBehaviorHelperStorage::SetScalar(image,0x5cu,radius);
            pal4::NativeBehaviorHelperStorage::SetScalar(image,0x68u,speed);
            pal4::NativeBehaviorHelperStorage::SetScalar(image,0x74u,2.f);
            pal4::NativeBehaviorHelperStorage::SetScalar(image,0x7cu,1.f);
            if(kind==3) {
                image.words[47]=std::bit_cast<std::uint32_t>(20.f);
                for(unsigned i=0;i<3;++i)image.words[53+i]=std::bit_cast<std::uint32_t>(position[i]);
            }
            (void)associations.UpdatePosition(image.words[42],position);
            memory.WriteByte(helper+0xb8u,1u);
            return helper;
        };
        std::unique_ptr<pal4::NativeBehaviorRandomMovement> step;
        pal4::NativeBehaviorRandomMovementResult last;
        auto services=std::make_shared<pal4::NativeBehaviorGridElements>(storage,
            [&](std::uint32_t helper,std::uint32_t context,std::uint32_t elapsed) {
                Check(context==manager.Address()+0x48u,"actual frame forwards original secondary context");
                const auto obstacles=manager.GridObstacleCandidates();
                last=step->Update(helper,obstacles,std::bit_cast<float>(elapsed));
            });
        manager.BindGridElements(services);
        const auto frame=[&] {
            (void)manager.ProcessWorldObjects(1.f/60.f,0u,memory,
                [](std::uint32_t,float,std::uint32_t){throw std::logic_error("empty object tree dispatched");});
        };
        auto self=create(3,{0.f,0.f,0.f},0.5f,0.f,{0.f,0.f,1.f});
        Check(manager.AddGridElement(self),"register actual random helper for native manager frame");
        step=std::make_unique<pal4::NativeBehaviorRandomMovement>(associations,random);
        const auto initial_heading=std::array<std::uint32_t,3>{memory.ReadDword(self+0xc8u),
            memory.ReadDword(self+0xccu),memory.ReadDword(self+0xd0u)};
        for(unsigned i=0;i<200;++i)frame();
        auto& image=At<pal4::BehaviorPathEndpointState>(self);
        const auto point=Vectors::ReadVector(image,0x18u);
        const auto heading=std::array<std::uint32_t,3>{image.words[50],image.words[51],image.words[52]};
        Check(last.active && step->Stats().frames==200u && step->Stats().sphere_queries==200u &&
                  step->Stats().neighbor_visits>=200u && step->Stats().random_draws>400u,
              "native manager runs actual sphere queries, RNG gates and timed random heading updates");
        Check(std::fabs(point[0])+std::fabs(point[2])>0.1f && heading!=initial_heading &&
                  std::bit_cast<float>(image.words[25])<=1.0001f && memory.ReadByte(self+0xb9u)==1u,
              "original autonomous random steering generates real bounded movement and changed heading");
        Check(manager.RemoveGridElement(self) && memory.OutstandingAllocations()==0u,
              "native manager deleting callback frees random helper and actual association");

        // Static sphere-record input uses its original +4 radius/+8 position.
        self=create(3,{0.f,0.f,0.f},0.5f,1.f,{0.f,0.f,1.f});
        step=std::make_unique<pal4::NativeBehaviorRandomMovement>(associations,random);
        const std::array<std::uint32_t,5> sphere{0u,std::bit_cast<std::uint32_t>(0.5f),
            std::bit_cast<std::uint32_t>(0.4f),0u,std::bit_cast<std::uint32_t>(2.f)};
        const std::array<std::uint32_t,1> obstacles{Address(sphere.data())};
        last=step->Update(self,obstacles,1.f/60.f);
        Check(last.active && step->Stats().candidate_tests==1u && step->Stats().candidate_hits==1u &&
                  last.requested[0]<0.f,
              "actual sphere collision time produces lateral obstacle correction");
        (void)storage.Delete(self);

        // Stationary BC helper is read through bounded common getters.
        self=create(3,{0.f,0.f,0.f},1.f,1.f,{0.f,0.f,1.f});
        auto other=create(2,{0.5f,0.f,0.f},1.f,0.f,{0.f,0.f,1.f});
        step=std::make_unique<pal4::NativeBehaviorRandomMovement>(associations,random);
        last=step->Update(self,{},1.f/60.f);
        Check(step->Stats().neighbor_visits==2u && step->Stats().proximity==1u && last.requested[0]<0.f,
              "actual spatial query and strict-radius overlap generate native neighbor avoidance");
        (void)storage.Delete(other);
        (void)storage.Delete(self);

        self=create(3,{0.f,0.f,0.f},0.2f,1.f,{0.f,0.f,1.f});
        other=create(2,{0.f,0.f,4.f},0.2f,1.f,{0.f,0.f,-1.f});
        step=std::make_unique<pal4::NativeBehaviorRandomMovement>(associations,random);
        last=step->Update(self,{},1.f/60.f);
        Check(step->Stats().neighbor_visits==2u && step->Stats().proximity==0u &&
                  step->Stats().predicted==1u && last.requested[0]>0.f,
              "actual opposing neighbors use projected collision and heading-based side selection");
        (void)storage.Delete(other);
        (void)storage.Delete(self);

        self=create(3,{0.f,0.f,0.f},1.f,0.f,{0.f,0.f,1.f});
        memory.WriteDword(self+0xbcu,0u);
        Check(manager.AddGridElement(self),"register inactive-range helper");
        step=std::make_unique<pal4::NativeBehaviorRandomMovement>(associations,random);
        frame();
        Check(!last.active && !step->Stats().random_draws && !step->Stats().sphere_queries &&
                  Vectors::ReadVector(At<pal4::BehaviorPathEndpointState>(self),0x18u)==pal4::BehaviorVector3{},
              "original nonpositive range gate skips random, query and physics");
        memory.WriteDword(self+0xbcu,std::bit_cast<std::uint32_t>(std::numeric_limits<float>::quiet_NaN()));
        frame();
        Check(!last.active && !step->Stats().random_draws && !step->Stats().sphere_queries,
              "native adapter safety gate rejects NaN; original unordered behavior differs");
        Check(manager.RemoveGridElement(self),"inactive helper still uses owning deleting callback");
        manager.BindGridElements({});
        manager.Destroy();
        Check(memory.OutstandingAllocations()==0u && manager.OutstandingAllocations()==0u,
              "random steering/obstacle/neighbor frame tests leave no native allocations");
        std::puts("PASS: actual native random grid frames, timed turns, sphere obstacles and two neighbor-avoidance branches");
        return 0;
    } catch(const std::exception& error) {
        std::fprintf(stderr,"FAIL: %s\n",error.what());
        return 1;
    }
}
