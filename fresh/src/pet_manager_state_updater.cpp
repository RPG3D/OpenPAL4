#include "pal4/pet_manager_state_updater.h"

#include <bit>

namespace pal4 {

std::uint32_t PetManagerUpdateRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
void PetManagerUpdateRuntimeStub::UpdateAndRenderClump(float) {}
void PetManagerUpdateRuntimeStub::UpdateInterpolator(
    std::uint32_t, float) {}
std::uint32_t PetManagerUpdateRuntimeStub::UpdateMovement(
    std::uint32_t, float) { return 0u; }
void PetManagerUpdateRuntimeStub::UpdateEmbeddedMotion(
    std::uint32_t, std::uint32_t, float, float) {}
float PetManagerUpdateRuntimeStub::ReadD3DVectorComponent(
    std::uint32_t, std::uint32_t) { return 0.0f; }
PetVectorView PetManagerUpdateRuntimeStub::ReadCsbVector(
    std::uint32_t) { return {}; }
void PetManagerUpdateRuntimeStub::ClampHorizontalDirection(
    PetVector3&) {}
double PetManagerUpdateRuntimeStub::NormalizeFloatRange(float) {
    return 0.0;
}
void PetManagerUpdateRuntimeStub::SetFacingTarget(
    std::uint32_t, const PetVector3&, std::uint32_t) {}
std::uint32_t PetManagerUpdateRuntimeStub::SetPosition(
    std::uint32_t, const PetVector3&) { return 0u; }
std::uint32_t PetManagerUpdateRuntimeStub::ZeroDirectionReturn(
    std::uint32_t) { return 0u; }

std::uint32_t PetManagerStateUpdater::Update(
    std::uint32_t pet, float elapsed, std::uint32_t argument,
    IPetManagerUpdateRuntime& runtime) {
    if (runtime.ReadDword(pet + 0x654u) == 0u) return 0u;
    if (runtime.ReadDword(pet + 0x1f0u) == 0u) return 0u;

    runtime.UpdateAndRenderClump(elapsed);
    runtime.UpdateInterpolator(pet + 0x170u, elapsed);
    if (runtime.ReadDword(pet + 0x660u) == 0u)
        return runtime.UpdateMovement(pet, elapsed);

    const auto embedded = pet + kEmbeddedOffset;
    runtime.UpdateEmbeddedMotion(embedded, argument, elapsed, 1.0f);
    const auto held_object = runtime.ReadDword(pet + 0x5f4u);
    if (held_object != 0u) return held_object;

    PetVector3 position{
        runtime.ReadD3DVectorComponent(embedded, 0u),
        runtime.ReadD3DVectorComponent(embedded, 1u),
        runtime.ReadD3DVectorComponent(embedded, 2u)};
    PetVector3 facing{
        std::bit_cast<float>(runtime.ReadDword(pet + 0xa0u)),
        std::bit_cast<float>(runtime.ReadDword(pet + 0xa4u)),
        std::bit_cast<float>(runtime.ReadDword(pet + 0xa8u))};

    auto vector = runtime.ReadCsbVector(embedded);
    if (vector.value.x == 0.0f) {
        vector = runtime.ReadCsbVector(embedded);
        if (vector.value.z == 0.0f)
            return runtime.ZeroDirectionReturn(vector.address);
    }

    const bool reverse = runtime.ReadDword(pet + 0x604u) != 0u;
    PetVector3 direction{};
    vector = runtime.ReadCsbVector(embedded);
    direction.x = reverse ? -vector.value.x : vector.value.x;
    vector = runtime.ReadCsbVector(embedded);
    direction.y = reverse ? -vector.value.y : vector.value.y;
    vector = runtime.ReadCsbVector(embedded);
    direction.z = reverse ? -vector.value.z : vector.value.z;
    direction.y = 0.0f;
    runtime.ClampHorizontalDirection(direction);

    double angle = runtime.NormalizeFloatRange(direction.z);
    if (direction.x < 0.0f) angle = kFullTurn - angle;
    facing.y = static_cast<float>(angle * kRadiansToDegrees);
    runtime.SetFacingTarget(pet, facing, 0u);
    position.y += std::bit_cast<float>(runtime.ReadDword(pet + 0x664u));
    return runtime.SetPosition(pet, position);
}

}  // namespace pal4
