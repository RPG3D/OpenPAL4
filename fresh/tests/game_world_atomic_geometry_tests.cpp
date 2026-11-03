#include "pal4/game_world_atomic_geometry.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IGameWorldAtomicGeometryRuntime {
public:
    std::uint32_t ForAllAtomics(
        std::uint32_t clump, pal4::GameWorldBoundsBits& bounds,
        pal4::GameWorldAtomicGeometryState& state) override {
        Check(clump == 0xa000u && bounds[0] == 17u &&
                  state.first_atomic == 1u && state.transform_mode == 9u,
              "wrapper initializes both globals before atomic traversal");
        ++calls;
        return result;
    }
    std::uint32_t UserDataCount(std::uint32_t atomic) override {
        Check(atomic == 0xb000u, "user data belongs to selected atomic");
        events.push_back("count");
        return static_cast<std::uint32_t>(user_data.size());
    }
    pal4::GameWorldAtomicUserData ReadUserData(
        std::uint32_t atomic, std::uint32_t index) override {
        Check(atomic == 0xb000u && index < user_data.size(),
              "user data is scanned in index order");
        events.push_back("user_data");
        return user_data[index];
    }
    std::uint32_t AtomicFlags(std::uint32_t atomic) override {
        Check(atomic == 0xb000u, "atomic flags come from callback object");
        events.push_back("flags");
        return flags;
    }
    void ResyncSphere(std::uint32_t atomic) override {
        Check(atomic == 0xb000u, "resync receives callback atomic");
        events.push_back("resync");
    }
    pal4::GameWorldBoundsBits ReadSphere(std::uint32_t atomic) override {
        Check(atomic == 0xb000u, "sphere comes from callback atomic");
        events.push_back("sphere");
        return local_sphere;
    }
    std::uint32_t AtomicFrame(std::uint32_t atomic) override {
        Check(atomic == 0xb000u, "frame comes from callback atomic");
        events.push_back("frame");
        return 0xc000u;
    }
    std::uint32_t ParentFrame(std::uint32_t atomic) override {
        Check(atomic == 0xb000u, "parent frame comes from callback atomic");
        events.push_back("parent_frame");
        return 0xc100u;
    }
    pal4::GameWorldMatrixBits ObjectTransform(
        std::uint32_t frame) override {
        Check(frame == 0xc000u || frame == 0xc100u,
              "object transform uses atomic or parent frame");
        events.push_back(frame == 0xc000u ? "transform" : "parent_transform");
        pal4::GameWorldMatrixBits matrix{};
        matrix[0] = frame;
        return matrix;
    }
    pal4::GameWorldMatrixBits SetupTransformMatrix(
        const pal4::GameWorldMatrixBits& matrix) override {
        Check(matrix[0] == 0xc100u, "parent matrix is prepared");
        events.push_back("setup");
        return matrix;
    }
    pal4::GameWorldMatrixBits MultiplyMatrix(
        const pal4::GameWorldMatrixBits& first,
        const pal4::GameWorldMatrixBits& second) override {
        Check(first[0] == 0xc000u && second[0] == 0xc100u,
              "atomic and prepared parent matrices multiply in native order");
        events.push_back("multiply");
        return first;
    }
    pal4::GameWorldBoundsBits TransformSphere(
        const pal4::GameWorldBoundsBits& sphere,
        const pal4::GameWorldMatrixBits& matrix) override {
        Check(sphere == local_sphere && matrix[0] == 0xc000u,
              "sphere transform uses chosen matrix");
        events.push_back("transform_sphere");
        return transformed;
    }
    double Distance(const std::array<float, 3>& delta) override {
        events.push_back("distance");
        return std::sqrt(static_cast<double>(delta[0]) * delta[0] +
                         static_cast<double>(delta[1]) * delta[1] +
                         static_cast<double>(delta[2]) * delta[2]);
    }
    unsigned calls = 0;
    std::uint32_t result = 0x1234u;
    std::uint32_t flags = 0u;
    std::vector<pal4::GameWorldAtomicUserData> user_data;
    pal4::GameWorldBoundsBits local_sphere{};
    pal4::GameWorldBoundsBits transformed{};
    std::vector<std::string> events;
};

