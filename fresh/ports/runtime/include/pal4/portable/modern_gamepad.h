#pragma once
#include "pal4/portable/controller.h"
namespace pal4::portable {
// Input adaptation from pal4_inject gamepad.cpp/gamepad_control_hooks.cpp.
// It does not replace the game's movement, collision or script algorithms.
struct ModernGamepadPlan {
    bool active{},walk_turn{};
    float magnitude{},speed_multiplier{1},animation_multiplier{1};
    std::uint32_t mode{};
    std::array<float,3> direction{};
};
ModernGamepadPlan PlanModernGamepad(float x,float y,const ControlCameraBasis& camera,float yaw,float elapsed);
std::array<float,2> ModernGamepadCameraDelta(float x,float y,float elapsed);
bool ModernGamepadEnabled() noexcept;
}
