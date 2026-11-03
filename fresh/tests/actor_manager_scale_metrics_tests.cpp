#include "pal4/actor_manager_scale_metrics.h"

#include <array>
#include <cmath>
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

struct Runtime final : pal4::IActorManagerScaleMetricsRuntime {
    std::uint32_t child = 0u;
    std::uint32_t mode = 0u;
    float dimension = 100.0f;
    std::vector<std::string> trace;

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x1328u) {
            trace.emplace_back("child");
            return child;
        }
        Check(address == 0x1324u, "actor mode offset");
        trace.emplace_back("mode");
        return mode;
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address == 0x226cu, "child dimension offset");
        trace.emplace_back("dimension");
        return dimension;
    }
};

void ScaleBranches() {
    Runtime runtime;
    Check(pal4::ActorManagerScaleMetrics::Scale(0x1000u, runtime) ==
              std::array<float, 3>{1.0f, 1.0f, 1.0f} &&
          runtime.trace == std::vector<std::string>{"child"},
          "missing child uses unit scale without reading mode");

    runtime.child = 0x2000u;
    runtime.trace.clear();
    Check(pal4::ActorManagerScaleMetrics::Scale(0x1000u, runtime) ==
              std::array<float, 3>{1.0f, 1.0f, 1.0f} &&
          runtime.trace == std::vector<std::string>{"child", "mode"},
          "other actor modes use unit scale");

    runtime.mode = 2u;
    runtime.trace.clear();
    const auto scale = pal4::ActorManagerScaleMetrics::Scale(0x1000u, runtime);
    Check(std::abs(scale[0] - 2.5f) < 0.00001f &&
          scale[0] == scale[1] && scale[1] == scale[2] &&
          runtime.trace == std::vector<std::string>{
              "child", "mode", "dimension"},
          "mode two scales every axis by child dimension times 0.025");
}

void SizeBounds() {
    Runtime runtime;
    Check(pal4::ActorManagerScaleMetrics::ClampedSize(0x1000u, runtime) ==
              70.0 &&
          runtime.trace == std::vector<std::string>{"child"},
          "missing child uses lower bound");

    runtime.child = 0x2000u;
    for (const auto [dimension, expected] :
         std::array<std::array<float, 2>, 5>{{
             {0.0f, 70.0f}, {60.0f, 70.0f}, {100.0f, 110.0f},
             {180.0f, 190.0f}, {300.0f, 190.0f}}}) {
        runtime.dimension = dimension;
        runtime.trace.clear();
        Check(pal4::ActorManagerScaleMetrics::ClampedSize(0x1000u, runtime) ==
                  expected &&
              runtime.trace == std::vector<std::string>{
                  "child", "dimension"},
              "size clamps dimension plus ten to 70..190");
    }
    runtime.dimension = std::numeric_limits<float>::quiet_NaN();
    Check(std::isnan(pal4::ActorManagerScaleMetrics::ClampedSize(
              0x1000u, runtime)), "x87 unordered comparison preserves NaN");
}

}  // namespace

int main() {
    try {
        ScaleBranches();
        SizeBounds();
        std::cout << "actor_manager_scale_metrics_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
