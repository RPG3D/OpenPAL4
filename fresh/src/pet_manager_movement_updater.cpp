#include "pal4/pet_manager_movement_updater.h"

#include <bit>

namespace pal4 {
namespace {

float ReadFloat(IPetManagerMovementRuntime& runtime,
                std::uint32_t address) {
    return std::bit_cast<float>(runtime.ReadDword(address));
}

}  // namespace

std::uint32_t PetManagerMovementRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void PetManagerMovementRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
std::uint32_t PetManagerMovementRuntimeStub::ActorControl() { return 0u; }
std::uint32_t PetManagerMovementRuntimeStub::ActorMode(
    std::uint32_t) { return 0u; }
double PetManagerMovementRuntimeStub::Magnitude(
    const PetVector3&) { return 0.0; }
void PetManagerMovementRuntimeStub::ClampDirection(PetVector3&) {}
double PetManagerMovementRuntimeStub::NormalizeFloatRange(float) {
    return 0.0;
}
void PetManagerMovementRuntimeStub::SetFacingTarget(
    std::uint32_t, const PetVector3&, std::uint32_t) {}
void PetManagerMovementRuntimeStub::SetPosition(
    std::uint32_t, const PetVector3&) {}
std::string_view PetManagerMovementRuntimeStub::AnimationName(
    std::uint32_t) { return {}; }
int PetManagerMovementRuntimeStub::CompareNames(
    std::string_view, std::string_view) { return 0; }
std::uint32_t PetManagerMovementRuntimeStub::SelectAnimation(
    std::uint32_t, std::string_view) { return 0u; }

std::uint32_t PetManagerMovementUpdater::EnsureAnimation(
    std::uint32_t pet, std::string_view name,
    IPetManagerMovementRuntime& runtime) {
    const auto animation = pet + 0x170u;
    const auto current = runtime.AnimationName(animation);
    const auto difference = runtime.CompareNames(current, name);
    if (difference == 0) return 0u;
    return runtime.SelectAnimation(animation, name);
}

std::uint32_t PetManagerMovementUpdater::Update(
    std::uint32_t pet, float elapsed, PetMovementScratch& scratch,
    IPetManagerMovementRuntime& runtime) {
    const auto source = runtime.ReadDword(pet + 0x658u);
    if (source == 0u) return 0u;
    if (runtime.ReadDword(source + 0x1f0u) == 0u) return source;

    const auto control = runtime.ActorControl();
    switch (runtime.ActorMode(control)) {
    case 0u:
        runtime.WriteDword(pet + 0x670u, 0x42a00000u);
        break;
    case 1u:
        runtime.WriteDword(pet + 0x670u, 0x43290000u);
        break;
    case 2u:
        runtime.WriteDword(pet + 0x670u, 0x43790000u);
        break;
    default:
        break;
    }

    const float pet_x = ReadFloat(runtime, pet + 0xacu);
    const auto active_source = runtime.ReadDword(pet + 0x658u);
    const float pet_z = ReadFloat(runtime, pet + 0xb4u);
    const float source_x = ReadFloat(runtime, active_source + 0xacu);
    const float source_z = ReadFloat(runtime, active_source + 0xb4u);
    scratch.direction = {pet_x - source_x, 0.0f, pet_z - source_z};

    const double distance = runtime.Magnitude(scratch.direction);
    const auto threshold_source = runtime.ReadDword(pet + 0x658u);
    const float threshold = ReadFloat(runtime, threshold_source + 0xecu);
    if (!(distance > threshold))
        return EnsureAnimation(pet, "C01", runtime);

    const double candidate_speed =
        (distance - threshold) * ReadFloat(runtime, pet + 0x668u);
    float speed = static_cast<float>(candidate_speed);
    const float maximum = ReadFloat(runtime, pet + 0x670u);
    if (candidate_speed > maximum) speed = maximum;
    if (speed < 1.0f) speed = 1.0f;

    const float target_x = ReadFloat(runtime, threshold_source + 0xacu);
    const float source_y = ReadFloat(runtime, threshold_source + 0xb0u);
    const float pet_y = ReadFloat(runtime, pet + 0xb0u);
    const float height = ReadFloat(runtime, pet + 0x664u);
    const float target_z = ReadFloat(runtime, threshold_source + 0xb4u);
    scratch.direction = {
        target_x - pet_x,
        static_cast<float>(static_cast<double>(source_y) + height - pet_y),
        target_z - pet_z};
    runtime.ClampDirection(scratch.direction);

    const double step = static_cast<double>(speed) * elapsed;
    scratch.next_position = {
        ReadFloat(runtime, pet + 0xacu) +
            static_cast<float>(scratch.direction.x * step),
        ReadFloat(runtime, pet + 0xb0u) +
            static_cast<float>(scratch.direction.y * step),
        ReadFloat(runtime, pet + 0xb4u) +
            static_cast<float>(scratch.direction.z * step)};
    double angle = runtime.NormalizeFloatRange(scratch.direction.z);
    if (scratch.direction.x < 0.0f) angle = kFullTurn - angle;
    const PetVector3 facing{
        0.0f, static_cast<float>(angle * kRadiansToDegrees), 0.0f};
    runtime.SetFacingTarget(pet, facing, 0u);
    runtime.SetPosition(pet, scratch.next_position);
    return EnsureAnimation(pet, "C02", runtime);
}

}  // namespace pal4
