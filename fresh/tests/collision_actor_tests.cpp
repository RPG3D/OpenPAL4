#include "pal4/collision_actor.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <stdexcept>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ID3dFacePathRuntime {
    std::map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> releases;
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return memory.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        memory[address] = value;
    }
    std::uint32_t Allocate(const std::uint32_t size) override {
        allocations.push_back(size);
        return 0x2000u;
    }
    void Free(const std::uint32_t address) override {
        releases.push_back(address);
    }
};

struct Placement final : pal4::ICollisionActorPlacementRuntime {
    std::vector<std::uint32_t> selected_trees;
    std::vector<std::uint32_t> projected_faces;
    pal4::D3dTriangleVector3 last_point{};
    std::uint32_t SelectFace(const std::uint32_t tree,
                             const pal4::D3dTriangleVector3& point) override {
        selected_trees.push_back(tree);
        last_point = point;
        return 0x9000u;
    }
    pal4::D3dTriangleVector3 Project(
        const std::uint32_t, const std::uint32_t face,
        const pal4::D3dTriangleVector3& point) override {
        projected_faces.push_back(face);
        last_point = point;
        return {point.x + 1.0f, point.y + 2.0f, point.z + 3.0f};
    }
};

struct Callbacks final : pal4::ICollisionActorCallbackRuntime {
    std::vector<std::string> events;
    std::uint32_t move_result = 0x1234abu;
    void Output(const std::uint32_t callback,
                const std::uint32_t destination,
                const std::uint32_t source,
                const std::uint32_t tick) override {
        events.push_back("output:" + std::to_string(callback) + ":" +
                         std::to_string(destination) + ":" +
                         std::to_string(source) + ":" +
                         std::to_string(tick));
    }
    void Path(const std::uint32_t callback,
              const std::uint32_t path,
              const std::uint32_t tick) override {
        events.push_back("path:" + std::to_string(callback) + ":" +
                         std::to_string(path) + ":" +
                         std::to_string(tick));
    }
    std::uint32_t Move(const std::uint32_t callback,
                       const std::uint32_t position,
                       const std::uint32_t point,
                       const std::uint32_t tick) override {
        events.push_back("move:" + std::to_string(callback) + ":" +
                         std::to_string(position) + ":" +
                         std::to_string(point) + ":" +
                         std::to_string(tick));
        return move_result;
    }
};

struct Movement final : pal4::ICollisionActorMovementRuntime {
    std::map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> calls;
    std::uint32_t status = 1u;
    std::uint32_t next_face = 0x7000u;
    pal4::D3dTriangleVector3 last_destination{};
    bool clear = false;
    pal4::D3dTriangleVector3 clearance_start{};
    pal4::D3dTriangleVector3 clearance_end{};

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return memory.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        memory[address] = value;
    }
    std::uint32_t Allocate(const std::uint32_t) override { return 0u; }
    void Free(const std::uint32_t) override {}
    std::uint32_t Trace(const std::uint32_t,
                        const pal4::D3dLine2State&,
                        std::uint32_t&) override {
        calls.push_back("trace");
        return 1u;
    }
    void ClampFacePoint(const std::uint32_t face,
                        pal4::D3dTriangleVector3& point) override {
        calls.push_back("clamp:" + std::to_string(face));
        point.y += 1.0f;
    }
    bool TestClearance(
        const std::uint32_t callback,
        const pal4::D3dTriangleVector3& start,
        const pal4::D3dTriangleVector3& end,
        const std::uint32_t event) override {
        calls.push_back("clearance:" + std::to_string(callback) + ":" +
                        std::to_string(event));
        clearance_start = start;
        clearance_end = end;
        return clear;
    }
    std::uint32_t AdvanceAcrossFaces(
        const std::uint32_t tree,
        const pal4::D3dTriangleVector3& start,
        const std::uint32_t face,
        pal4::D3dTriangleVector3& destination,
        std::uint32_t& final_face) override {
        calls.push_back("advance:" + std::to_string(tree) + ":" +
                        std::to_string(face));
        Check(start.x == 1.0f && start.y == 2.0f && start.z == 3.0f,
              "movement forwards current actor position");
        last_destination = destination;
        destination.y = 8.0f;
        final_face = next_face;
        return status;
    }
};

