#include "pal4/clump_vector_motion.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(const double lhs, const double rhs) {
    return std::fabs(lhs - rhs) < 0.0001;
}

struct Runtime final : pal4::IClumpVectorMotionRuntime {
    std::vector<std::string> events;
    void Normalize(pal4::ClumpMotionVector& vector) override {
        events.emplace_back("normalize");
        const double magnitude = std::sqrt(
            vector.x * vector.x + vector.y * vector.y +
            vector.z * vector.z);
        if (magnitude == 0.0) return;
        vector.x = static_cast<float>(vector.x / magnitude);
        vector.y = static_cast<float>(vector.y / magnitude);
        vector.z = static_cast<float>(vector.z / magnitude);
    }
    [[nodiscard]] double ApproximateMagnitude(
        const pal4::ClumpMotionVector& vector) override {
        Check(vector.x == 4.0f && vector.y == 0.0f && vector.z == 0.0f,
              "passes horizontal delta to native magnitude helper");
        events.emplace_back("magnitude");
        return 4.0;
    }
};

void CheckPc24LaunchVelocities() {
    struct AxisRuntime final : pal4::IClumpVectorMotionRuntime {
        void Normalize(pal4::ClumpMotionVector& value) override {
            Check(value.x > 0.f && value.y == 0.f && value.z == 0.f,
                  "trajectory reference uses an exact positive axis");
            value = {1.f, 0.f, 0.f};
        }
        double ApproximateMagnitude(const pal4::ClumpMotionVector& value) override {
            Check(value.y == 0.f && value.z == 0.f,
                  "trajectory reference length has no vertical component");
            return value.x;
        }
    } runtime;
    // Independent Win32 x87 PC24 reference executes the verified5FB86C..983
    // arithmetic. It is retained under objs/m10-jump-reference, including its
    // output, rather than deriving expected words from this implementation.
    struct Case { float delta_x, delta_y, apex; unsigned vx, vy; };
    constexpr std::array cases{
        Case{100.f, 0.f, 15.f, 0x42dd5c03u, 0x4384d0d0u},
        Case{160.f, 10.f, 15.f, 0x43518907u, 0x43607f30u},
        Case{240.f, -10.f, 20.f, 0x437a6253u, 0x438ce74du}};
    const pal4::ClumpMotionVector start{542.801513671875f,
        211.50360107421875f, -538.32373046875f};
    for (const auto& test : cases) {
        pal4::ClumpVectorMotionState state;
        pal4::ClumpMotionVector output;
        const pal4::ClumpMotionVector finish{start.x + test.delta_x,
            start.y + test.delta_y, start.z};
        (void)pal4::ClumpVectorMotion::Initialize(state, output, start, finish,
            test.apex, 588.f, runtime);
        if (std::bit_cast<unsigned>(output.x) != test.vx ||
            std::bit_cast<unsigned>(output.y) != test.vy) {
            std::cerr << "PC24 actual vx=" << std::hex
                      << std::bit_cast<unsigned>(output.x) << " vy="
                      << std::bit_cast<unsigned>(output.y) << " expected vx="
                      << test.vx << " vy=" << test.vy << std::dec << '\n';
        }
        Check(std::bit_cast<unsigned>(output.x) == test.vx &&
                  std::bit_cast<unsigned>(output.y) == test.vy && output.z == 0.f,
              "trajectory velocities match independent PC24 words for all height branches");
    }
}

}  // namespace

int main() {
    CheckPc24LaunchVelocities();
    using Motion = pal4::ClumpVectorMotion;
    pal4::ClumpVectorMotionState state;
    pal4::ClumpMotionVector output;
    Runtime runtime;
    const pal4::ClumpMotionVector start{0.0f, 0.0f, 0.0f};
    const pal4::ClumpMotionVector end{4.0f, 0.0f, 0.0f};
    Check(&Motion::Initialize(state, output, start, end,
                              2.0f, 2.0f, runtime) == &output &&
              state.start.x == 0.0f && state.end.x == 4.0f &&
              state.launch_value == 2.0f && state.gravity == 2.0f &&
              Near(output.x, 1.0 / std::sqrt(2.0)) &&
              Near(output.y, 4.0 * std::sqrt(2.0)) &&
              output.z == 0.0f &&
              runtime.events == std::vector<std::string>({
                  "normalize", "magnitude", "magnitude", "magnitude"}),
          "equal-height trajectory stores inputs and calculates velocity");
    runtime.events.clear();
    const pal4::ClumpMotionVector higher{4.0f, 2.0f, 0.0f};
    Check(&Motion::Initialize(state, output, start, higher,
                              2.0f, 2.0f, runtime) == &output &&
              Near(output.x, 4.0 / (2.0 * (1.0 + std::sqrt(2.0)))) &&
              Near(output.y, 2.0 * (1.0 + std::sqrt(2.0))),
          "higher destination follows positive-height time branch");
    runtime.events.clear();
    const pal4::ClumpMotionVector elevated_start{0.0f, 2.0f, 0.0f};
    Check(&Motion::Initialize(state, output, elevated_start, end,
                              2.0f, 2.0f, runtime) == &output &&
              Near(output.y,
                   2.0 * (1.0 + std::sqrt(2.0) +
                          std::sqrt(2.0 + std::sqrt(2.0) / 4.0))),
          "lower destination follows negative-height time branch");
    runtime.events.clear();
    (void)Motion::Initialize(state, output, start, end,
                             0.25f, 2.0f, runtime);
    Check(state.launch_value == 1.0f && std::isfinite(output.x) &&
              std::isfinite(output.y),
          "launch value below one is clamped before trajectory calculation");

    state.velocity = {1.0f, 3.0f, -2.0f};
    state.gravity = 2.0f;
    Check(&Motion::ScaleAndSetStatus(state, output, 0.5f) == &output &&
              Near(output.x, 0.5) && Near(output.y, 1.5) &&
              Near(output.z, -1.0) && Near(state.velocity.y, 2.0) &&
              state.status == 1u,
          "scale outputs old velocity before applying gravity");
    Check(&Motion::ScaleAndSetStatus(state, output, 1.0f) == &output &&
              Near(output.y, 2.0) && Near(state.velocity.y, 0.0) &&
              state.status == 2u,
          "zero remaining vertical velocity enters native status two");
    Check(&Motion::ScaleAndSetStatus(state, output, 1.0f) == &output &&
              Near(output.y, 0.0) && Near(state.velocity.y, -2.0) &&
              state.status == 2u,
          "negative remaining vertical velocity keeps status two");
    pal4::ClumpVectorMotionRuntimeStub stub;
    state.velocity = {1.0f, 1.0f, 1.0f};
    Check(&Motion::ScaleAndSetStatus(state, output, 1.0f) == &output,
          "vector motion boundary stub links");
    return EXIT_SUCCESS;
}
