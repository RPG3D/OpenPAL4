#include "pal4/actor_manager_screen_projection.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IActorManagerScreenProjectionRuntime {
    std::uint32_t child = 0u;
    std::vector<std::string> trace;
    pal4::ActorWorldPoint projected_point{};

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x1328u, "actor child offset");
        trace.emplace_back("child");
        return child;
    }
    float ReadFloat(std::uint32_t address) override {
        trace.push_back("float:" + std::to_string(address));
        switch (address) {
        case 0x20acu: return 12.5f;
        case 0x20b0u: return -3.0f;
        case 0x20b4u: return 41.0f;
        default: throw std::runtime_error("unexpected child coordinate");
        }
    }
    pal4::ActorScreenPoint ProjectCamera(
        std::uint32_t camera, const pal4::ActorWorldPoint& point) override {
        Check(camera == 0x3000u, "camera argument");
        projected_point = point;
        trace.emplace_back("camera");
        return {7.0f, -11.0f};
    }
    pal4::ActorScreenPoint ConvertUIFrame(
        const pal4::ActorScreenPoint& point) override {
        Check(point.x == 7.0f && point.y == -11.0f,
              "UI conversion receives camera result");
        trace.emplace_back("ui");
        return {100.0f, 200.0f};
    }
};

void MissingChild() {
    Runtime runtime;
    const auto result = pal4::ActorManagerScreenProjection::Project(
        0x1000u, 0x3000u, runtime);
    Check(result.x == std::numeric_limits<float>::max() &&
          result.y == std::numeric_limits<float>::max() &&
          runtime.trace == std::vector<std::string>{"child"},
          "missing child returns FLT_MAX without projecting");
}

void WorldToUI() {
    Runtime runtime;
    runtime.child = 0x2000u;
    const auto result = pal4::ActorManagerScreenProjection::Project(
        0x1000u, 0x3000u, runtime);
    Check(runtime.projected_point.x == 12.5f &&
          runtime.projected_point.y == 77.0f &&
          runtime.projected_point.z == 41.0f &&
          result.x == 100.0f && result.y == 200.0f &&
          runtime.trace == std::vector<std::string>{
              "child", "float:8364", "float:8368", "float:8372",
              "camera", "ui"},
          "child coordinates project with 80-unit head offset");
}

}  // namespace

int main() {
    try {
        MissingChild();
        WorldToUI();
        std::cout << "actor_manager_screen_projection_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
