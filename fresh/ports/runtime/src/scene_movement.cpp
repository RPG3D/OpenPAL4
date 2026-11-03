#include "pal4/portable/scene_movement.h"
#include "pal4/pal_movement_adapter.h"
#include "pal4/config_composite_collision.h"
#include <cmath>
#include <bit>
#include <stdexcept>
namespace pal4::portable {
struct SceneMovement::Impl final:pal4::PalMovementAdapterRuntimeStub {
    std::array<std::span<const CollisionTriangle>,2> geometry;
    pal4::PalMovementAdapterState state{{1,2,1,0,0}};
    Point3 position{};
    std::array<std::optional<FloorHit>,2> hits;
    explicit Impl(std::span<const CollisionTriangle> first,std::span<const CollisionTriangle> second):geometry{first,second}{}
    unsigned Index(std::uint32_t child){if(child<1 || child>2){throw std::logic_error("invalid movement child handle");}return child-1;}
    void Validate(std::uint32_t position_handle,std::uint32_t above,std::uint32_t below) {
        if(position_handle!=0x1000)throw std::logic_error("invalid movement position handle");
        // Also validates finite coordinates/ranges for empty geometry.
        (void)FindFloorFromAbove({},position,std::bit_cast<float>(above),std::bit_cast<float>(below));
    }
    std::uint32_t ProbeChild(std::uint32_t child,std::uint32_t flag0,std::uint32_t flag1,std::uint32_t point,std::uint32_t above,std::uint32_t below,pal4::PalMovementProbeScratch& scratch) override {
        if(flag0!=1 || flag1!=1)throw std::logic_error("invalid movement probe flags");
        Validate(point,above,below);const auto i=Index(child);
        hits[i]=FindFloorFromAbove(geometry[i],position,std::bit_cast<float>(above),std::bit_cast<float>(below));
        if(!hits[i])return 0;
        scratch={std::bit_cast<std::uint32_t>(hits[i]->position[0]),hits[i]->position[1],std::bit_cast<std::uint32_t>(hits[i]->position[2])};return 1;
    }
    std::uint32_t CanSwitch(std::uint32_t child,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) override {(void)Index(child);return 1;}
    std::uint32_t UpdateChild(std::uint32_t child,std::uint32_t,std::uint32_t,std::uint32_t) override {(void)Index(child);return 1;}
    void DeactivateChild(std::uint32_t child) override {(void)Index(child);}
    void ActivateChild(std::uint32_t child) override {(void)Index(child);}
    std::uint32_t ProbeActive(std::uint32_t child,std::uint32_t flag0,std::uint32_t flag1,const std::array<std::uint32_t,4>& args) override {
        if(flag0 || flag1 || (args[3] && args[3]!=0x1004))throw std::logic_error("invalid active movement probe flags");
        Validate(args[0],args[1],args[2]);const auto i=Index(child);
        hits[i]=FindFloorFromAbove(geometry[i],position,std::bit_cast<float>(args[1]),std::bit_cast<float>(args[2]));
        if(hits[i] && args[3])position=hits[i]->position;
        return hits[i].has_value();
    }
};
SceneMovement::SceneMovement(std::span<const CollisionTriangle> primary,std::span<const CollisionTriangle> secondary):impl_(std::make_unique<Impl>(primary,secondary)){}
SceneMovement::~SceneMovement()=default;
bool SceneMovement::SelectAndUpdate(Point3 position,float above,float below) {
    impl_->position=position;
    return pal4::PalMovementAdapter::SelectAndUpdate(impl_->state,0x1000,std::bit_cast<std::uint32_t>(above),std::bit_cast<std::uint32_t>(below),1,*impl_)!=0;
}
bool SceneMovement::CheckActive(Point3 position,float above,float below) {
    impl_->position=position;
    return pal4::PalMovementAdapter::CheckActive(impl_->state,{0x1000,std::bit_cast<std::uint32_t>(above),std::bit_cast<std::uint32_t>(below),0},*impl_);
}
bool SceneMovement::CheckAndAdjust(Point3& position,float above,float below) {
    impl_->position=position;
    const bool result=pal4::PalMovementAdapter::CheckActive(impl_->state,{0x1000,std::bit_cast<std::uint32_t>(above),std::bit_cast<std::uint32_t>(below),0x1004},*impl_);
    if(result)position=impl_->position;
    return result;
}
unsigned SceneMovement::ActiveChild() const{return impl_->state.words[2]-1;}
bool SceneMovement::CheckPrimaryAndAdjust(Point3& position,float above,float below){
    // Validate even an absent primary clump at the portable boundary.
    (void)FindFloorFromAbove({},position,above,below);
    class Floor final:public pal4::IConfigFloorClumpCollisionRuntime {
        std::span<const CollisionTriangle> geometry_;
    public:
        explicit Floor(std::span<const CollisionTriangle> geometry):geometry_(geometry){}
        std::uint32_t ProcessClumpCollision(const pal4::CollisionSegment& segment,std::uint32_t clump,pal4::ConfigCollisionScratch& scratch) override {
            if(clump!=1 || segment.start[0]!=segment.end[0] || segment.start[2]!=segment.end[2])throw std::logic_error("invalid primary floor segment");
            const float length=segment.start[1]-segment.end[1];
            if(!std::isfinite(length) || length<0)throw std::invalid_argument("invalid primary floor extent");
            const auto hit=FindFloorFromAbove(geometry_,segment.start,0,length);if(!hit)return 0;
            scratch[4]=std::bit_cast<std::uint32_t>(hit->position[1]);return 1;
        }
    } runtime(impl_->geometry[0]);
    pal4::ConfigCompositeState state{};state.words[13]=impl_->geometry[0].empty()?0:1;
    return pal4::ConfigCompositeCollision::CheckFloorClumpIntersection(state,position,above,below,&position,nullptr,nullptr,runtime)!=0;
}
std::optional<FloorHit> SceneMovement::LastHit() const{return impl_->hits[ActiveChild()];}
}
