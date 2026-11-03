#include "pal4/behavior_helper_physics.h"
#include "pal4/comparison_helper_interpolation_trace.h"
#include "pal4/grid_manager.h"
#include "pal4/object_movement_vectors.h"
#include "pal4/object_movement_scalars.h"
#include "pal4/object_transform_basis.h"
#include <bit>
#include <cmath>
#include <algorithm>
#include <span>
#include <stdexcept>

namespace pal4 {
namespace {
constexpr float kRequestedVectorToken=1.f; // Checked call token; never a host address.
ObjectMovementState Movement(const GridManagerState& state) {
    ObjectMovementState image{};
    const auto bytes=std::as_bytes(std::span{&state,1u});
    std::ranges::copy(bytes,std::as_writable_bytes(std::span{&image,1u}).begin());
    return image;
}
void Store(GridManagerState& state,const ObjectMovementState& image) {
    const auto bytes=std::as_bytes(std::span{&image,1u});
    std::copy_n(bytes.begin(),sizeof(state),std::as_writable_bytes(std::span{&state,1u}).begin());
}
class Runtime final : public IGridManagerMotionRuntime,
    private IGridManagerDirectionRuntime,private IGridManagerVelocityRuntime,
    private IObjectMovementScalarRuntime,private IObjectTransformRuntime {
public:
    explicit Runtime(const BehaviorVector3& requested,BehaviorHelperPhysics::Power power=nullptr)
        :requested_(requested),power_(power) {}
    GridVector3 CalculateAcceleration(GridManagerState& state,float input,float elapsed) override {
        if(std::bit_cast<std::uint32_t>(input)!=
            std::bit_cast<std::uint32_t>(kRequestedVectorToken))
            throw std::invalid_argument("helper physics input token");
        GridVector3 output{};
        GetComparisonHelperInterpolationTrace().Add(4u,state.base_words[22u],elapsed,
            {std::bit_cast<float>(state.base_words[10u]),std::bit_cast<float>(state.base_words[11u]),std::bit_cast<float>(state.base_words[12u])},
            {std::bit_cast<float>(state.base_words[39u]),std::bit_cast<float>(state.base_words[40u]),std::bit_cast<float>(state.base_words[41u])},
            {std::bit_cast<float>(state.base_words[1u]),std::bit_cast<float>(state.base_words[2u]),std::bit_cast<float>(state.base_words[3u])});
        (void)GridManager::ConstrainVelocity(state,output,requested_,*this);
        GetComparisonHelperInterpolationTrace().Add(3u,state.base_words[22u],
            std::bit_cast<float>(state.base_words[25u]),requested_,
            ReadSlot10VectorFrom(Movement(state)),output);
        return output;
    }
    float ReadScalar(GridManagerState& state,std::uint32_t slot) override {
        const auto image=Movement(state);
        switch(slot) {
        case 0x50u:return ObjectMovementScalars::Get23(image);
        case 0x64u:return ObjectMovementScalars::Get25(image);
        case 0x70u:return ObjectMovementScalars::Get26(image);
        case 0x78u:return ObjectMovementScalars::Get27(image);
        default:throw std::invalid_argument("helper physics scalar slot");
        }
    }
    GridVector3 ReadVelocity(GridManagerState& state) override {
        auto image=Movement(state);
        BehaviorVector3 output{};
        (void)ObjectMovementScalars::ScaledSlot10Vector(image,output,*this);
        return output;
    }
    GridVector3 ReadPosition(GridManagerState& state) override {
        BehaviorVector3 output{};
        (void)ObjectMovementVectors::CopyAt40(Movement(state),output);
        return output;
    }
    GridVector3 ReadNormal(GridManagerState& state) override { return ReadSlot10VectorFrom(Movement(state)); }
    GridVector3 ReadOrientation(GridManagerState& state) override {
        BehaviorVector3 output{};
        (void)ObjectMovementVectors::CopyAt4(Movement(state),output);
        return output;
    }
    void SetSpeed(GridManagerState& state,float speed) override {
        auto image=Movement(state);
        (void)ObjectMovementScalars::Set25(image,speed);
        Store(state,image);
    }
    void SetPosition(GridManagerState& state,const GridVector3& position) override {
        auto image=Movement(state);
        BehaviorVector3 output{};
        (void)ObjectMovementVectors::SetAt40(image,output,position);
        Store(state,image);
    }
    void CommitVelocity(GridManagerState& state,const GridVector3& velocity,float) override {
        (void)GridManager::SetVelocityDirection(state,velocity,*this);
    }
private:
    static GridVector3 ReadSlot10VectorFrom(const ObjectMovementState& image) {
        BehaviorVector3 output{};
        (void)ObjectMovementVectors::CopyAt28(image,output);
        return output;
    }
    float ReadParameter(const GridManagerState& state,std::uint32_t slot) override {
        // Existing getters do not mutate state. The shared interface for the
        // motion path is mutable; use a bounded local copy for this const view.
        auto copy=state;
        return ReadScalar(copy,slot);
    }
    GridVector3 ReadNormal(const GridManagerState& state) override { return ReadSlot10VectorFrom(Movement(state)); }
    GridVector3 ReadZeroVector() override { return {}; }
    double Power(double base,double exponent) override {
        return power_?power_(base,exponent):std::pow(base,exponent);
    }
    float ReadSpeed(GridManagerState& state) override { return ReadScalar(state,0x64u); }
    std::uint32_t ApplyDirection(GridManagerState& state,const GridVector3& direction) override {
        auto image=Movement(state);
        const auto result=ObjectTransformBasis::SetThirdAxis(image,direction,*this);
        Store(state,image);
        return result;
    }
    BehaviorVector3 ReadSlot10Vector(ObjectMovementState& state) override { return ReadSlot10VectorFrom(state); }
    std::uint32_t OrientationFlag(ObjectMovementState& state) override {
        if(state.words[0]!=0x842898u && state.words[0]!=0x842ae8u)
            throw std::invalid_argument("helper physics basis vtable");
        return 0u; // Both original +20 entries use the shared false callback.
    }
    void PrepareAxis(ObjectMovementState& state) override {
        (void)ObjectTransformBasis::NormalizeFirstAxis(state,*this);
    }
    std::uint32_t ProcessNormalized(ObjectMovementState&,const BehaviorVector3&) override {
        throw std::logic_error("physics does not use normalized input dispatch");
    }
    void MakeDifference(ObjectMovementState&,BehaviorVector3&,const BehaviorVector3&) override {
        throw std::logic_error("physics does not use transform difference");
    }
    void TransformDifference(ObjectMovementState&,BehaviorVector3&,const BehaviorVector3&) override {
        throw std::logic_error("physics does not use transformed difference");
    }
    void ProcessTransformed(ObjectMovementState&,BehaviorVector3&,const BehaviorVector3&) override {
        throw std::logic_error("physics does not use transform output");
    }
    const BehaviorVector3& requested_;
    BehaviorHelperPhysics::Power power_;
};
}
BehaviorVector3 BehaviorHelperPhysics::PredictPosition(const GridManagerState& state,float elapsed) {
    if(!std::isfinite(elapsed) || elapsed<0.f)
        throw std::invalid_argument("helper physics prediction frame");
    if(state.base_words[0]!=0x842898u && state.base_words[0]!=0x842ae8u)
        throw std::invalid_argument("helper physics prediction vtable");
    auto copy=state;
    const BehaviorVector3 unused{};
    Runtime runtime(unused);
    return GridManager::CalculatePosition(copy,elapsed,runtime);
}
void BehaviorHelperPhysics::Integrate(GridManagerState& state,const BehaviorVector3& requested,float elapsed,
                                     Power power) {
    if(!std::isfinite(elapsed) || elapsed<0.f)
        throw std::invalid_argument("helper physics frame");
    for(const auto component:requested) if(!std::isfinite(component))
        throw std::invalid_argument("helper physics requested vector");
    if(state.base_words[0]!=0x842898u && state.base_words[0]!=0x842ae8u)
        throw std::invalid_argument("helper physics vtable");
    Runtime runtime(requested,power);
    (void)GridManager::UpdatePositionAndVelocity(state,kRequestedVectorToken,elapsed,runtime);
}
}
