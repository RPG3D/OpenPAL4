#include "pal4/game_world_pixel_ray.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IGameWorldPixelRayRuntime {
public:
    void CalculatePixelRay(
        std::uint32_t camera, pal4::GameWorldRayBits& ray,
        const std::array<std::uint32_t, 2>& pixel) override {
        Check(camera == 0x4000u && pixel[0] == 123u && pixel[1] == 456u,
              "camera and both pixel coordinates reach ray calculation");
        ++ray_calls;
        ray = {0u, 0u, 0u, 0x41200000u, 0x41a00000u, 0x41f00000u};
    }
    void ForAllWorldIntersections(
        std::uint32_t world, const pal4::GameWorldRayBits& ray,
        std::uint32_t intersection_type,
        pal4::GameWorldCollisionBits& collision) override {
        Check(ray == pal4::GameWorldRayBits{0u, 0u, 0u,
                                           0x41200000u, 0x41a00000u,
                                           0x41f00000u} &&
                  world == 0x9000u && intersection_type == 1u &&
                  collision[0] == 0xffffffffu && collision[1] == 0u &&
                  collision[2] == pal4::GameWorldCollision::kMaxDistanceBits,
              "rebuilt collision initializes record and traverses calculated ray");
        ++collision_calls;
        if (hit)
            static_cast<void>(pal4::GameWorldCollision::OnIntersection(
                0xa000u, 17u, 0x3f000000u, collision));
    }
    void ForAllClumpAtomics(
        std::uint32_t,
        pal4::GameWorldClumpCollisionContext&) override {}
    void ForAllAtomicIntersections(
        std::uint32_t,
        pal4::GameWorldClumpCollisionContext&) override {}

    unsigned ray_calls = 0;
    unsigned collision_calls = 0;
    bool hit = false;
};

}  // namespace

int main() {
    pal4::GameWorldObjectState state;
    state.words[4] = 0x9000u;
    Runtime runtime;
    Check(pal4::GameWorldPixelRay::Intersect(
              state, 0x4000u, 123u, 456u, nullptr, runtime) == 0u &&
              runtime.ray_calls == 0u && runtime.collision_calls == 0u,
          "null output exits before ray and collision calls");

    pal4::GameWorldVectorBits intersection{1u, 2u, 3u};
    Check(pal4::GameWorldPixelRay::Intersect(
              state, 0x4000u, 123u, 456u, &intersection, runtime) == 0u &&
              intersection == pal4::GameWorldVectorBits{0x800000u,
                                                         0x800000u,
                                                         0x800000u},
          "collision miss leaves all three default output words");

    runtime.hit = true;
    Check(pal4::GameWorldPixelRay::Intersect(
              state, 0x4000u, 123u, 456u, &intersection, runtime) == 1u &&
              intersection == pal4::GameWorldVectorBits{
                  0x40a00000u, 0x41200000u, 0x41700000u} &&
              runtime.ray_calls == 2u && runtime.collision_calls == 2u,
          "collision hit copies rebuilt intersection point");
}
