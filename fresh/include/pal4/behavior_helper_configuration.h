#pragma once

#include "pal4/behavior_path_endpoint_helper.h"
#include "pal4/global_render_state.h"
#include "pal4/grid_vector_statics.h"
#include "pal4/object_transform_basis.h"


namespace pal4 {
class IBehaviorHelperConfigurationBindings {
public:
    virtual ~IBehaviorHelperConfigurationBindings()=default;
    virtual std::uint32_t ReadWord(std::uint32_t address)=0;
    // Bind uses the actual helper image, not a padded temporary. On failure,
    // publish the remaining association in helper.words[42] before throwing.
    virtual void Bind(BehaviorPathEndpointState& helper,std::uint32_t wrapper)=0;
    virtual std::uint32_t UpdatePosition(std::uint32_t association,const BehaviorVector3& position)=0;
    virtual std::uint32_t Rand()=0;
};

// Shared 42A500/42A580 and base 42C3E0/42C7F0/42C410 composition.
// Supports all four verified helper vtables. Path and grid images are borrowed;
// the helper owns its association through the supplied memory/runtime.
// The caller supplies the shared CRT random stream (values 0..32767).
class BehaviorHelperConfiguration final :
    private IBehaviorPathEndpointConfigurationRuntime,
    private IBehaviorPathInitializationRuntime,
    private IObjectOrientationFlagRuntime,
    private IGlobalRenderStateRuntime {
public:
    explicit BehaviorHelperConfiguration(IBehaviorHelperConfigurationBindings& bindings);
    [[nodiscard]] std::uint32_t Configure(BehaviorPathEndpointState& helper,
        std::uint32_t wrapper, std::uint32_t path);
    static void ConstructEndpoint(BehaviorPathEndpointState&,std::uint32_t identifier);
    // Canonical E0 image; random lifecycle is composed through a typed copy.
    static void ConstructRandom(BehaviorPathEndpointState&,std::uint32_t identifier,IGridRandomRuntime&);
    static void SetScalar(ObjectMovementState&,std::uint32_t slot,float value);
    static void SetScalar(BehaviorPathEndpointState&,std::uint32_t slot,float value);
    // Verified vector slots also used by C03 restore and collision update.
    [[nodiscard]] static BehaviorVector3 ReadVector(
        const BehaviorPathEndpointState& helper, std::uint32_t slot);
    static void SetVector(BehaviorPathEndpointState& helper,
                           std::uint32_t slot, const BehaviorVector3& value);

private:
    class BaseRuntime;
    std::uint32_t ConfigureImage(BehaviorPathEndpointState&,std::uint32_t wrapper,std::uint32_t path);
    std::uint32_t Read32(std::uint32_t address) override;
    void ApplyPathParameter(BehaviorPathEndpointState& helper,
                            std::uint32_t wrapper) override;
    std::uint32_t Activate(BehaviorPathEndpointState& helper) override;
    BehaviorVector3 ReadGlobalVector(std::uint32_t address) override;
    void CallVoidSlot(BehaviorPathEndpointState& helper, std::uint32_t slot) override;
    void CallScalarSlot(BehaviorPathEndpointState& helper,
        std::uint32_t slot, std::uint32_t bits) override;
    void CallVectorSlot0C(BehaviorPathEndpointState& helper,
        BehaviorVector3& output, const BehaviorVector3& input) override;
    void CallVectorSlot14(BehaviorPathEndpointState& helper,
        BehaviorContext3& output, const BehaviorVector3& input) override;
    std::uint32_t CallContextSlot10(BehaviorPathEndpointState& helper,
                                    BehaviorContext3& output) override;
    BehaviorVector3 CallTokenSlot48(BehaviorPathEndpointState& helper,
        BehaviorVector3& output, std::uint32_t token) override;
    void CallVectorSlot04(BehaviorPathEndpointState& helper,
        BehaviorVector3& output, const BehaviorVector3& input) override;
    std::uint32_t Rand() override;
    std::uint32_t CallContextSlot18(BehaviorPathEndpointState& helper,
                                    BehaviorContext3& output) override;
    std::uint32_t CallTargetSlot04(std::uint32_t captured_vtable,
        std::uint32_t target, std::uint32_t position) override;
    std::uint32_t OrientationFlag(ObjectMovementState& helper) override;
    std::array<std::uint32_t, 3> TransformDirection(const GlobalRenderState& state,
        const std::array<std::uint32_t, 3>& source) override;
    std::uint32_t CommitDirection(GlobalRenderState&,
        const std::array<float, 3>&) override;
    IBehaviorHelperConfigurationBindings& bindings_;
    BehaviorVector3 context_direction_{},context_position_{};
    bool running_{};
    GridVectorStaticsState vectors_{};
};
}  // namespace pal4
