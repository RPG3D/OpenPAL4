#include "pal4/collision_tree_storage.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICollisionTreePathRuntime {
    std::vector<pal4::D3dFaceCandidate> entries;
    std::vector<std::uint32_t> initial_faces;
    std::map<std::uint32_t, std::vector<std::uint32_t>> propagated_faces;
    std::map<std::uint32_t, std::uint32_t> markers;
    std::map<std::uint32_t, std::uint32_t> next_faces;
    std::vector<std::string> events;
    std::vector<std::uint32_t> path_faces;
    std::vector<pal4::D3dTriangleVector3> path_points;
    std::uint32_t begin_generation = 0;
    pal4::D3dTriangleVector3 begin_target{};
    int actor_begins = 0;
    int actor_ends = 0;

    std::span<pal4::D3dFaceCandidate> Append(
        pal4::D3dFaceTraversalState& state,
        const pal4::D3dFaceCandidate& candidate) override {
        const auto count = (state.words[3] - state.words[2]) / 8u;
        if (count == entries.size()) entries.push_back(candidate);
        else entries[count] = candidate;
        state.words[3] += 8u;
        return {entries.data(), count + 1u};
    }
    std::span<pal4::D3dFaceCandidate> Entries(
        pal4::D3dFaceTraversalState& state) override {
        const auto count = (state.words[3] - state.words[2]) / 8u;
        return {entries.data(), count};
    }
    void InitializeSeedFace(std::uint32_t face,
                            pal4::D3dFaceTraversalState& state) override {
        events.push_back("seed:" + std::to_string(face));
        Check(state.words[2] == state.words[3],
              "reset empties the queue before seed-face initialization");
        for (auto initial : initial_faces) {
            static_cast<void>(Append(state, {initial, 0.0f}));
        }
    }
    void PropagateFace(std::uint32_t face,
                       pal4::D3dFaceTraversalState& state) override {
        events.push_back("propagate:" + std::to_string(face));
        for (auto next : propagated_faces[face]) {
            static_cast<void>(Append(state, {next, 0.0f}));
        }
    }
    std::uint32_t Marker(std::uint32_t face) override {
        events.push_back("marker:" + std::to_string(face));
        return markers.at(face);
    }
    pal4::D3dTriangleVector3 Midpoint(
        std::uint32_t face, std::uint32_t marker) override {
        events.push_back("midpoint:" + std::to_string(face));
        Check(marker == markers.at(face), "marker forwarded to midpoint lookup");
        return {static_cast<float>(face), 0.0f,
                static_cast<float>(marker)};
    }
    pal4::D3dTriangleVector3 Project(
        std::uint32_t face,
        const pal4::D3dTriangleVector3& point) override {
        events.push_back("project:" + std::to_string(face));
        auto projected = point;
        projected.y = static_cast<float>(face + 1u);
        return projected;
    }
    std::uint32_t Neighbor(std::uint32_t face,
                           std::uint32_t marker) override {
        events.push_back("neighbor:" + std::to_string(face));
        Check(marker == markers.at(face), "marker forwarded to neighbor lookup");
        return next_faces.at(face);
    }
    void BeginActorPath(
        std::uint32_t actor, const pal4::CollisionTreeStorageState& storage,
        const pal4::D3dTriangleVector3& start, std::uint32_t start_face,
        const pal4::D3dTriangleVector3& target,
        std::uint32_t target_face) override {
        events.push_back("begin:" + std::to_string(actor));
        ++actor_begins;
        begin_generation = storage.words[5];
        begin_target = target;
        Check(start_face == 0x30u || start_face == target_face ||
                  start_face == 0u,
              "actor receives start face");
        Check(target_face == 0x10u &&
                  start.x == 1.0f && start.y == 2.0f && start.z == 3.0f &&
                  target.x == 4.0f && target.y == 5.0f &&
                  target.z == 6.0f,
              "actor receives both path endpoints and faces");
    }
    void AddActorPoint(std::uint32_t actor,
                       const pal4::D3dTriangleVector3& point,
                       std::uint32_t face) override {
        events.push_back("point:" + std::to_string(face));
        Check(actor == 0xa0u, "actor address forwarded for each path point");
        path_faces.push_back(face);
        path_points.push_back(point);
    }
    void EndActorPath(std::uint32_t actor) override {
        events.push_back("end:" + std::to_string(actor));
        Check(actor == 0xa0u, "actor address forwarded to path finalization");
        ++actor_ends;
    }
};

