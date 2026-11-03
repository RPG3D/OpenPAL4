#pragma once
#include "pal4/portable/world.h"
#include <string>

namespace pal4::portable {
// Static-scene bring-up policy. These are explicit configuration values,
// pending binding to the original loaded actor/controller properties.
struct MovementConfig { float speed{120},height{72},step_up{16},step_down{32}; };
enum class MovementObstacle { none,wall,no_floor };
struct MovementState {
    Point3 position{};
    float yaw{};
    std::uint32_t mode{};
    std::string animation{"C01"};
    std::optional<FloorHit> floor;
    std::uint64_t accepted_steps{},rejected_steps{},clipped_steps{};
    double distance{};
    bool input_active{},last_accepted{};
    MovementObstacle last_obstacle{};
};
// Calls the same typed vector-control flow as the PC adapter. Static geometry
// services use leading-radius wall segments and floor ranges; full original
// sliding, attributes, dynamic collisions and event services remain separate.
void StepActorMovement(MovementState& state,const MovementConfig& config,
                       const CollisionMesh& mesh,Point3 input,float elapsed);
const char* MovementObstacleName(MovementObstacle reason);
}  // namespace pal4::portable
