#include "pal4/collision_tree_storage.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Step final {
    std::uint32_t face = 0;
    std::uint32_t status = 0;
    std::uint32_t neighbor = 0;
    std::uint32_t edge = 0;
    pal4::D3dTriangleVector2 intersection{};
    float expected_start_x = 0.0f;
    float expected_end_x = 0.0f;
};

struct Runtime final : pal4::ICollisionTreeMovementRuntime {
    std::vector<Step> steps;
    std::size_t next_step = 0;
    int adjust_calls = 0;
    int clamp_calls = 0;
    int height_calls = 0;
    std::uint32_t projected_face = 0;
    std::uint32_t adjusted_edge = 0;
    bool extend_end_on_adjust = false;

    std::uint32_t Trace(
        std::uint32_t face, const pal4::D3dLine2State& line,
        std::uint32_t& neighbor, std::uint32_t& edge,
        pal4::D3dTriangleVector2& intersection) override {
        const auto& step = steps.at(next_step++);
        Check(face == step.face &&
                  std::fabs(line.start.x - step.expected_start_x) <
                      0.00001f &&
                  std::fabs(line.end.x - step.expected_end_x) <
                      0.00001f,
              "trace receives current face and updated line endpoints");
        neighbor = step.neighbor;
        edge = step.edge;
        intersection = step.intersection;
        return step.status;
    }
    void AdjustEdge(std::uint32_t face, std::uint32_t edge,
                    pal4::D3dLine2State& line) override {
        Check(face == 0x10u && line.start.x == 2.0f,
              "edge adjustment runs after hit becomes line start");
        ++adjust_calls;
        adjusted_edge = edge;
        if (extend_end_on_adjust) line.end.x = 12.0f;
    }
    void ClampPoint(std::uint32_t face,
                    pal4::D3dTriangleVector2& point) override {
        Check(face == 0x10u, "clamp uses current face");
        ++clamp_calls;
        point = {99.0f, 99.0f};
    }
    void ProjectHeight(std::uint32_t face,
                       pal4::D3dTriangleVector3& point) override {
        ++height_calls;
        projected_face = face;
        point.y = static_cast<float>(face) + 0.5f;
    }
};

pal4::CollisionTreeStorageState State(std::uint32_t max_steps) {
    pal4::CollisionTreeStorageState state{};
    state.words[15] = max_steps;
    return state;
}

void TestEqualAndSimpleExit() {
    const auto state = State(100u);
    Runtime runtime;
    pal4::D3dTriangleVector3 destination{1.0f, 99.0f, 2.0f};
    std::uint32_t final_face = 0u;
    Check(pal4::CollisionTreeStorage::AdvanceAcrossFaces(
              state, {1.0f, 0.0f, 2.0f}, 0x20u,
              destination, final_face, runtime) == 0u &&
              runtime.next_step == 0u && runtime.height_calls == 1 &&
              final_face == 0x20u && destination.y == 32.5f,
          "equal X/Z endpoints skip trace but still project height");

    runtime = Runtime{};
    runtime.steps = {{0x10u, 0u, 0u, 0u, {}, 0.0f, 10.0f}};
    destination = {10.0f, 99.0f, 0.0f};
    Check(pal4::CollisionTreeStorage::AdvanceAcrossFaces(
              state, {0.0f, 0.0f, 0.0f}, 0x10u,
              destination, final_face, runtime) == 0u &&
              runtime.clamp_calls == 1 && runtime.height_calls == 1 &&
              destination.x == 10.0f && destination.z == 0.0f &&
              destination.y == 16.5f,
          "status zero clamps local start but writes original line end");
}

void TestNeighborAndLimit() {
    const auto state = State(100u);
    Runtime runtime;
    runtime.steps = {
        {0x10u, 2u, 0x20u, 1u, {3.0f, 0.0f}, 0.0f, 10.0f},
        {0x20u, 1u, 0u, 0u, {}, 3.0f, 10.0f}};
    pal4::D3dTriangleVector3 destination{10.0f, 99.0f, 0.0f};
    std::uint32_t final_face = 0u;
    Check(pal4::CollisionTreeStorage::AdvanceAcrossFaces(
              state, {0.0f, 0.0f, 0.0f}, 0x10u,
              destination, final_face, runtime) == 1u &&
              runtime.next_step == 2u && runtime.adjust_calls == 0 &&
              final_face == 0x20u && destination.y == 32.5f,
          "status two changes to neighbor face before subsequent trace");

    runtime = Runtime{};
    runtime.steps = {{0x10u, 2u, 0x20u, 1u,
                      {3.0f, 0.0f}, 0.0f, 10.0f}};
    destination = {10.0f, 99.0f, 0.0f};
    Check(pal4::CollisionTreeStorage::AdvanceAcrossFaces(
              State(1u), {0.0f, 0.0f, 0.0f}, 0x10u,
              destination, final_face, runtime) == 3u &&
              runtime.next_step == 1u && runtime.adjust_calls == 0 &&
              final_face == 0x10u && destination.x == 10.0f &&
              destination.y == 16.5f,
          "step cap returns status three before applying the intersection");
}

void TestBoundaryRetreatAndUnknownStatus() {
    const auto state = State(100u);
    Runtime runtime;
    runtime.extend_end_on_adjust = true;
    runtime.steps = {
        {0x10u, 2u, 0u, 7u, {2.0f, 0.0f}, 0.0f, 10.0f},
        {0x10u, 1u, 0u, 0u, {}, 2.0f, 11.0f}};
    pal4::D3dTriangleVector3 destination{10.0f, 99.0f, 0.0f};
    std::uint32_t final_face = 0u;
    Check(pal4::CollisionTreeStorage::AdvanceAcrossFaces(
              state, {0.0f, 0.0f, 0.0f}, 0x10u,
              destination, final_face, runtime) == 1u &&
              runtime.adjust_calls == 1 && runtime.adjusted_edge == 7u &&
              destination.x == 11.0f && final_face == 0x10u,
          "unlinked edge adjusts line before retreating end to ninety percent");

    runtime = Runtime{};
    runtime.steps = {{0x10u, 4u, 0u, 0u, {}, 0.0f, 10.0f},
                     {0x10u, 1u, 0u, 0u, {}, 0.0f, 10.0f}};
    destination = {10.0f, 99.0f, 0.0f};
    Check(pal4::CollisionTreeStorage::AdvanceAcrossFaces(
              state, {0.0f, 0.0f, 0.0f}, 0x10u,
              destination, final_face, runtime) == 1u &&
              runtime.next_step == 2u,
          "other trace statuses retry without changing the line");
    pal4::CollisionTreeMovementRuntimeStub stub;
    destination = {10.0f, 99.0f, 0.0f};
    Check(pal4::CollisionTreeStorage::AdvanceAcrossFaces(
              state, {0.0f, 0.0f, 0.0f}, 0x10u,
              destination, final_face, stub) == 1u &&
              final_face == 0x10u && destination.y == 99.0f,
          "linkable movement stub ends a nonempty trace without changing height");
}

}  // namespace

int main() {
    TestEqualAndSimpleExit();
    TestNeighborAndLimit();
    TestBoundaryRetreatAndUnknownStatus();
}