struct PathRuntime final : pal4::ICollisionActorPathRuntime {
    std::vector<std::string> events;
    std::int32_t count = 3;
    std::int32_t random = 8;
    std::uint32_t start_face = 0x7000u;
    bool found = true;
    std::uint32_t first_node = 0x2200u;
    pal4::D3dTriangleVector3 projected_start{};
    pal4::D3dTriangleVector3 target_seen{};
    std::uint32_t target_face_seen = 0u;
    std::uint32_t extra_seen = 0u;

    std::int32_t FaceCount(const std::uint32_t tree) override {
        events.push_back("count:" + std::to_string(tree));
        return count;
    }
    std::int32_t RandomValue() override {
        events.push_back("random");
        return random;
    }
    std::uint32_t FaceAt(const std::uint32_t tree,
                         const std::uint32_t index) override {
        events.push_back("face:" + std::to_string(tree) + ":" +
                         std::to_string(index));
        return 0x9000u;
    }
    pal4::D3dTriangleVector3 Centroid(
        const std::uint32_t face) override {
        events.push_back("centroid:" + std::to_string(face));
        return {8.0f, 9.0f, 10.0f};
    }
    std::uint32_t SelectFace(
        const std::uint32_t tree,
        const pal4::D3dTriangleVector3& point) override {
        events.push_back("select:" + std::to_string(tree));
        Check(point.x == 1.0f && point.y == 2.0f && point.z == 3.0f,
              "path selection receives requested start point");
        return start_face;
    }
    pal4::D3dTriangleVector3 Project(
        const std::uint32_t tree, const std::uint32_t face,
        const pal4::D3dTriangleVector3& point) override {
        events.push_back("project:" + std::to_string(tree) + ":" +
                         std::to_string(face));
        return {point.x, point.y + 1.0f, point.z};
    }
    bool TracePath(
        const std::uint32_t tree, const std::uint32_t path,
        const std::uint32_t face,
        const pal4::D3dTriangleVector3& start,
        const pal4::D3dTriangleVector3& target,
        const std::uint32_t target_face,
        const std::uint32_t extra) override {
        events.push_back("trace:" + std::to_string(tree) + ":" +
                         std::to_string(path) + ":" +
                         std::to_string(face));
        projected_start = start;
        target_seen = target;
        target_face_seen = target_face;
        extra_seen = extra;
        return found;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return first_node;
    }
};

void TestConstructAndDestroyEmpty() {
    Runtime runtime;
    pal4::CollisionActorState state{};
    state.words.fill(0xa5a5a5a5u);
    Check(&pal4::CollisionActor::Construct(state, 0x6bu, runtime) == &state,
          "constructor returns actor state");
    Check(state.words[0u] == pal4::CollisionActor::kVtableAddress &&
              state.words[1u] == 0u && state.words[2u] == 0u &&
              state.words[3u] == 0u && state.words[8u] == 0u &&
              state.words[9u] == 0x42c96148u &&
              state.words[10u] == 0xa5a5a500u &&
              state.words[20u] == 0xa5a5a56bu &&
              state.words[21u] == 0x2000u &&
              state.words[22u] == 0u &&
              state.words[24u] == 0u && state.words[27u] == 0u &&
              state.words[29u] == 0x40a00000u &&
              state.words[36u] == 1u,
          "constructor initializes observed fields and preserves upper flag bytes");
    Check(state.words[11u] == 0xa5a5a5a5u &&
              state.words[23u] == 0xa5a5a5a5u &&
              state.words[28u] == 0xa5a5a5a5u &&
              state.words[39u] == 0xa5a5a5a5u,
          "constructor leaves unspecified actor words untouched");
    Check(runtime.allocations == std::vector<std::uint32_t>{24u} &&
              runtime.memory.at(0x2000u) == 0x2000u &&
              runtime.memory.at(0x2004u) == 0x2000u,
          "constructor allocates self-linked 24-byte path sentinel");
    Check(pal4::CollisionActor::Destroy(state, runtime) == 0u &&
              runtime.releases == std::vector<std::uint32_t>{0x2000u} &&
              state.words[21u] == 0u && state.words[22u] == 0u,
          "empty destructor frees sentinel and clears list fields");
}