pal4::CollisionTreeStorageState FreshState() {
    pal4::CollisionTreeStorageState state{};
    state.words[5] = 41u;
    state.words[8] = 0x1000u;
    state.words[9] = 0x1010u;
    return state;
}

bool Trace(pal4::CollisionTreeStorageState& state,
           std::uint32_t target, std::uint32_t seed_face,
           Runtime& runtime) {
    return pal4::CollisionTreeStorage::TraceFacePath(
        state, 0xa0u, target, {1.0f, 2.0f, 3.0f},
        seed_face, {4.0f, 5.0f, 6.0f}, runtime);
}

void TestEmptyAndExhausted() {
    auto state = FreshState();
    Runtime runtime;
    Check(!Trace(state, 0x30u, 0x10u, runtime) &&
              state.words[5] == 42u && state.words[9] == 0x1000u &&
              state.words[11] == 42u &&
              state.words[12] == std::bit_cast<std::uint32_t>(1.0f) &&
              state.words[13] == std::bit_cast<std::uint32_t>(2.0f) &&
              state.words[14] == std::bit_cast<std::uint32_t>(3.0f) &&
              runtime.actor_begins == 0 && runtime.actor_ends == 0,
          "empty queue still resets seed and generation without actor calls");
    runtime.initial_faces = {0x20u, 0x28u};
    runtime.events.clear();
    Check(!Trace(state, 0x30u, 0x10u, runtime) &&
              state.words[5] == 43u && state.words[9] == 0x1000u &&
              runtime.events == std::vector<std::string>{
                  "seed:16", "propagate:32", "propagate:40"} &&
              runtime.actor_begins == 0,
          "exhausted queue propagates each non-target face then returns false");
}

void TestTargetAndNeighborPath() {
    auto state = FreshState();
    Runtime runtime;
    runtime.initial_faces = {0x20u};
    runtime.propagated_faces[0x20u] = {0x30u};
    runtime.markers = {{0x30u, 1u}, {0x20u, 2u}};
    runtime.next_faces = {{0x30u, 0x20u}, {0x20u, 0x10u}};
    Check(Trace(state, 0x30u, 0x10u, runtime) &&
              state.words[5] == 42u && state.words[9] == 0x1000u &&
              runtime.actor_begins == 1 && runtime.actor_ends == 1 &&
              runtime.begin_generation == 42u &&
              runtime.begin_target.x == 4.0f &&
              runtime.path_faces == std::vector<std::uint32_t>{0x30u, 0x20u} &&
              runtime.path_points[0].x == 48.0f &&
              runtime.path_points[0].y == 49.0f &&
              runtime.path_points[0].z == 1.0f &&
              runtime.path_points[1].z == 2.0f,
          "propagated target builds projected points back to seed face");
    Check(runtime.events == std::vector<std::string>{
              "seed:16", "propagate:32", "begin:160",
              "marker:48", "midpoint:48", "project:48",
              "point:48", "neighbor:48",
              "marker:32", "midpoint:32", "project:32",
              "point:32", "neighbor:32", "end:160"},
          "path callbacks follow native neighbor order");

    state = FreshState();
    runtime = Runtime{};
    runtime.initial_faces = {0x10u};
    Check(Trace(state, 0x10u, 0x10u, runtime) &&
              runtime.actor_begins == 1 && runtime.actor_ends == 1 &&
              runtime.path_faces.empty() &&
              runtime.events == std::vector<std::string>{
                  "seed:16", "begin:160", "end:160"},
          "seed face target skips neighbor traversal");
    state = FreshState();
    runtime = Runtime{};
    runtime.initial_faces = {0u};
    Check(Trace(state, 0u, 0x10u, runtime) &&
              runtime.actor_begins == 1 && runtime.path_faces.empty(),
          "zero target candidate still finalizes actor without path points");
}

void TestGenerationWrapAndStub() {
    auto state = FreshState();
    state.words[5] = 0xffffffffu;
    pal4::CollisionTreePathRuntimeStub stub;
    Check(!pal4::CollisionTreeStorage::TraceFacePath(
              state, 0xa0u, 0x30u, {1.0f, 2.0f, 3.0f},
              0x10u, {4.0f, 5.0f, 6.0f}, stub) &&
              state.words[5] == 0u && state.words[11] == 0u,
          "generation wraps as a 32-bit word and stub is linkable");
}

}  // namespace

int main() {
    TestEmptyAndExhausted();
    TestTargetAndNeighborPath();
    TestGenerationWrapAndStub();
}
