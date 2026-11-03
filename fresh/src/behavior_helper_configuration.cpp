#include "pal4/behavior_helper_configuration.h"

#include "pal4/object_movement_scalars.h"
#include "pal4/object_movement_vectors.h"
#include "pal4/object_derived_lifecycle.h"
#include "pal4/object_movement_reset.h"
#include "pal4/grid_manager.h"
#include "pal4/object_random_movement_lifecycle.h"

#include <bit>
#include <algorithm>
#include <span>
#include <stdexcept>
#include <utility>


namespace pal4 {
namespace {
template<class To,class From> void Prefix(To& to,const From& from,std::size_t size) {
    const auto source=std::as_bytes(std::span{&from,1u});
    const auto target=std::as_writable_bytes(std::span{&to,1u});
    if(size>source.size() || size>target.size())throw std::logic_error("helper image prefix extent");
    std::copy_n(source.begin(),size,target.begin());
}
void Validate(const BehaviorPathEndpointState& helper) {
    const auto vtable = helper.words[0];
    if (vtable != 0x842898u && vtable != 0x842978u &&
        vtable != 0x842a30u && vtable != 0x842ae8u)
        throw std::invalid_argument("helper configuration vtable");
}
ObjectMovementState Movement(const BehaviorPathEndpointState& helper) {
    ObjectMovementState output{};
    Prefix(output,helper,sizeof(GridManagerState));
    return output;
}
GlobalRenderState Basis(const BehaviorPathEndpointState& helper) {
    GlobalRenderState output;
    for (auto word = 0u; word < 13u; ++word)
        output.WriteDword(word * 4u, helper.words[word]);
    return output;
}
void StoreBasis(BehaviorPathEndpointState& helper, const GlobalRenderState& basis) {
    for (auto word = 0u; word < 13u; ++word)
        helper.words[word] = basis.ReadDword(word * 4u);
}
BehaviorContext3 Bits(const BehaviorVector3& value) {
    return std::bit_cast<BehaviorContext3>(value);
}
}  // namespace

// Configuration for BC/C0/E0 non-endpoint images. The association payload
// always points at the actual allocation; padded state never escapes this call.
class BehaviorHelperConfiguration::BaseRuntime final :
    public IObjectExtendedRuntime, private IObjectMovementResetRuntime {
public:
    BaseRuntime(BehaviorHelperConfiguration& owner, BehaviorPathEndpointState& helper,
        ObjectMovementState& state) : owner_(owner), helper_(helper), state_(state) {}
    void ConstructBase(ObjectMovementState&) override { throw std::logic_error("configuration is not construction"); }
    void CleanupBase(ObjectMovementState&) override { throw std::logic_error("configuration is not cleanup"); }
    void Free(std::uint32_t) override { throw std::logic_error("configuration cannot free helper"); }
    void CallCapturedSlotB0(std::uint32_t vtable, std::uint32_t wrapper) override {
        if (vtable != 0x842978u && vtable != 0x842a30u && vtable != 0x842ae8u)
            throw std::invalid_argument("base configuration association vtable");
        try { owner_.bindings_.Bind(helper_, wrapper); }
        catch (...) { state_.words[42] = helper_.words[42]; throw; }
        state_.words[42] = helper_.words[42];
    }
    std::uint32_t CallCurrentSlotA0(ObjectMovementState& state) override {
        return ObjectMovementReset::Reset(state, *this);
    }
private:
    void CallSlot24(ObjectMovementState& state) override {
        BehaviorPathEndpointState image{};
        Prefix(image,state,sizeof(state));
        owner_.CallVoidSlot(image, 0x24u);
        Prefix(state,image,sizeof(state));
    }
    void CallScalarSetter(ObjectMovementState& state, std::uint32_t slot,
                          std::uint32_t bits) override {
        BehaviorPathEndpointState image{};
        Prefix(image,state,sizeof(state));
        owner_.CallScalarSlot(image, slot, bits);
        Prefix(state,image,sizeof(state));
    }
    std::array<std::uint32_t,3> GlobalVector() override { return {}; }
    std::uint32_t MemberVtable(std::uint32_t member) override { return owner_.Read32(member); }
    std::uint32_t CallSlot18(ObjectMovementState& state,
                            std::array<std::uint32_t,3>& output) override {
        BehaviorPathEndpointState image{};
        Prefix(image,state,sizeof(state));
        return owner_.CallContextSlot18(image, output);
    }
    std::uint32_t CallMemberSlot4(std::uint32_t vtable, std::uint32_t member,
                                  std::uint32_t token) override {
        return owner_.CallTargetSlot04(vtable, member, token);
    }
    BehaviorHelperConfiguration& owner_;
    BehaviorPathEndpointState& helper_;
    ObjectMovementState& state_;
};

namespace {
class Construction final:public IGridManagerRuntime,private IObjectOrientationFlagRuntime {
    std::uint32_t identifier_;
public:
    explicit Construction(std::uint32_t id):identifier_(id){}
    void Construct(BehaviorPathEndpointState& state) {
        GridManagerState image{};Prefix(image,state,sizeof(image));
        (void)GridManager::Construct(image,*this);Prefix(state,image,sizeof(image));
    }
    void Cleanup(BehaviorPathEndpointState& state) {
        GridManagerState image{};Prefix(image,state,sizeof(image));
        GridManager::Cleanup(image,*this);Prefix(state,image,sizeof(image));
    }
    std::uint32_t Free(std::uint32_t) override {throw std::logic_error("construction cannot free helper");}
private:
    std::array<std::uint32_t,3> TransformBasis(GridManagerState& state,const std::array<std::uint32_t,3>& source) override {
        ObjectMovementState image{};Prefix(image,state,sizeof(state));BehaviorVector3 output{};
        (void)ObjectTransformBasis::ReorderAndNegate(image,output,std::bit_cast<BehaviorVector3>(source),*this);return Bits(output);
    }
    void SetParameter(GridManagerState& state,std::uint32_t slot,std::uint32_t bits) override {
        ObjectMovementState image{};Prefix(image,state,sizeof(state));BehaviorHelperConfiguration::SetScalar(image,slot,std::bit_cast<float>(bits));Prefix(state,image,sizeof(state));
    }
    std::array<std::uint32_t,3> ZeroVector() override {return {};}
    std::uint32_t NextIdentifier() override {return identifier_;}
    void DestroyNested(std::uint32_t,bool) override {throw std::logic_error("construction cannot release association");}
    void FreeSelf(GridManagerState&) override {throw std::logic_error("construction cannot free grid");}
    std::uint32_t OrientationFlag(ObjectMovementState& state) override {
        if(state.words[0]!=0x84b780u && state.words[0]!=0x84b6e8u)
            throw std::invalid_argument("helper construction vtable");
        return 0;
    }
};
}
void BehaviorHelperConfiguration::ConstructEndpoint(BehaviorPathEndpointState& state,std::uint32_t identifier) {
    if(state.words[42] || state.words[43])throw std::invalid_argument("construct requires unowned helper storage");
    class Endpoint final:public IBehaviorPathEndpointLifecycleRuntime {
        Construction construction_;
    public:
        explicit Endpoint(std::uint32_t id):construction_(id){}
        void ConstructBase(BehaviorPathEndpointState& value) override {construction_.Construct(value);}
        void DestructBase(BehaviorPathEndpointState&) override {throw std::logic_error("construction cannot destruct helper");}
        void Free(std::uint32_t) override {throw std::logic_error("construction cannot free helper");}
    } runtime(identifier);
    (void)BehaviorPathEndpointHelper::Construct(state,runtime);
}
void BehaviorHelperConfiguration::ConstructRandom(BehaviorPathEndpointState& state,std::uint32_t identifier,IGridRandomRuntime& random) {
    if(state.words[42] || state.words[43])throw std::invalid_argument("construct requires unowned helper storage");
    class Random final:public IObjectRandomMovementRuntime {
        Construction construction_;IGridRandomRuntime& random_;
    public:
        Random(std::uint32_t id,IGridRandomRuntime& random):construction_(id),random_(random){}
        void ConstructBase(ObjectRandomMovementState& value) override {
            auto image=std::bit_cast<BehaviorPathEndpointState>(value);
            construction_.Construct(image);value=std::bit_cast<ObjectRandomMovementState>(image);
        }
        // A freshly constructed base has no association or external allocation.
        void CleanupBase(ObjectRandomMovementState& value) override {
            if(value.words[42] || value.words[43])throw std::logic_error("random constructor acquired unexpected ownership");
            auto image=std::bit_cast<BehaviorPathEndpointState>(value);
            construction_.Cleanup(image);value=std::bit_cast<ObjectRandomMovementState>(image);
        }
        void Free(std::uint32_t) override {throw std::logic_error("construction cannot free helper");}
        BehaviorVector3 RandomVector() override {
            GridVector3 output{};return GridVectorStatics::RandomInsideUnitSphere(output,random_);
        }
    } runtime(identifier,random);
    auto image=std::bit_cast<ObjectRandomMovementState>(state);
    (void)ObjectRandomMovementLifecycle::Construct(image,runtime);
    state=std::bit_cast<BehaviorPathEndpointState>(image);
}
BehaviorHelperConfiguration::BehaviorHelperConfiguration(IBehaviorHelperConfigurationBindings& bindings):bindings_(bindings) {
    GridVectorStatics::InitializeZero(vectors_);
    GridVectorStatics::InitializePositiveY(vectors_);
}
std::uint32_t BehaviorHelperConfiguration::Configure(BehaviorPathEndpointState& helper,std::uint32_t wrapper,std::uint32_t path) {
    if(running_)throw std::logic_error("helper configuration is not reentrant");
    running_=true;
    try {const auto result=ConfigureImage(helper,wrapper,path);running_=false;return result;}
    catch (...) {running_=false;throw;}
}
std::uint32_t BehaviorHelperConfiguration::ConfigureImage(
    BehaviorPathEndpointState& state, std::uint32_t wrapper, std::uint32_t path) {
    Validate(state);
    if (state.words[0] != BehaviorPathEndpointHelper::kVtable) {
        ObjectMovementState image{};
        // Only copy the common BA-byte base. Preserve original padding and
        // derived fields, including class3's random direction and start point.
        Prefix(image,state,sizeof(GridManagerState));
        if (state.words[0] == 0x842978u) image.words[47] = state.words[47];
        BaseRuntime runtime(*this, state, image);
        const auto store = [&] {
            Prefix(state,image,sizeof(GridManagerState));
            if (image.words[0] == 0x842978u) state.words[47] = image.words[47];
        };
        try {
            const auto result = image.words[0] == 0x842978u
                ? ObjectExtendedLifecycle::SetPropertyAndDispatch(image, wrapper, path, runtime)
                : ObjectDerivedLifecycle::Dispatch(image, wrapper, path, runtime);
            store();
            return result;
        } catch (...) { store(); throw; }
    }
    // Original zero-point paths dereference the preceding element. Reject
    // invalid borrowed images before mutating ownership in the host adapter.
    if (path && (!Read32(path + 8u) || !Read32(path + 12u)))
        throw std::invalid_argument("helper path has no points");
    return BehaviorPathEndpointHelper::SetPathAndCacheEndpoints(state, wrapper, path, *this);
}
std::uint32_t BehaviorHelperConfiguration::Read32(std::uint32_t address) {
    return bindings_.ReadWord(address);
}
void BehaviorHelperConfiguration::ApplyPathParameter(
    BehaviorPathEndpointState& helper, std::uint32_t wrapper) {
    bindings_.Bind(helper, wrapper);
}
std::uint32_t BehaviorHelperConfiguration::Activate(BehaviorPathEndpointState& helper) {
    return BehaviorPathEndpointHelper::InitializeMovement(helper, *this);
}
BehaviorVector3 BehaviorHelperConfiguration::ReadGlobalVector(std::uint32_t address) {
    if (address == 0x93cf6cu) return vectors_.zero;
    if (address == 0x93cf60u) return vectors_.positive_y;
    throw std::invalid_argument("helper initialization vector");
}
void BehaviorHelperConfiguration::CallVoidSlot(
    BehaviorPathEndpointState& helper, std::uint32_t slot) {
    if (slot != 0x24u) throw std::invalid_argument("helper initialization slot");
    auto basis = Basis(helper);
    (void)basis.InitializeDefaultBasis(*this);
    StoreBasis(helper, basis);
}
void BehaviorHelperConfiguration::CallScalarSlot(
    BehaviorPathEndpointState& helper, std::uint32_t slot, std::uint32_t bits) {
    SetScalar(helper, slot, std::bit_cast<float>(bits));
}
void BehaviorHelperConfiguration::SetScalar(ObjectMovementState& state,std::uint32_t slot,float value) {
    switch(slot){
    case 0x54u:(void)ObjectMovementScalars::Set23(state,value);break;
    case 0x5cu:(void)ObjectMovementScalars::Set24(state,value);break;
    case 0x68u:(void)ObjectMovementScalars::Set25(state,value);break;
    case 0x74u:(void)ObjectMovementScalars::Set26(state,value);break;
    case 0x7cu:(void)ObjectMovementScalars::Set27(state,value);break;
    default:throw std::invalid_argument("helper scalar slot");
    }
}
void BehaviorHelperConfiguration::SetScalar(BehaviorPathEndpointState& state,std::uint32_t slot,float value) {
    auto movement=Movement(state);SetScalar(movement,slot,value);Prefix(state,movement,sizeof(GridManagerState));
}
BehaviorVector3 BehaviorHelperConfiguration::ReadVector(
    const BehaviorPathEndpointState& helper, std::uint32_t slot) {
    Validate(helper);
    const auto movement = Movement(helper);
    BehaviorVector3 output{};
    switch (slot) {
    case 0x00u: (void)ObjectMovementVectors::CopyAt4(movement, output); break;
    case 0x08u: (void)ObjectMovementVectors::CopyAt16(movement, output); break;
    case 0x10u: (void)ObjectMovementVectors::CopyAt28(movement, output); break;
    case 0x18u: (void)ObjectMovementVectors::CopyAt40(movement, output); break;
    default: throw std::invalid_argument("helper vector getter slot");
    }
    return output;
}
void BehaviorHelperConfiguration::SetVector(
    BehaviorPathEndpointState& helper, std::uint32_t slot, const BehaviorVector3& value) {
    Validate(helper);
    if (slot == 0x0cu || slot == 0x14u) {
        auto basis = Basis(helper);
        BehaviorContext3 output{};
        if (slot == 0x0cu) (void)basis.SetBasisY(output, Bits(value));
        else (void)basis.SetBasisZ(output, Bits(value));
        StoreBasis(helper, basis);
        return;
    }
    auto movement = Movement(helper);
    BehaviorVector3 output{};
    if (slot == 0x04u) (void)ObjectMovementVectors::SetAt4(movement, output, value);
    else if (slot == 0x1cu) (void)ObjectMovementVectors::SetAt40(movement, output, value);
    else throw std::invalid_argument("helper vector setter slot");
    Prefix(helper,movement,sizeof(GridManagerState));
}
void BehaviorHelperConfiguration::CallVectorSlot0C(
    BehaviorPathEndpointState& helper, BehaviorVector3& output, const BehaviorVector3& input) {
    SetVector(helper, 0x0cu, input);
    output = ReadVector(helper, 0x08u);
}
void BehaviorHelperConfiguration::CallVectorSlot14(
    BehaviorPathEndpointState& helper, BehaviorContext3& output, const BehaviorVector3& input) {
    SetVector(helper, 0x14u, input);
    output = Bits(ReadVector(helper, 0x10u));
}
std::uint32_t BehaviorHelperConfiguration::CallContextSlot10(
    BehaviorPathEndpointState& helper, BehaviorContext3& output) {
    output = Bits(ReadVector(helper, 0x10u));
    context_direction_=std::bit_cast<BehaviorVector3>(output);
    return 1u;
}
BehaviorVector3 BehaviorHelperConfiguration::CallTokenSlot48(
    BehaviorPathEndpointState& helper, BehaviorVector3& output, std::uint32_t token) {
    if(token!=1u)throw std::invalid_argument("helper direction context token");
    auto movement = Movement(helper);
    return ObjectTransformBasis::ReorderAndNegate(movement, output,context_direction_, *this);
}
void BehaviorHelperConfiguration::CallVectorSlot04(
    BehaviorPathEndpointState& helper, BehaviorVector3& output, const BehaviorVector3& input) {
    SetVector(helper, 0x04u, input);
    output = ReadVector(helper, 0x00u);
}
std::uint32_t BehaviorHelperConfiguration::Rand() {
    const auto sample = bindings_.Rand();
    if (sample > 32767u) throw std::out_of_range("helper rand exceeds original range");
    return sample;
}
std::uint32_t BehaviorHelperConfiguration::CallContextSlot18(
    BehaviorPathEndpointState& helper, BehaviorContext3& output) {
    output = Bits(ReadVector(helper, 0x18u));
    context_position_=std::bit_cast<BehaviorVector3>(output);
    return 2u;
}
std::uint32_t BehaviorHelperConfiguration::CallTargetSlot04(
    std::uint32_t captured_vtable, std::uint32_t target, std::uint32_t position) {
    if (captured_vtable != 0x84b6c0u)
        throw std::invalid_argument("helper initialization association vtable");
    if(position!=2u)throw std::invalid_argument("helper position context token");
    return bindings_.UpdatePosition(target,context_position_);
}
std::uint32_t BehaviorHelperConfiguration::OrientationFlag(ObjectMovementState& helper) {
    BehaviorPathEndpointState image{};
    image.words[0] = helper.words[0];
    Validate(image);
    return 0u; // Verified shared false callback at 0x842898+0x20.
}
std::array<std::uint32_t, 3> BehaviorHelperConfiguration::TransformDirection(
    const GlobalRenderState& basis, const std::array<std::uint32_t, 3>& source) {
    ObjectMovementState movement{};
    for (auto word = 0u; word < 13u; ++word)
        movement.words[word] = basis.ReadDword(word * 4u);
    BehaviorVector3 output{};
    (void)ObjectTransformBasis::ReorderAndNegate(movement, output,
        std::bit_cast<BehaviorVector3>(source), *this);
    return Bits(output);
}
std::uint32_t BehaviorHelperConfiguration::CommitDirection(
    GlobalRenderState&, const std::array<float, 3>&) {
    throw std::logic_error("endpoint initialization does not call normalized-direction commit");
}
}  // namespace pal4
