#include "pal4/behavior_path_endpoint_helper.h"
#include "pal4/grid_vector_statics.h"

#include <bit>
#include <cmath>

namespace pal4 {
namespace {

constexpr float kRandScale = 3.0518509447574615e-05f;

void StoreVector(BehaviorPathEndpointState& state, std::uint32_t first_word,
                 const BehaviorVector3& value) {
    for (std::uint32_t i = 0u; i != 3u; ++i)
        state.words[first_word + i] = std::bit_cast<std::uint32_t>(value[i]);
}

BehaviorVector3 GenerateRandomPoint(IBehaviorPathInitializationRuntime& runtime) {
    class RandomAdapter final : public IGridRandomRuntime {
    public:
        explicit RandomAdapter(IBehaviorPathInitializationRuntime& source) : source_(source) {}
        std::int32_t Rand() override { return static_cast<std::int32_t>(source_.Rand()); }
    private:
        IBehaviorPathInitializationRuntime& source_;
    } random(runtime);
    BehaviorVector3 output{};
    (void)GridVectorStatics::RandomInsideUnitSphere(output, random);
    return output;
}

BehaviorVector3 NormalizeHorizontal(const BehaviorVector3& value) {
    BehaviorVector3 result{value[0], 0.0f, value[2]};
    const float magnitude = std::sqrt(result[0] * result[0] +
                                      result[2] * result[2]);
    if (magnitude != 0.0f) {
        const float reciprocal = 1.0f / magnitude;
        result[0] *= reciprocal;
        result[2] *= reciprocal;
    }
    return result;
}

}  // namespace

BehaviorVector3 BehaviorPathInitializationRuntimeStub::ReadGlobalVector(
    std::uint32_t) { return {}; }
void BehaviorPathInitializationRuntimeStub::CallVoidSlot(
    BehaviorPathEndpointState&, std::uint32_t) {}
void BehaviorPathInitializationRuntimeStub::CallScalarSlot(
    BehaviorPathEndpointState&, std::uint32_t, std::uint32_t) {}
void BehaviorPathInitializationRuntimeStub::CallVectorSlot0C(
    BehaviorPathEndpointState&, BehaviorVector3& output,
    const BehaviorVector3& input) { output = input; }
void BehaviorPathInitializationRuntimeStub::CallVectorSlot14(
    BehaviorPathEndpointState&, BehaviorContext3&, const BehaviorVector3&) {}
std::uint32_t BehaviorPathInitializationRuntimeStub::CallContextSlot10(
    BehaviorPathEndpointState&, BehaviorContext3&) { return 0u; }
BehaviorVector3 BehaviorPathInitializationRuntimeStub::CallTokenSlot48(
    BehaviorPathEndpointState&, BehaviorVector3& output, std::uint32_t) {
    return output;
}
void BehaviorPathInitializationRuntimeStub::CallVectorSlot04(
    BehaviorPathEndpointState&, BehaviorVector3& output,
    const BehaviorVector3& input) { output = input; }
std::uint32_t BehaviorPathInitializationRuntimeStub::Rand() { return 16384u; }
std::uint32_t BehaviorPathInitializationRuntimeStub::Read32(
    std::uint32_t) { return 0u; }
std::uint32_t BehaviorPathInitializationRuntimeStub::CallContextSlot18(
    BehaviorPathEndpointState&, BehaviorContext3&) { return 0u; }
std::uint32_t BehaviorPathInitializationRuntimeStub::CallTargetSlot04(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }

std::uint32_t BehaviorPathEndpointHelper::InitializeMovement(
    BehaviorPathEndpointState& state,
    IBehaviorPathInitializationRuntime& runtime) {
    runtime.CallVoidSlot(state, 0x24u);
    state.words[13] = 0u;
    state.words[14] = 0u;
    state.words[21] &= 0xffffff00u;
    runtime.CallScalarSlot(state, 0x54u, 0x3f800000u);
    runtime.CallScalarSlot(state, 0x68u, 0u);
    runtime.CallScalarSlot(state, 0x5cu, 0x3f000000u);
    runtime.CallScalarSlot(state, 0x74u, 0x3dcccccdu);
    runtime.CallScalarSlot(state, 0x7cu, 0x3f800000u);

    const auto default_vector = runtime.ReadGlobalVector(0x93cf6cu);
    StoreVector(state, 35u, default_vector);
    StoreVector(state, 29u, default_vector);
    StoreVector(state, 32u, default_vector);
    state.words[28] = 0u;
    state.words[38] = 0u;
    StoreVector(state, 39u, default_vector);

    BehaviorVector3 scratch = default_vector;
    runtime.CallVectorSlot0C(
        state, scratch, runtime.ReadGlobalVector(0x93cf60u));
    const auto direction = NormalizeHorizontal(GenerateRandomPoint(runtime));
    BehaviorContext3 context{};
    runtime.CallVectorSlot14(state, context, direction);
    const auto token = runtime.CallContextSlot10(state, context);
    BehaviorVector3 resolved_output{};
    const auto resolved = runtime.CallTokenSlot48(
        state, resolved_output, token);
    runtime.CallVectorSlot04(state, scratch, resolved);

    const double sample = static_cast<double>(runtime.Rand()) * kRandScale;
    state.words[48] = sample <= 0.5 ? 1u : 0xffffffffu;
    const auto captured_vtable = runtime.Read32(state.words[42]);
    const auto value = runtime.CallContextSlot18(state, context);
    return runtime.CallTargetSlot04(
        captured_vtable, state.words[42], value);
}

}  // namespace pal4
