#include "pal4/native_behavior_movement.h"
#include "pal4/native_behavior_curve_movement.h"
#include "pal4/grid_node_object.h"
#include "pal4/native_behavior_grid_elements.h"
#include "pal4/native_behavior_helper_configuration.h"
#include "pal4/native_behavior_grid_association_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_npc_manager_owner.h"
#include "pal4/pathway_grid.h"
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace {
void Check(bool value,const char* message) { if(!value)throw std::runtime_error(message); }
template<class T> T& At(std::uint32_t address) { return *reinterpret_cast<T*>(address); }
class Paths final : public pal4::IPathwayGridRuntime {
public:
    explicit Paths(pal4::NativeBaseObjectMemory& memory):memory_(memory) {}
    std::uint32_t Allocate(std::uint32_t bytes) override { return memory_.Allocate(bytes); }
    void Free(std::uint32_t address) override { (void)memory_.Free(address); }
    std::uint32_t ReadWord(std::uint32_t address) override { return memory_.ReadDword(address); }
    void WriteWord(std::uint32_t address,std::uint32_t value) override { memory_.WriteDword(address,value); }
private:
    pal4::NativeBaseObjectMemory& memory_;
};
}
int main() {
    try {
        pal4::NativeBaseObjectMemory memory;
        pal4::NativeNpcManagerOwner manager;
        (void)manager.Construct();
        Check(manager.InitializeSceneGrid()!=0u,"native grid initialized");
        pal4::NativeBehaviorGridAssociationRuntime associations(memory);
        unsigned draws=0u;
        std::uint32_t sample=16384u;
        bool walking=false;
        const auto random=[&] {
            constexpr std::array<std::uint32_t,6> stream{16384u,24575u,8192u,16384u,8192u,24575u};
            const auto value=walking?stream[draws%stream.size()]:sample;
            ++draws;return value;
        };
        std::uint32_t identifier=0u;
        pal4::NativeBehaviorHelperStorage storage(memory,identifier,
            [&](std::uint32_t node,bool free_self){associations.DestroyAssociation(node,free_self);},random);
        pal4::NativeBehaviorHelperConfiguration configuration(memory,associations,random);
        Paths paths(memory);
        const auto path=memory.Allocate(sizeof(pal4::PathwayGridState));
        auto& grid=At<pal4::PathwayGridState>(path);
        grid={};
        (void)pal4::PathwayGrid::Construct(grid,true);
        constexpr std::array points{pal4::PathwayPoint{{0.f,0.f,0.f}},pal4::PathwayPoint{{0.f,0.f,800.f}}};
        Check(pal4::PathwayGrid::Initialize(grid,points,std::bit_cast<std::uint32_t>(35.f),false,paths),"native path initialized");
        const auto baseline=memory.OutstandingAllocations();
        using Vectors=pal4::NativeBehaviorHelperConfiguration;
        float maximum_speed=100.f,maximum_force=50.f;
        const auto create=[&](int kind,const pal4::BehaviorVector3& point,float radius,float speed,
                              const pal4::BehaviorVector3& heading) {
            const auto helper=memory.Allocate(pal4::NativeBehaviorHelperStorage::ImageSize(kind));
            storage.Construct(kind,helper);
            (void)configuration.Configure(helper,manager.GridWrapperAddress(),kind<2 || kind==4?path:0u);
            auto& image=At<pal4::BehaviorPathEndpointState>(helper);
            Vectors::SetVector(image,0x04u,{1.f,0.f,0.f});
            Vectors::SetVector(image,0x0cu,{0.f,1.f,0.f});
            Vectors::SetVector(image,0x14u,heading);
            Vectors::SetVector(image,0x1cu,point);
            pal4::NativeBehaviorHelperStorage::SetScalar(image,0x5cu,radius);
            pal4::NativeBehaviorHelperStorage::SetScalar(image,0x68u,speed);
            pal4::NativeBehaviorHelperStorage::SetScalar(image,0x74u,maximum_force);
            pal4::NativeBehaviorHelperStorage::SetScalar(image,0x7cu,maximum_speed);
            if(kind<2) {
                image.words[48]=1u;
                memory.WriteByte(helper+0xc4u,0u);
                memory.WriteByte(helper+0xc5u,1u);
            }
            (void)associations.UpdatePosition(image.words[42],point);
            memory.WriteByte(helper+0xb8u,1u);
            return helper;
        };
        pal4::NativeBehaviorMovement movement(associations,random);
        pal4::NativeBehaviorMovementResult last;
        auto services=std::make_shared<pal4::NativeBehaviorGridElements>(storage,
            [&](std::uint32_t helper,std::uint32_t context,std::uint32_t elapsed) {
                Check(context==manager.Address()+0x48u,"native manager forwards actual secondary context");
                last=movement.UpdatePath(helper,manager.GridObstacleCandidates(),std::bit_cast<float>(elapsed));
            });
        manager.BindGridElements(services);
        const auto self=create(0,{0.f,0.f,100.f},0.2f,100.f,{0.f,0.f,1.f});
        // The original saved behavior enables wander. Exactly collinear motion
        // with wander disabled is a degenerate low-speed direction constraint.
        walking=true;
        memory.WriteByte(self+0xc4u,1u);
        Check(manager.AddGridElement(self),"native endpoint registered with real manager");
        float minimum=100.f,maximum=100.f;
        bool reversed=false,returned=false;
        for(unsigned frame=0;frame<2400;++frame) {
            (void)manager.ProcessWorldObjects(1.f/60.f,0u,memory,
                [](std::uint32_t,float,std::uint32_t){throw std::logic_error("unexpected object-tree entry");});
            const auto point=Vectors::ReadVector(At<pal4::BehaviorPathEndpointState>(self),0x18u);
            minimum=std::fmin(minimum,point[2]);maximum=std::fmax(maximum,point[2]);
            reversed|=memory.ReadDword(self+0xc0u)==0xffffffffu;
            returned|=reversed && point[2]<100.f;
            Check(std::isfinite(point[2]) && std::isfinite(point[0]) && point[1]==0.f &&
                  std::bit_cast<float>(memory.ReadDword(self+100u))<=100.001f,"real path integration finite and speed-capped");
        }
        std::printf("Native endpoint manager2400 frames: z range=%g..%g, flips=%u queries=%u\n",
            minimum,maximum,movement.Stats().direction_flips,movement.Stats().sphere_queries);
        Check(last.active && movement.Stats().frames==2400u && movement.Stats().sphere_queries==2400u &&
              movement.Stats().path_corrections==2400u && movement.Stats().direction_flips>=3u &&
              maximum>765.f && minimum<100.f && reversed && returned,"generated path steering repeatedly reaches endpoints and returns");
        Check(manager.RemoveGridElement(self) && memory.OutstandingAllocations()==baseline,"native path deleting ownership has no growth");
        walking=false;
        maximum_speed=1.f;maximum_force=4.f;

        // Static sphere notification changes direction before outer endpoint check.
        grid.kind=std::bit_cast<std::uint32_t>(0.1f);
        auto helper=create(1,{0.f,0.f,0.f},0.5f,1.f,{0.f,0.f,1.f});
        auto& image=At<pal4::BehaviorPathEndpointState>(helper);
        for(unsigned axis=0;axis<3;++axis) {
            image.words[50+axis]=std::bit_cast<std::uint32_t>(-100.f);
            image.words[53+axis]=std::bit_cast<std::uint32_t>(pal4::BehaviorVector3{0.4f,0.f,2.f}[axis]);
        }
        const std::array<float,5> sphere{0.f,0.5f,0.4f,0.f,2.f};
        const std::array<std::uint32_t,1> obstacles{reinterpret_cast<std::uint32_t>(sphere.data())};
        pal4::NativeBehaviorMovement static_movement(associations,random);
        const auto obstacle_result=static_movement.UpdatePath(helper,obstacles,0.f);
        Check(static_movement.Stats().candidate_hits==1u && obstacle_result.requested[0]<0.f &&
              image.words[48]==0xffffffffu,"endpoint80 uses actual sphere position to flip cached endpoint direction");
        (void)storage.Delete(helper);

        helper=create(0,{0.f,0.f,3.f},1.f,1.f,{0.f,0.f,1.f});
        auto other=create(2,{0.4f,0.f,3.f},1.f,0.f,{0.f,0.f,1.f});
        auto& overlap=At<pal4::BehaviorPathEndpointState>(helper);
        for(unsigned axis=0;axis<3;++axis) {
            overlap.words[50+axis]=std::bit_cast<std::uint32_t>(-100.f);
            overlap.words[53+axis]=std::bit_cast<std::uint32_t>(pal4::BehaviorVector3{0.4f,0.f,3.f}[axis]);
        }
        pal4::NativeBehaviorMovement proximity(associations,random);
        const auto overlap_result=proximity.UpdatePath(helper,{},0.f);
        Check(proximity.Stats().proximity==1u && overlap_result.requested[0]<0.f && overlap.words[48]==0xffffffffu,
              "endpoint88 reads actual BC neighbor and changes direction without outer endpoint contact");
        (void)storage.Delete(other);(void)storage.Delete(helper);

        helper=create(0,{0.f,0.f,0.f},0.2f,1.f,{0.f,0.f,1.f});
        other=create(2,{0.f,0.f,4.f},0.2f,1.f,{0.f,0.f,-1.f});
        auto& predicted=At<pal4::BehaviorPathEndpointState>(helper);
        for(unsigned axis=0;axis<3;++axis) {
            predicted.words[50+axis]=std::bit_cast<std::uint32_t>(-100.f);
            predicted.words[53+axis]=std::bit_cast<std::uint32_t>(pal4::BehaviorVector3{0.f,0.f,2.f}[axis]);
        }
        pal4::NativeBehaviorMovement prediction(associations,random);
        const auto prediction_result=prediction.UpdatePath(helper,{},0.f);
        Check(prediction.Stats().predicted==1u && prediction_result.requested[0]>0.f && predicted.words[48]==0xffffffffu,
              "endpoint8C uses projected-other position for original direction notification");
        (void)storage.Delete(other);(void)storage.Delete(helper);

        helper=create(1,{0.f,0.f,3.f},0.2f,1.f,{0.f,0.f,1.f});
        memory.WriteByte(helper+0xc5u,0u); // Original unclamped path variant.
        memory.WriteByte(helper+0xc4u,1u);
        sample=32767u;
        pal4::NativeBehaviorMovement wandering(associations,random);
        const auto drift=wandering.UpdatePath(helper,{},0.1f);
        Check(wandering.Stats().drift_updates==2u && wandering.Stats().random_draws==4u &&
              drift.requested[0]>0.f && drift.requested[1]==0.f && wandering.Stats().path_corrections==1u,
              "wander reuses shared random clamp and right/up basis before final XZ projection");
        memory.WriteDword(helper+0xbcu,0u);
        memory.WriteByte(helper+0xb9u,1u);
        const auto count=wandering.Stats().random_draws;
        Check(!wandering.UpdatePath(helper,{},0.1f).active && memory.ReadByte(helper+0xb9u)==0u &&
              wandering.Stats().random_draws==count,"missing path clears B9 without RNG or physics");
        (void)storage.Delete(helper);
        helper=create(4,{5.f,0.f,3.f},0.2f,1.f,{0.f,0.f,1.f});
        pal4::NativeBehaviorCurveMovement::Update(helper,1.f,associations);
        const auto curve_position=Vectors::ReadVector(At<pal4::BehaviorPathEndpointState>(helper),0x18u);
        const auto& curve_node=At<pal4::GridNodeObjectState>(memory.ReadDword(helper+0xa8u));
        Check(curve_position[0]>4.f && curve_position[2]>3.f && curve_position[2]<4.f &&
              curve_node.node.position_bits==std::bit_cast<std::array<std::uint32_t,3>>(curve_position) &&
              memory.ReadByte(helper+0xb9u)==1u,
              "C0 curve helper notifies actual next position, not its off-path target point");
        memory.WriteDword(helper+0xbcu,0u);
        pal4::NativeBehaviorCurveMovement::Update(helper,1.f,associations);
        Check(memory.ReadByte(helper+0xb9u)==0u,"curve missing path clears activity");
        (void)storage.Delete(helper);
        manager.BindGridElements({});
        manager.Destroy();
        (void)pal4::PathwayGrid::Destroy(grid,paths);
        (void)memory.Free(path);
        Check(memory.OutstandingAllocations()==0u && manager.OutstandingAllocations()==0u,"path movement no native leaks");
        std::puts("PASS: native generated endpoint walking/turns, static/proximity/predicted notifications, drift and no-path gate");
        return 0;
    } catch(const std::exception& error) {
        std::fprintf(stderr,"FAIL: %s\n",error.what());return 1;
    }
}