void TestDestroyPopulatedPath() {
    Runtime runtime;
    pal4::CollisionActorState state{};
    pal4::CollisionActor::Construct(state, 0u, runtime);
    runtime.memory[0x2000u] = 0x2100u;
    runtime.memory[0x2004u] = 0x2200u;
    runtime.memory[0x2100u] = 0x2200u;
    runtime.memory[0x2104u] = 0x2000u;
    runtime.memory[0x2200u] = 0x2000u;
    runtime.memory[0x2204u] = 0x2100u;
    state.words[22u] = 2u;
    state.words[0u] = 0xdeadbeefu;
    Check(pal4::CollisionActor::Destroy(state, runtime) == 0u &&
              runtime.releases ==
                  std::vector<std::uint32_t>{0x2100u, 0x2200u, 0x2000u} &&
              state.words[0u] == pal4::CollisionActor::kVtableAddress &&
              state.words[21u] == 0u && state.words[22u] == 0u &&
              runtime.memory.at(0x2000u) == 0x2000u &&
              runtime.memory.at(0x2004u) == 0x2000u,
          "destructor unlinks nodes in order before freeing sentinel");
}

void TestPlacementAndClearing() {
    pal4::CollisionActorState state{};
    state.words.fill(0xa5a5a5a5u);
    Placement runtime;
    const pal4::D3dTriangleVector3 point{1.0f, 2.0f, 3.0f};
    Check(pal4::CollisionActor::Place(
              state, 0x1000u, 0u, 0x1111u, point, 0x2222u,
              0x3333u, runtime) == 0u &&
              runtime.selected_trees.empty() &&
              runtime.projected_faces.empty() &&
              state.words[3u] == std::bit_cast<std::uint32_t>(1.0f) &&
              state.words[4u] == std::bit_cast<std::uint32_t>(2.0f) &&
              state.words[5u] == std::bit_cast<std::uint32_t>(3.0f) &&
              state.words[28u] == 0x3333u &&
              state.words[29u] == 0x1111u &&
              state.words[2u] == 0x2222u &&
              state.words[36u] == 1u &&
              state.words[6u] == 0u && state.words[8u] == 0u,
          "placement without tree stores inputs and skips face services");
    Check(pal4::CollisionActor::Place(
              state, 0x1000u, 0x5000u, 0x1111u, point,
              0x7000u, 0x3333u, runtime) == 0x100cu &&
              runtime.selected_trees.empty() &&
              runtime.projected_faces == std::vector<std::uint32_t>{0x7000u} &&
              state.words[2u] == 0x7000u &&
              state.words[3u] == std::bit_cast<std::uint32_t>(2.0f) &&
              state.words[4u] == std::bit_cast<std::uint32_t>(4.0f) &&
              state.words[5u] == std::bit_cast<std::uint32_t>(6.0f),
          "given face projects position and returns actor position address");
    Check(pal4::CollisionActor::Place(
              state, 0x1000u, 0x5000u, 0x1111u, point,
              0u, 0x3333u, runtime) == 0x100cu &&
              runtime.selected_trees == std::vector<std::uint32_t>{0x5000u} &&
              runtime.projected_faces ==
                  std::vector<std::uint32_t>{0x7000u, 0x9000u} &&
              state.words[2u] == 0x9000u && runtime.last_point.x == 1.0f,
          "missing face selects before projecting original point");
    state.words[10u] = 0xa5a5a5ffu;
    state.words[9u] = 0x4444u;
    Check(pal4::CollisionActor::ClearBinding(state) == 0u &&
              state.words[1u] == 0u && state.words[2u] == 0u &&
              state.words[10u] == 0xa5a5a500u &&
              state.words[6u] == 0u && state.words[8u] == 0u &&
              state.words[24u] == 0u && state.words[27u] == 0u &&
              state.words[36u] == 1u && state.words[9u] == 0x4444u,
          "clear resets binding, active flag, velocity and callbacks only");
}

void TestCallbackDispatch() {
    pal4::CollisionActorState state{};
    Callbacks runtime;
    Check(pal4::CollisionActor::SetPathCallback(state, 0x11u) == 0x11u &&
              pal4::CollisionActor::SetMovementCallback(state, 0x22u) ==
                  0x22u &&
              pal4::CollisionActor::SetOutputCallback(state, 0x33u) ==
                  0x33u &&
              pal4::CollisionActor::SetCallback108(state, 0x44u) ==
                  0x44u &&
              state.words[24u] == 0x11u && state.words[25u] == 0x22u &&
              state.words[26u] == 0x33u && state.words[27u] == 0x44u,
          "four callback setters write only their selected slots");
    state.words[23u] = 0x2000u;
    Check(pal4::CollisionActor::DispatchCallbacks(
              state, 0x1000u, 7u, runtime) == 0u &&
              runtime.events ==
                  std::vector<std::string>{"output:51:4228:4216:7"},
          "output callback runs even when path flag is clear");
    state.words[10u] = 1u;
    runtime.events.clear();
    Check(pal4::CollisionActor::DispatchCallbacks(
              state, 0x1000u, 7u, runtime) == 0xabu &&
              runtime.events == std::vector<std::string>{
                  "output:51:4228:4216:7", "path:17:4140:7",
                  "move:34:4108:8200:7"},
          "enabled path calls output, path and movement in order");
    runtime.events.clear();
    static_cast<void>(pal4::CollisionActor::SetMovementCallback(state, 0u));
    Check(pal4::CollisionActor::DispatchCallbacks(
              state, 0x1000u, 8u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "output:51:4228:4216:8", "path:17:4140:8"},
          "missing movement callback leaves earlier callbacks active");
}

