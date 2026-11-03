#include "pal4/pal_movement_secondary_collision.h"

#include <cmath>
#include <limits>

namespace pal4 {
namespace {

struct ScanResult {
    std::uint32_t found = 0u;
    std::uint32_t element = 0u;
    float height = -std::numeric_limits<float>::max();
};

template <typename OnAccepted>
ScanResult ScanHighestClump(
    const PalMovementSecondaryChildState& state,
    const std::array<float, 3>& position,
    float upward_distance, float downward_distance,
    IPalMovementSecondaryCollisionRuntime& runtime,
    OnAccepted on_accepted) {
    const std::array<float, 6> line{
        position[0], position[1] + upward_distance, position[2],
        position[0], position[1] - downward_distance, position[2]};
    ScanResult result;
    for (auto cursor = state.words[28]; cursor != state.words[29];
         cursor += 4u) {
        const auto element = runtime.ReadElement(cursor);
        const auto clump = runtime.GetClump(element);
        if (!runtime.QueryCollisionData(clump)) continue;
        PalMovementCollisionCandidate candidate{};
        if (!runtime.ProcessClumpCollision(
                line, runtime.GetClump(element), candidate) ||
            !(result.height < candidate.height))
            continue;
        result.height = candidate.height;
        result.element = element;
        on_accepted(candidate);
        result.found = 1u;
    }
    return result;
}

}  // namespace

std::uint32_t PalMovementSecondaryCollisionRuntimeStub::ReadElement(
    std::uint32_t) { return 0u; }
std::uint32_t PalMovementSecondaryCollisionRuntimeStub::GetClump(
    std::uint32_t) { return 0u; }
bool PalMovementSecondaryCollisionRuntimeStub::QueryCollisionData(
    std::uint32_t) { return false; }
bool PalMovementSecondaryCollisionRuntimeStub::ProcessClumpCollision(
    const std::array<float, 6>&, std::uint32_t,
    PalMovementCollisionCandidate&) { return false; }
double PalMovementSecondaryCollisionRuntimeStub::SwitchThreshold(
    std::uint32_t) { return 0.0; }
void PalMovementSecondaryCollisionRuntimeStub::Leave(std::uint32_t) {}
void PalMovementSecondaryCollisionRuntimeStub::Enter(std::uint32_t) {}

std::uint32_t PalMovementSecondaryChild::FindHighestCollision(
    PalMovementSecondaryChildState& state,
    std::uint32_t write_field84, std::uint32_t write_field80,
    const std::array<float, 3>& position,
    float upward_distance, float downward_distance,
    std::array<float, 3>* output_position,
    IPalMovementSecondaryCollisionRuntime& runtime) {
    state.words[32] = 0xffffffffu;
    return ScanHighestClump(
        state, position, upward_distance, downward_distance, runtime,
        [&](const PalMovementCollisionCandidate& candidate) {
            if (write_field84 != 0u) state.words[33] = candidate.field84;
            if (write_field80 != 0u) state.words[32] = candidate.field80;
            if (output_position != nullptr) {
                *output_position = position;
                (*output_position)[1] = candidate.height;
            }
        }).found;
}

std::uint32_t PalMovementSecondaryChild::HasCollisionWithinHeight(
    const PalMovementSecondaryChildState& state,
    const std::array<float, 3>& position,
    float upward_distance, float downward_distance,
    std::uint32_t check_height,
    IPalMovementSecondaryCollisionRuntime& runtime) {
    const auto result = ScanHighestClump(
        state, position, upward_distance, downward_distance, runtime,
        [](const PalMovementCollisionCandidate&) {});
    if (check_height == 0u) return result.found;
    const auto distance = std::fabs(
        static_cast<double>(position[1]) -
        static_cast<double>(result.height));
    return distance > 10.0 ? 0u : result.found;
}

std::uint32_t PalMovementSecondaryChild::UpdateActiveCollision(
    PalMovementSecondaryChildState& state,
    const std::array<float, 3>& position,
    float upward_distance, float downward_distance,
    IPalMovementSecondaryCollisionRuntime& runtime) {
    const auto result = ScanHighestClump(
        state, position, upward_distance, downward_distance, runtime,
        [](const PalMovementCollisionCandidate&) {});
    if (result.element == 0u || result.element == state.words[31])
        return result.found;
    const auto distance = std::fabs(
        static_cast<double>(position[1]) -
        static_cast<double>(result.height));
    if (!(runtime.SwitchThreshold(result.element) >= distance)) return 0u;
    if (state.words[31] != 0u) runtime.Leave(state.words[31]);
    runtime.Enter(result.element);
    state.words[31] = result.element;
    return result.found;
}

}  // namespace pal4
