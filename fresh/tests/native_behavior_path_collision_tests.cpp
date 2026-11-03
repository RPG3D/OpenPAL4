#include "pal4/native_behavior_path_collision.h"
#include "pal4/native_behavior_helper_physics.h"
#include "pal4/native_behavior_helper_configuration.h"
#include "pal4/native_behavior_helper_storage.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_world_path_planner_owner.h"
#include "pal4/pathway_grid.h"
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace {
void Check(bool condition,const char* message) { if(!condition)throw std::runtime_error(message); }
template<class T> T& At(std::uint32_t address) { return *reinterpret_cast<T*>(address); }
class PathMemory final : public pal4::IPathwayGridRuntime {
public:
    explicit PathMemory(pal4::NativeBaseObjectMemory& memory):memory_(memory) {}
    std::uint32_t Allocate(std::uint32_t bytes) override { return memory_.Allocate(bytes); }
    void Free(std::uint32_t address) override { (void)memory_.Free(address); }
    std::uint32_t ReadWord(std::uint32_t address) override { return memory_.ReadDword(address); }
    void WriteWord(std::uint32_t address,std::uint32_t value) override { memory_.WriteDword(address,value); }
private:
    pal4::NativeBaseObjectMemory& memory_;
};
}
int main(int argc,char** argv) {
    try {
        pal4::NativeBaseObjectMemory memory;
        PathMemory paths(memory);
        std::uint32_t identifier=0u;
        pal4::NativeBehaviorHelperStorage storage(memory,identifier,
            [](std::uint32_t,bool){throw std::logic_error("unconfigured helper has no association");},
            []{return 16384u;});
        const auto helper=memory.Allocate(pal4::NativeBehaviorHelperStorage::ImageSize(0));
        storage.Construct(0,helper);
        auto& image=At<pal4::BehaviorPathEndpointState>(helper);
        using Vectors=pal4::NativeBehaviorHelperConfiguration;
        Vectors::SetVector(image,0x14u,{1.f,0.f,0.f});
        Vectors::SetVector(image,0x1cu,{5.f,0.f,0.25f});
        pal4::NativeBehaviorHelperStorage::SetScalar(image,0x68u,1.f);
        const auto path=memory.Allocate(sizeof(pal4::PathwayGridState));
        auto& grid=At<pal4::PathwayGridState>(path);
        grid={};
        (void)pal4::PathwayGrid::Construct(grid,true);
        constexpr std::array points{pal4::PathwayPoint{{0.f,0.f,0.f}},
            pal4::PathwayPoint{{10.f,0.f,0.f}},pal4::PathwayPoint{{10.f,0.f,10.f}}};
        Check(pal4::PathwayGrid::Initialize(grid,points,std::bit_cast<std::uint32_t>(1.f),false,paths),"native tube initialized");
        image.words[47]=path;
        image.words[48]=1u;
        const auto before=image;
        Check(pal4::NativeBehaviorHelperPhysics::PredictPosition(helper,2.5f)==
                  pal4::BehaviorVector3{7.5f,0.f,0.25f},"slot6C extrapolates actual speed and forward");
        Check(pal4::NativeBehaviorPathCollision::Clamped(helper,2.5f)==pal4::BehaviorVector3{} &&
              pal4::NativeBehaviorPathCollision::Unclamped(helper,2.5f)==pal4::BehaviorVector3{},
              "forward motion inside tube needs no path correction");
        Check(std::memcmp(&before,&image,sizeof(image))==0,"queries preserve helper including derived tail");
        image.words[48]=0xffffffffu;
        Check(pal4::NativeBehaviorPathCollision::Clamped(helper,2.5f)==
                  pal4::BehaviorVector3{-3.5f,0.f,-0.25f},"opposing path direction generates reversal correction");
        image.words[48]=1u;
        Vectors::SetVector(image,0x1cu,{5.f,0.f,3.f});
        Check(pal4::NativeBehaviorPathCollision::Clamped(helper,2.5f)==pal4::BehaviorVector3{1.5f,0.f,-3.f} &&
              pal4::NativeBehaviorPathCollision::Unclamped(helper,2.5f)==pal4::BehaviorVector3{4.f,0.f,0.f},
              "corner: clamped follows current path measure; unclamped selects nearest later segment");
        Vectors::SetVector(image,0x1cu,{5.f,0.f,1.f});
        Check(pal4::NativeBehaviorPathCollision::Unclamped(helper,2.5f)==pal4::BehaviorVector3{1.5f,0.f,-1.f},
              "exact tube boundary is outside branch, not negative-inside early return");
        bool rejected=false;
        try { (void)pal4::NativeBehaviorPathCollision::Clamped(helper,-1.f); }
        catch(const std::invalid_argument&) { rejected=true; }
        Check(rejected,"invalid prediction rejected");
        grid.secondary_vtable=0u;
        rejected=false;
        try { (void)pal4::NativeBehaviorPathCollision::Clamped(helper,2.5f); }
        catch(const std::invalid_argument&) { rejected=true; }
        Check(rejected,"unbound secondary interface rejected");
        grid.secondary_vtable=0x84b5d4u;
        constexpr std::array single{pal4::PathwayPoint{{4.f,0.f,0.f}}};
        Check(pal4::PathwayGrid::Initialize(grid,single,std::bit_cast<std::uint32_t>(1.f),false,paths),
              "single-point original path has allocated arrays and zero segments");
        const auto single_before=image;
        const auto clamped=pal4::NativeBehaviorPathCollision::Clamped(helper,2.5f);
        const auto unclamped=pal4::NativeBehaviorPathCollision::Unclamped(helper,2.5f);
        for(unsigned axis=0;axis<3;++axis)
            Check(std::isfinite(clamped[axis]) && std::isfinite(unclamped[axis]),
                  "both native collision paths accept original single-point geometry");
        Check(std::memcmp(&single_before,&image,sizeof(image))==0,
              "single-point collision queries preserve helper ownership and state");
        grid.point_count=0u;
        rejected=false;
        try { (void)pal4::NativeBehaviorPathCollision::Clamped(helper,2.5f); }
        catch(const std::invalid_argument&) { rejected=true; }
        Check(rejected,"empty uninitialized geometry remains rejected");
        grid.point_count=1u;
        (void)pal4::PathwayGrid::Destroy(grid,paths);
        (void)memory.Free(path);

        if(argc>1) {
            pal4::NativeWorldPathPlannerOwner original;
            (void)original.Construct();
            Check(original.LoadScene(argv[1],"Q02","Q02"),"original Q02 pathways loaded");
            const auto original_path=original.FindPath("4064");
            Check(original_path!=0u,"original 4064 path found");
            auto& original_grid=At<pal4::PathwayGridState>(original_path);
            Check(original_grid.point_count==17u,"original 4064 has 17 points");
            const auto allocations=original.OutstandingAllocations();
            image.words[47]=original_path;
            unsigned queries=0u;
            for(unsigned index=1;index<original_grid.point_count;++index) {
                const auto first=At<pal4::PathwayPoint>(original_grid.positions+12u*(index-1u));
                const auto last=At<pal4::PathwayPoint>(original_grid.positions+12u*index);
                pal4::BehaviorVector3 midpoint{},heading{};
                for(unsigned axis=0;axis<3;++axis) {
                    midpoint[axis]=(first.coordinates[axis]+last.coordinates[axis])*0.5f;
                    heading[axis]=last.coordinates[axis]-first.coordinates[axis];
                }
                const auto length=std::sqrt(heading[0]*heading[0]+heading[1]*heading[1]+heading[2]*heading[2]);
                if(length<=0.f)continue;
                for(auto& component:heading)component/=length;
                Vectors::SetVector(image,0x1cu,midpoint);
                Vectors::SetVector(image,0x14u,heading);
                const auto saved=image;
                for(auto direction:{1u,0xffffffffu}) {
                    image.words[48]=direction;
                    const auto clamped=pal4::NativeBehaviorPathCollision::Clamped(helper,2.5f);
                    const auto unclamped=pal4::NativeBehaviorPathCollision::Unclamped(helper,2.5f);
                    for(unsigned axis=0;axis<3;++axis)
                        Check(std::isfinite(clamped[axis]) && std::isfinite(unclamped[axis]),"original path correction finite");
                    ++queries;
                }
                image.words[48]=saved.words[48];
                Check(std::memcmp(&saved,&image,sizeof(image))==0,"original path queries do not integrate helper");
            }
            Check(queries>0u && original.OutstandingAllocations()==allocations,"original borrowed queries allocate nothing");
            std::printf("Original Q02 path4064: %u bidirectional clamped/unclamped query pairs, radius=%g\n",
                queries,std::bit_cast<float>(original_grid.kind));
            original.Destroy();
            Check(original.OutstandingAllocations()==0u,"original planner cleanup");
        }
        image.words[47]=0u;
        (void)storage.Delete(helper);
        Check(memory.OutstandingAllocations()==0u,"native path collision zero leaks");
        std::puts("Native path collision tests passed");
        return 0;
    } catch(const std::exception& error) {
        std::fprintf(stderr,"Native path collision failure: %s\n",error.what());
        return 1;
    }
}
