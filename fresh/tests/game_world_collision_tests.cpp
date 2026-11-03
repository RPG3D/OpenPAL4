#include "pal4/game_world_collision.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IGameWorldCollisionRuntime {
public:
    void ForAllWorldIntersections(
        std::uint32_t world, const pal4::GameWorldRayBits& ray,
        std::uint32_t intersection_type,
        pal4::GameWorldCollisionBits& collision) override {
        Check(world == 0x9000u && intersection_type == 1u &&
                  ray == expected_ray && collision[0] == 0xffffffffu &&
                  collision[1] == 0u &&
                  collision[2] == pal4::GameWorldCollision::kMaxDistanceBits,
              "world traversal receives exact ray, type and initialized record");
        ++calls;
        for (const auto distance : distances)
            static_cast<void>(pal4::GameWorldCollision::OnIntersection(
                0xa000u, 0x77u, distance, collision));
    }
    void ForAllClumpAtomics(
        std::uint32_t clump,
        pal4::GameWorldClumpCollisionContext& context) override {
        Check(clump == 0xb000u && context.ray == expected_ray &&
                  context.intersection_type == 1u &&
                  context.collision != nullptr,
              "clump traversal receives copied ray and output record");
        ++clump_calls;
        for (const auto atomic : atomics)
            static_cast<void>(pal4::GameWorldCollision::OnAtomic(
                atomic, context, *this));
    }
    void ForAllAtomicIntersections(
        std::uint32_t atomic,
        pal4::GameWorldClumpCollisionContext& context) override {
        ++atomic_calls;
        if (atomic == 0xc000u)
            static_cast<void>(pal4::GameWorldCollision::OnAtomicIntersection(
                0xd000u, 7u, 0x3f400000u, context));
        if (atomic == 0xc100u)
            static_cast<void>(pal4::GameWorldCollision::OnAtomicIntersection(
                0xd100u, 8u, 0x3f000000u, context));
    }

    pal4::GameWorldRayBits expected_ray{};
    std::vector<std::uint32_t> distances;
    std::vector<std::uint32_t> atomics;
    unsigned calls = 0;
    unsigned clump_calls = 0;
    unsigned atomic_calls = 0;
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::GameWorldCollisionBits collision;
    collision.fill(0x55555555u);
    const pal4::GameWorldRayBits ray{
        0x3f800000u, 0x40000000u, 0x40400000u,
        0x41200000u, 0x41b00000u, 0x42000000u};
    runtime.expected_ray = ray;
    Check(pal4::GameWorldCollision::Process(
              nullptr, 0x9000u, collision, runtime) == 0u &&
              pal4::GameWorldCollision::Process(
                  &ray, 0u, collision, runtime) == 0u &&
              collision[0] == 0x55555555u && runtime.calls == 0u,
          "null ray and world return before touching output");

    Check(pal4::GameWorldCollision::Process(
              &ray, 0x9000u, collision, runtime) == 0u &&
              collision[0] == 0xffffffffu && collision[1] == 0u &&
              collision[2] == pal4::GameWorldCollision::kMaxDistanceBits &&
              runtime.calls == 1u,
          "missing intersections leave initialized sentinel fields");

    runtime.distances = {0x3f400000u, 0x3f000000u, 0x3f800000u};
    Check(pal4::GameWorldCollision::Process(
              &ray, 0x9000u, collision, runtime) == 1u &&
              collision[0] == 0x77u && collision[2] == 0x3f000000u &&
              collision[3] == 0x40b00000u &&
              collision[4] == 0x41400000u &&
              collision[5] == 0x418c0000u,
          "closest hit computes origin plus distance times ray delta");

    collision[2] = 0x3f000000u;
    Check(pal4::GameWorldCollision::OnIntersection(
              0xa000u, 0x99u, 0x3f000000u, collision) == 0xa000u &&
              collision[0] == 0x77u && collision[2] == 0x3f000000u,
          "equal distance keeps prior triangle");
    Check(pal4::GameWorldCollision::OnIntersection(
              0xb000u, 0x99u, 0x7fc00000u, collision) == 0xb000u &&
              collision[0] == 0x77u,
          "unordered distance keeps prior triangle");

    collision.fill(0x55555555u);
    Check(pal4::GameWorldCollision::ProcessClump(
              nullptr, 0xb000u, collision, runtime) == 0u &&
              pal4::GameWorldCollision::ProcessClump(
                  &ray, 0u, collision, runtime) == 0u &&
              collision[0] == 0x55555555u && runtime.clump_calls == 0u,
          "clump null gates preserve the output record");
    runtime.atomics = {0xc000u, 0xc100u};
    Check(pal4::GameWorldCollision::ProcessClump(
              &ray, 0xb000u, collision, runtime) == 1u &&
              collision[0] == 8u && collision[1] == 0xc100u &&
              collision[2] == 0x3f000000u &&
              collision[3] == 0x40b00000u &&
              collision[4] == 0x41400000u &&
              collision[5] == 0x418c0000u &&
              runtime.clump_calls == 1u && runtime.atomic_calls == 2u,
          "clump traversal keeps nearest triangle and owning atomic");

    collision.fill(0x55555555u);
    Check(pal4::GameWorldCollision::ProcessAtomic(
              &ray, 0xc000u, collision, runtime) == 1u &&
              collision[0] == 7u && collision[1] == 0xc000u &&
              collision[2] == 0x3f400000u && runtime.atomic_calls == 3u,
          "single atomic path shares hit initialization and point calculation");
}