void TestMovement() {
    Movement runtime;
    pal4::CollisionActorState state{};
    state.words[3u] = std::bit_cast<std::uint32_t>(1.0f);
    state.words[4u] = std::bit_cast<std::uint32_t>(2.0f);
    state.words[5u] = std::bit_cast<std::uint32_t>(3.0f);
    state.words[6u] = std::bit_cast<std::uint32_t>(4.0f);
    state.words[9u] = std::bit_cast<std::uint32_t>(2.0f);
    state.words[29u] = std::bit_cast<std::uint32_t>(5.0f);
    Check(pal4::CollisionActor::UpdateMovement(
              state, 0x1000u, 9u, 1.0f, runtime) == 0u &&
              runtime.calls.empty() && state.words[6u] ==
                  std::bit_cast<std::uint32_t>(4.0f),
          "unbound actor exits without changing movement state");
    state.words[1u] = 0x5000u;
    state.words[2u] = 0x6000u;
    Check(pal4::CollisionActor::UpdateMovement(
              state, 0x1000u, 9u, 1.0f, runtime) == 0x1084u &&
              runtime.calls == std::vector<std::string>{
                  "advance:20480:24576"} &&
              runtime.last_destination.x == 3.0f &&
              state.words[3u] == std::bit_cast<std::uint32_t>(3.0f) &&
              state.words[4u] == std::bit_cast<std::uint32_t>(8.0f) &&
              state.words[2u] == 0x7000u && state.words[36u] == 1u &&
              state.words[30u] == std::bit_cast<std::uint32_t>(8.0f) &&
              state.words[33u] == std::bit_cast<std::uint32_t>(-2.0f),
          "free velocity decays, caps to step, advances and refreshes bounds");

    state.words[3u] = std::bit_cast<std::uint32_t>(1.0f);
    state.words[4u] = std::bit_cast<std::uint32_t>(2.0f);
    state.words[5u] = std::bit_cast<std::uint32_t>(3.0f);
    state.words[6u] = std::bit_cast<std::uint32_t>(4.0f);
    state.words[7u] = 0u;
    state.words[8u] = 0u;
    state.words[2u] = 0x6000u;
    state.words[10u] = 1u;
    state.words[21u] = 0x2000u;
    state.words[23u] = 0x2100u;
    runtime.memory = {{0x2100u, 0x2000u},
                      {0x2108u, std::bit_cast<std::uint32_t>(1.0f)},
                      {0x210cu, std::bit_cast<std::uint32_t>(2.0f)},
                      {0x2110u, std::bit_cast<std::uint32_t>(3.0f)}};
    runtime.calls.clear();
    Check(pal4::CollisionActor::UpdateMovement(
              state, 0x1000u, 9u, 1.0f, runtime) == 0x1084u &&
              runtime.calls.empty() && state.words[23u] == 0x2000u &&
              (state.words[10u] & 0xffu) == 0u &&
              state.words[6u] == 0u && state.words[8u] == 0u &&
              state.words[36u] == 0u,
          "reached final path node moves cursor to sentinel and stops path");

    state.words[10u] = 0u;
    state.words[6u] = std::bit_cast<std::uint32_t>(4.0f);
    runtime.status = 0u;
    Check(pal4::CollisionActor::UpdateMovement(
              state, 0x1000u, 10u, 1.0f, runtime) == 0x1084u &&
              state.words[36u] == 3u && state.words[6u] == 0u &&
              state.words[3u] == std::bit_cast<std::uint32_t>(1.0f) &&
              state.words[37u] == std::bit_cast<std::uint32_t>(3.0f) &&
              state.words[38u] == std::bit_cast<std::uint32_t>(8.0f),
          "status zero records stopped point without moving actor");
    state.words[6u] = std::bit_cast<std::uint32_t>(4.0f);
    runtime.status = 3u;
    Check(pal4::CollisionActor::UpdateMovement(
              state, 0x1000u, 11u, 1.0f, runtime) == 0x1084u &&
              state.words[36u] == 2u && state.words[6u] == 0u,
          "status three reports crossing limit and clears velocity");
}