std::uint32_t Bits(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

void UpdateAtomicBranches() {
    Runtime runtime;
    pal4::GameWorldAtomicGeometryState state{1u, 1u};
    pal4::GameWorldBoundsBits bounds{17u, 18u, 19u, 20u};
    runtime.user_data = {{3u, "hook_bone"}};
    Check(pal4::GameWorldAtomicGeometry::UpdateAtomic(
              0xb000u, bounds, state, runtime) == 0xb000u &&
              state.first_atomic == 1u && bounds[0] == 17u &&
              runtime.events == std::vector<std::string>{"count", "user_data"},
          "hook-prefixed user data skips sphere processing and retains first flag");

    runtime.events.clear();
    runtime.user_data = {{2u, "hook_bone"}, {3u, "hook_"},
                         {3u, "other_name"}};
    runtime.flags = 2u;
    runtime.local_sphere = {Bits(1.f), Bits(2.f), Bits(3.f), Bits(4.f)};
    runtime.transformed = {Bits(5.f), Bits(6.f), Bits(7.f), Bits(8.f)};
    Check(pal4::GameWorldAtomicGeometry::UpdateAtomic(
              0xb000u, bounds, state, runtime) == 0xb000u &&
              state.first_atomic == 0u && bounds == runtime.transformed &&
              runtime.events == std::vector<std::string>{
                  "count", "user_data", "user_data", "user_data", "flags",
                  "resync", "sphere", "frame", "transform",
                  "transform_sphere"},
          "nonmatching user data permits resync and first sphere copy");

    runtime.events.clear();
    runtime.user_data.clear();
    state.transform_mode = 0u;
    bounds = {Bits(0.f), Bits(0.f), Bits(0.f), Bits(2.f)};
    runtime.flags = 0u;
    runtime.transformed = {Bits(4.f), Bits(0.f), Bits(0.f), Bits(2.f)};
    Check(pal4::GameWorldAtomicGeometry::UpdateAtomic(
              0xb000u, bounds, state, runtime) == 0xb000u &&
              bounds == pal4::GameWorldBoundsBits{
                  Bits(2.f), Bits(0.f), Bits(0.f), Bits(4.f)} &&
              runtime.events == std::vector<std::string>{
                  "count", "flags", "sphere", "frame", "transform",
                  "parent_frame", "parent_transform", "setup", "multiply",
                  "transform_sphere", "distance"},
          "later sphere uses composed matrix and merges centers and radii");

    runtime.events.clear();
    bounds = {Bits(0.f), Bits(0.f), Bits(0.f), Bits(10.f)};
    runtime.transformed = {Bits(1.f), Bits(0.f), Bits(0.f), Bits(1.f)};
    Check(pal4::GameWorldAtomicGeometry::UpdateAtomic(
              0xb000u, bounds, state, runtime) == 0xb000u &&
              bounds[0] == Bits(0.f) && bounds[3] == Bits(10.f),
          "sphere fully within previous radius leaves bounds unchanged");
}

}  // namespace

int main() {
    pal4::GameWorldBoundsBits bounds{17u, 0u, 0u, 0u};
    pal4::GameWorldAtomicGeometryState state{0u, 42u};
    Runtime runtime;
    Check(pal4::GameWorldAtomicGeometry::Process(
              0xa000u, 9u, bounds, state, runtime) == 0x1234u &&
              runtime.calls == 1u && state.first_atomic == 1u &&
              state.transform_mode == 9u,
          "wrapper forwards traversal result and preserves initialized state");
    runtime.result = 0u;
    Check(pal4::GameWorldAtomicGeometry::Process(
              0xa000u, 9u, bounds, state, runtime) == 0u &&
              runtime.calls == 2u,
          "zero traversal result is forwarded");
    UpdateAtomicBranches();
}
