#include "pal4/game_world_navmesh_projector.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameWorldNavmeshRuntime {
    std::vector<std::string> calls;
    std::uint32_t collision_result = 0u;
    std::uint32_t clump = 0xabcdu;
    std::uint32_t camera = 0u;
    std::int32_t x = 0;
    std::int32_t y = 0;

    void CalculatePixelRay(std::uint32_t camera_arg, std::int32_t x_arg,
                           std::int32_t y_arg,
                           pal4::GameWorldPixelRay& ray) override {
        calls.push_back("ray");
        camera = camera_arg;
        x = x_arg;
        y = y_arg;
        ray = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    }
    std::uint32_t NavClump() override {
        calls.push_back("nav");
        return clump;
    }
    std::uint32_t Collide(
        const pal4::GameWorldPixelRay& ray, std::uint32_t clump_arg,
        pal4::GameWorldCollisionSample& sample) override {
        calls.push_back("collision");
        Check(ray == pal4::GameWorldPixelRay{
                         1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f} &&
                  clump_arg == clump,
              "collision receives computed six-float ray and nav clump");
        sample = {0x10u, 0x20u, 0x30u, 0x40u, 0x50u, 0x60u};
        return collision_result;
    }
};

void NullOutput() {
    Runtime runtime;
    Check(pal4::GameWorldNavmeshProjector::Project(
              0x777u, 12, 34, nullptr, runtime) == 0u &&
              runtime.calls.empty(),
          "null output exits before ray or navigation work");
}

void MissAndHit() {
    Runtime miss;
    pal4::GameWorldPointWords output{1u, 2u, 3u};
    Check(pal4::GameWorldNavmeshProjector::Project(
              0x777u, -12, 34, &output, miss) == 0u &&
              output == pal4::GameWorldPointWords{
                            0x800000u, 0x800000u, 0x800000u} &&
              miss.calls == std::vector<std::string>{
                  "ray", "nav", "collision"} &&
              miss.camera == 0x777u && miss.x == -12 && miss.y == 34,
          "miss keeps all three sentinel coordinates and call order");

    Runtime hit;
    hit.collision_result = 7u;
    output = {1u, 2u, 3u};
    Check(pal4::GameWorldNavmeshProjector::Project(
              0x888u, 123, -45, &output, hit) == 7u &&
              output == pal4::GameWorldPointWords{
                            0x40u, 0x50u, 0x60u} &&
              hit.calls == std::vector<std::string>{
                  "ray", "nav", "collision"} &&
              hit.camera == 0x888u && hit.x == 123 && hit.y == -45,
          "hit copies sample words three through five and raw result");
}

void NativeStub() {
    pal4::GameWorldNavmeshRuntimeStub runtime;
    pal4::GameWorldPointWords output{1u, 2u, 3u};
    Check(pal4::GameWorldNavmeshProjector::Project(
              0x777u, 12, 34, &output, runtime) == 0u &&
              output == pal4::GameWorldPointWords{
                            0x800000u, 0x800000u, 0x800000u},
          "native ray/collision stub links and leaves miss sentinels");
}

}  // namespace

int main() {
    try {
        NullOutput();
        MissAndHit();
        NativeStub();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