void TestStopAndPathRequests() {
    pal4::CollisionActorState state{};
    PathRuntime runtime;
    const pal4::D3dTriangleVector3 start{1.0f, 2.0f, 3.0f};
    const pal4::D3dTriangleVector3 target{4.0f, 5.0f, 6.0f};
    state.words[6u] = std::bit_cast<std::uint32_t>(4.0f);
    state.words[10u] = 0xa5a5a501u;
    Check(pal4::CollisionActor::StopMovement(state) == 0u &&
              state.words[6u] == 0u && state.words[7u] == 0u &&
              state.words[8u] == 0u && state.words[10u] == 0xa5a5a500u,
          "stop clears only path byte and velocity");
    Check(!pal4::CollisionActor::RequestPath(
              state, 0x1000u, start, target, 0x8000u, 0x55u,
              runtime) && runtime.events.empty(),
          "path request without tree exits before mutating state");
    Check(!pal4::CollisionActor::RequestRandomPath(
              state, 0x1000u, start, 0x55u, runtime) &&
              runtime.events.empty(),
          "random path request without tree skips RNG and face access");

    state.words[1u] = 0x5000u;
    state.words[21u] = 0x2000u;
    runtime.start_face = 0u;
    state.words[10u] = 0xa5a5a501u;
    state.words[6u] = std::bit_cast<std::uint32_t>(3.0f);
    Check(pal4::CollisionActor::RequestPath(
              state, 0x1000u, start, target, 0x8000u, 0x55u,
              runtime) &&
              runtime.events == std::vector<std::string>{"select:20480"} &&
              state.words[2u] == 0u && state.words[6u] == 0u &&
              state.words[3u] == std::bit_cast<std::uint32_t>(1.0f),
          "missing start face leaves previous path flag and skips trace");

    runtime.events.clear();
    runtime.start_face = 0x7000u;
    Check(pal4::CollisionActor::RequestPath(
              state, 0x1000u, start, target, 0x8000u, 0x55u,
              runtime) &&
              runtime.events == std::vector<std::string>{
                  "select:20480", "project:20480:28672",
                  "trace:20480:4140:28672", "read:8192"} &&
              state.words[4u] == std::bit_cast<std::uint32_t>(3.0f) &&
              state.words[23u] == 0x2200u &&
              (state.words[10u] & 0xffu) == 1u &&
              runtime.projected_start.y == 3.0f &&
              runtime.target_seen.x == 4.0f &&
              runtime.target_face_seen == 0x8000u &&
              runtime.extra_seen == 0x55u,
          "successful path request projects start and selects first node");
    runtime.events.clear();
    runtime.found = false;
    Check(!pal4::CollisionActor::RequestPath(
              state, 0x1000u, start, target, 0x8000u, 0x55u,
              runtime) &&
              (state.words[10u] & 0xffu) == 0u &&
              state.words[23u] == 0x2200u &&
              runtime.events.size() == 3u,
          "failed trace clears path flag without reading first node");

    runtime.found = true;
    runtime.events.clear();
    Check(pal4::CollisionActor::RequestRandomPath(
              state, 0x1000u, start, 0x66u, runtime) &&
              runtime.events == std::vector<std::string>{
                  "count:20480", "random", "face:20480:2",
                  "centroid:36864", "select:20480",
                  "project:20480:28672", "trace:20480:4140:28672",
                  "read:8192"} &&
              runtime.target_seen.x == 8.0f &&
              runtime.target_face_seen == 0x9000u &&
              runtime.extra_seen == 0x66u,
          "random path chooses face modulo count and delegates request");
    runtime.count = 0;
    bool threw = false;
    try {
        static_cast<void>(pal4::CollisionActor::RequestRandomPath(
            state, 0x1000u, start, 0u, runtime));
    } catch (const std::domain_error&) {
        threw = true;
    }
    Check(threw, "empty face list reports division precondition");
}

void TestRedirectMovement() {
    Movement runtime;
    pal4::CollisionActorState state{};
    state.words[1u] = 0x5000u;
    state.words[2u] = 0x6000u;
    state.words[3u] = std::bit_cast<std::uint32_t>(1.0f);
    state.words[4u] = std::bit_cast<std::uint32_t>(2.0f);
    state.words[5u] = std::bit_cast<std::uint32_t>(3.0f);
    state.words[21u] = 0x2000u;
    state.words[23u] = 0x2100u;
    state.words[27u] = 0u;
    state.words[28u] = std::bit_cast<std::uint32_t>(0.5f);
    runtime.memory = {{0x2100u, 0x2200u},
                      {0x2200u, 0x2300u},
                      {0x2208u, std::bit_cast<std::uint32_t>(5.0f)},
                      {0x220cu, std::bit_cast<std::uint32_t>(2.0f)},
                      {0x2210u, std::bit_cast<std::uint32_t>(3.0f)},
                      {0x2214u, 0x8000u},
                      {0x2300u, 0x2000u},
                      {0x2308u, std::bit_cast<std::uint32_t>(8.0f)},
                      {0x230cu, std::bit_cast<std::uint32_t>(2.0f)},
                      {0x2310u, std::bit_cast<std::uint32_t>(3.0f)},
                      {0x2314u, 0x9000u}};
    Check(!pal4::CollisionActor::RedirectMovement(
              state, 2.0f, 7u, runtime) && runtime.calls.empty(),
          "disabled clearance callback returns without reading path");
    state.words[27u] = 0x44u;
    runtime.clear = false;
    Check(!pal4::CollisionActor::RedirectMovement(
              state, 2.0f, 7u, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "clamp:24576", "clearance:68:7"} &&
              state.words[23u] == 0x2100u &&
              runtime.clearance_start.y == 3.5f &&
              runtime.clearance_end.y == 2.5f,
          "rejected clearance preserves cursor and applies height offset");
    runtime.calls.clear();
    runtime.clear = true;
    Check(pal4::CollisionActor::RedirectMovement(
              state, 2.0f, 8u, runtime) &&
              state.words[23u] == 0x2300u &&
              state.words[6u] == std::bit_cast<std::uint32_t>(2.0f) &&
              state.words[7u] == 0u && state.words[8u] == 0u &&
              runtime.calls == std::vector<std::string>{
                  "clamp:24576", "clearance:68:8", "trace",
                  "clearance:68:8", "trace"},
          "clear path advances to farthest face and caps velocity to step");
    runtime.calls.clear();
    Check(!pal4::CollisionActor::RedirectMovement(
              state, 2.0f, 8u, runtime) && runtime.calls.empty(),
          "last path node has no successor to redirect toward");
}

void TestActorAccessors() {
    pal4::CollisionActorState state{};
    state.words.fill(0xa5a5a5a5u);
    state.words[6u] = std::bit_cast<std::uint32_t>(1.0f);
    state.words[7u] = std::bit_cast<std::uint32_t>(2.0f);
    state.words[8u] = std::bit_cast<std::uint32_t>(3.0f);
    Check(pal4::CollisionActor::AddVelocity(
              state, 0x1000u, {4.0f, -2.0f, 5.0f}) == 0x1018u &&
              state.words[6u] == std::bit_cast<std::uint32_t>(5.0f) &&
              state.words[7u] == std::bit_cast<std::uint32_t>(0.0f) &&
              state.words[8u] == std::bit_cast<std::uint32_t>(8.0f) &&
              state.words[5u] == 0xa5a5a5a5u,
          "velocity accessor adds vector at +24 and returns its address");
    Check(pal4::CollisionActor::SetSpeedBits(state, 0x7fc00001u) ==
              0x7fc00001u && state.words[9u] == 0x7fc00001u,
          "speed setter preserves raw float bits and returns input");
    state.words[10u] = 0x123456abu;
    Check(pal4::CollisionActor::PathEnabled(state) == 0xabu &&
              pal4::CollisionActor::PositionAddress(0x1000u) == 0x100cu,
          "path getter reads only low byte and position accessor uses +12");
}

}  // namespace

int main() {
    TestConstructAndDestroyEmpty();
    TestDestroyPopulatedPath();
    TestPlacementAndClearing();
    TestCallbackDispatch();
    TestMovement();
    TestStopAndPathRequests();
    TestRedirectMovement();
    TestActorAccessors();
}
