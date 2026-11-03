#include "pal4/config_composite_collision.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IConfigCompositeCollisionRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> memory;
    std::unordered_map<std::uint32_t, bool> patch_mesh;
    std::unordered_map<std::uint32_t, bool> hierarchy;
    std::unordered_map<std::uint32_t, bool> plugin_data;
    std::unordered_map<std::uint32_t, pal4::CollisionPoint3> vertex_positions;
    std::vector<bool> check_results;
    std::vector<bool> process_results;
    std::vector<std::string> events;
    std::vector<pal4::ConfigCollisionPosition> origins;
    std::vector<pal4::CollisionSegment> segments;
    std::vector<std::uint32_t> atomics;
    std::vector<pal4::CollisionPoint3> magnitude_deltas;
    std::vector<float> magnitude_results;
    std::optional<pal4::CollisionPoint3> first_movement;
    pal4::CollisionPoint3* expected_normal = nullptr;
    float* expected_distance = nullptr;
    pal4::CollisionPoint3 normalized_result{};
    bool check_output_pointers = true;
    std::size_t normalize_calls = 0u;
    std::uint32_t expected_mode = 1u;
    std::uint32_t last_previous_state = 0xffffffffu;
    bool abort_on_enter = false;
    pal4::CollisionPoint3 transformed_normal{};
    pal4::CollisionPoint3 last_normalize_input{};
    std::uint32_t last_transform = 0u;
    std::uint32_t last_frame = 0u;
    std::uint32_t last_intersection_callback = 0u;
    pal4::ConfigCollisionSphereQuery last_query{};
    pal4::CollisionPoint3 candidate_point{};
    float candidate_distance = 0.0f;
    bool candidate_enabled = false;
    pal4::ConfigCollisionScratch clump_scratch{};
    pal4::CollisionSegment last_clump_segment{};
    std::uint32_t clump_collision_result = 0u;
    std::uint32_t last_clump = 0u;
    std::size_t clump_calls = 0u;
    std::size_t check_index = 0u;
    std::size_t process_index = 0u;

    [[nodiscard]] std::uint32_t ReadWord(const std::uint32_t address) override {
        const auto found = memory.find(address);
        Check(found != memory.end(), "list read uses initialized address");
        return found->second;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        memory[address] = value;
        events.push_back("write:" + std::to_string(address));
    }

    [[nodiscard]] bool CheckLineSegment(
        const pal4::ConfigCollisionPosition& origin,
        const std::uint32_t atomic, pal4::CollisionPoint3& movement,
        pal4::CollisionPoint3* normal, float* distance,
        const std::uint32_t mode) override {
        Check(mode == expected_mode &&
                  (!check_output_pointers ||
                   (normal == expected_normal && distance == expected_distance)),
              "segment helper receives native mode and output pointers");
        Check(check_index < check_results.size(),
              "segment helper called expected number of times");
        origins.push_back(origin);
        atomics.push_back(atomic);
        events.push_back("check:" + std::to_string(atomic));
        if (check_index == 0u && first_movement.has_value()) {
            movement = *first_movement;
        }
        return check_results[check_index++];
    }

    [[nodiscard]] bool ProcessWorldCollision(
        const pal4::CollisionSegment& segment,
        const std::uint32_t atomic,
        pal4::ConfigCollisionScratch& scratch) override {
        Check(process_index < process_results.size(),
              "world collision called expected number of times");
        Check(scratch == pal4::ConfigCollisionScratch{},
              "C++ collision scratch starts initialized");
        segments.push_back(segment);
        atomics.push_back(atomic);
        events.push_back("process:" + std::to_string(atomic));
        return process_results[process_index++];
    }

    void NormalizeMovement(pal4::CollisionPoint3& output,
                           const pal4::CollisionPoint3& input) override {
        ++normalize_calls;
        last_normalize_input = input;
        output = normalized_result;
        events.push_back("normalize");
    }
    [[nodiscard]] bool HasPatchMesh(const std::uint32_t atomic) override {
        events.push_back("patch:" + std::to_string(atomic));
        return patch_mesh[atomic];
    }
    [[nodiscard]] bool HasHierarchy(const std::uint32_t atomic) override {
        events.push_back("hierarchy:" + std::to_string(atomic));
        return hierarchy[atomic];
    }
    [[nodiscard]] bool HasPluginData(const std::uint32_t atomic) override {
        events.push_back("plugin:" + std::to_string(atomic));
        const auto found = plugin_data.find(atomic);
        return found == plugin_data.end() ? true : found->second;
    }
    void NotifyEnter(const std::uint32_t callback,
                     const std::uint32_t previous_state,
                     pal4::ConfigCompositeState& state) override {
        last_previous_state = previous_state;
        events.push_back("enter:" + std::to_string(callback));
        if (abort_on_enter) state.words[22u] = 1u;
    }
    void NotifyLeave(const std::uint32_t callback,
                     pal4::ConfigCompositeState&) override {
        events.push_back("leave:" + std::to_string(callback));
    }
    [[nodiscard]] pal4::CollisionPoint3 VertexWorldPosition(
        const std::uint32_t vertex_reference,
        const std::uint32_t transform) override {
        last_transform = transform;
        events.push_back("vertex:" + std::to_string(vertex_reference));
        return vertex_positions.at(vertex_reference);
    }
    [[nodiscard]] pal4::CollisionPoint3 TransformNormal(
        const pal4::CollisionPoint3& normal,
        const std::uint32_t transform) override {
        Check(normal == pal4::CollisionPoint3{0.0f, 0.0f, 1.0f},
              "triangle callback passes native normal");
        last_transform = transform;
        events.push_back("transform");
        return transformed_normal;
    }
    [[nodiscard]] float VectorMagnitudeApprox(
        const pal4::CollisionPoint3& delta) override {
        magnitude_deltas.push_back(delta);
        events.push_back("magnitude");
        if (magnitude_deltas.size() <= magnitude_results.size()) {
            return magnitude_results[magnitude_deltas.size() - 1u];
        }
        return std::sqrt(delta[0u] * delta[0u] +
                         delta[1u] * delta[1u] +
                         delta[2u] * delta[2u]);
    }
    [[nodiscard]] std::uint32_t ObjectTransform(
        const std::uint32_t frame) override {
        last_frame = frame;
        events.push_back("object-transform");
        return 0x9000u;
    }
    void ForAllAtomicIntersections(
        const std::uint32_t atomic,
        const pal4::ConfigCollisionSphereQuery& query,
        const std::uint32_t callback,
        pal4::ConfigCollisionClosestPoint& closest) override {
        Check(atomic == 0x8000u &&
                  closest.object_transform == 0x9000u &&
                  closest.distance == std::numeric_limits<float>::max(),
              "atomic traversal receives initialized closest context");
        last_query = query;
        last_intersection_callback = callback;
        events.push_back("intersections");
        if (candidate_enabled) {
            closest.point = candidate_point;
            closest.distance = candidate_distance;
        }
    }
    [[nodiscard]] std::uint32_t ProcessClumpCollision(
        const pal4::CollisionSegment& segment,
        const std::uint32_t clump,
        pal4::ConfigCollisionScratch& scratch) override {
        Check(scratch == pal4::ConfigCollisionScratch{},
              "clump collision scratch starts initialized");
        last_clump_segment = segment;
        last_clump = clump;
        ++clump_calls;
        events.push_back("clump-collision");
        scratch = clump_scratch;
        return clump_collision_result;
    }
};

void LinkList(Runtime& runtime, pal4::ConfigCompositeState& state,
              const std::vector<std::pair<std::uint32_t, bool>>& nodes,
              const std::uint32_t list_word = 5u) {
    const auto head = list_word == 5u ? 0x3000u
                    : list_word == 8u ? 0x6000u : 0x7000u;
    state.words[list_word] = head;
    runtime.memory[head] = nodes.empty() ? head : nodes.front().first;
    for (std::size_t index = 0u; index < nodes.size(); ++index) {
        const auto node = nodes[index].first;
        runtime.memory[node] = index + 1u == nodes.size()
            ? head : nodes[index + 1u].first;
        runtime.memory[node + 8u] = node + 0x100u;
        runtime.memory[node + 20u] = nodes[index].second ? 1u : 0u;
    }
}

void LinkFourthList(Runtime& runtime, pal4::ConfigCompositeState& state,
                    const std::vector<std::pair<std::uint32_t, bool>>& nodes) {
    LinkList(runtime, state, nodes, 11u);
    for (const auto& entry : nodes) {
        const auto node = entry.first;
        runtime.memory[node + 12u] = 1u;
        runtime.memory[node + 16u] = 0u;
        runtime.memory[node + 24u] = 0u;
        runtime.memory[node + 28u] = 0u;
        runtime.memory[node + 0x100u + 60u] = 3u;
    }
}

void TestInputGatesAndEmptyList() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    LinkList(runtime, state, {});
    const pal4::ConfigCollisionPosition start{1.0f, 2.0f, 3.0f, 4.0f};
    pal4::ConfigCollisionPosition position{5.0f, 6.0f, 7.0f, 8.0f};
    pal4::CollisionPoint3 movement{9.0f, 10.0f, 11.0f};
    Check(!pal4::ConfigCompositeCollision::CheckSecondList(
              state, nullptr, &position, movement, 0u, nullptr, nullptr,
              runtime),
          "null start returns false");
    Check(!pal4::ConfigCompositeCollision::CheckSecondList(
              state, &start, nullptr, movement, 0u, nullptr, nullptr,
              runtime),
          "null position returns false");
    Check(!pal4::ConfigCompositeCollision::CheckSecondList(
              state, &start, &position, movement, 123u, nullptr, nullptr,
              runtime) && runtime.events.empty() &&
              position == pal4::ConfigCollisionPosition{5.0f, 6.0f, 7.0f, 8.0f} &&
              movement == pal4::CollisionPoint3{9.0f, 10.0f, 11.0f},
          "empty second list does not change inputs");
}

void TestFirstHitAdvancesPosition() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    LinkList(runtime, state, {{0x4000u, true}});
    runtime.check_results = {true, false};
    runtime.first_movement = pal4::CollisionPoint3{2.0f, 3.0f, 4.0f};
    const pal4::ConfigCollisionPosition start{1.0f, 2.0f, 3.0f, 4.0f};
    pal4::ConfigCollisionPosition position{10.0f, 20.0f, 30.0f, 99.0f};
    pal4::CollisionPoint3 movement{1.0f, 1.0f, 1.0f};
    pal4::CollisionPoint3 normal{};
    float distance = 0.0f;
    runtime.expected_normal = &normal;
    runtime.expected_distance = &distance;
    Check(pal4::ConfigCompositeCollision::CheckSecondList(
              state, &start, &position, movement, 0u, &normal, &distance,
              runtime),
          "first segment hit returns true");
    Check(position == pal4::ConfigCollisionPosition{
              12.0f, 23.0f, 34.0f, 99.0f} &&
              runtime.origins == std::vector<pal4::ConfigCollisionPosition>{
                  start, position} &&
              runtime.events == std::vector<std::string>{
                  "check:16640", "check:16640"} &&
              runtime.process_index == 0u,
          "position advances by post-helper movement before second check");
}

void TestSecondHitReturnsWithoutHardBlock() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    LinkList(runtime, state, {{0x4000u, false}, {0x5000u, true}});
    runtime.check_results = {false, true};
    const pal4::ConfigCollisionPosition start{0.0f, 1.0f, 2.0f, 3.0f};
    pal4::ConfigCollisionPosition position{4.0f, 5.0f, 6.0f, 7.0f};
    pal4::CollisionPoint3 movement{1.0f, 2.0f, 3.0f};
    Check(pal4::ConfigCompositeCollision::CheckSecondList(
              state, &start, &position, movement, 0u, nullptr, nullptr,
              runtime) &&
              position == pal4::ConfigCollisionPosition{4.0f, 5.0f, 6.0f, 7.0f} &&
              runtime.check_index == 2u && runtime.process_index == 0u,
          "second hit returns before later nodes or world processing");
}

void TestHardBlockTraversesAndStopsMovement() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    LinkList(runtime, state, {{0x4000u, true}, {0x5000u, true}});
    runtime.check_results = {false, false, false, false};
    runtime.process_results = {false, true};
    const pal4::ConfigCollisionPosition start{0.0f, 1.0f, 2.0f, 3.0f};
    pal4::ConfigCollisionPosition position{4.0f, 5.0f, 6.0f, 7.0f};
    pal4::CollisionPoint3 movement{1.0f, 2.0f, 3.0f};
    Check(pal4::ConfigCompositeCollision::CheckSecondList(
              state, &start, &position, movement, 123u, nullptr, nullptr,
              runtime) && movement == pal4::CollisionPoint3{0.0f, 0.0f, 0.0f} &&
              runtime.events == std::vector<std::string>{
                  "check:16640", "check:16640", "process:16640",
                  "check:20736", "check:20736", "process:20736"} &&
              runtime.segments.size() == 2u &&
              runtime.segments[0u].start == pal4::CollisionPoint3{4.0f, 5.0f, 6.0f} &&
              runtime.segments[0u].end == pal4::CollisionPoint3{5.0f, 7.0f, 9.0f} &&
              runtime.segments[1u].start == pal4::CollisionPoint3{4.0f, 5.0f, 6.0f} &&
              runtime.segments[1u].end == pal4::CollisionPoint3{5.0f, 7.0f, 9.0f},
          "world hit after second node zeroes movement and returns true");
}

void TestUnflaggedMissTraversesWithoutWorldCall() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    LinkList(runtime, state, {{0x4000u, false}, {0x5000u, false}});
    runtime.check_results = {false, false, false, false};
    const pal4::ConfigCollisionPosition start{};
    pal4::ConfigCollisionPosition position{};
    pal4::CollisionPoint3 movement{1.0f, 0.0f, 0.0f};
    Check(!pal4::ConfigCompositeCollision::CheckSecondList(
              state, &start, &position, movement, 0u, nullptr, nullptr,
              runtime) && runtime.check_index == 4u &&
              runtime.process_index == 0u,
          "unflagged misses scan every node without world collision");
}

void TestThirdListInputGates() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    LinkList(runtime, state, {}, 8u);
    const pal4::ConfigCollisionPosition start{1.0f, 2.0f, 3.0f, 4.0f};
    const pal4::ConfigCollisionPosition position{5.0f, 6.0f, 7.0f, 8.0f};
    pal4::CollisionPoint3 movement{9.0f, 10.0f, 11.0f};
    std::uint32_t blocked = 17u;
    Check(!pal4::ConfigCompositeCollision::CheckThirdList(
              state, nullptr, &position, movement, blocked, 1u, 2u,
              runtime) && blocked == 17u,
          "null start leaves hard-block output untouched");
    Check(!pal4::ConfigCompositeCollision::CheckThirdList(
              state, &start, nullptr, movement, blocked, 1u, 2u,
              runtime) && blocked == 17u,
          "null position leaves hard-block output untouched");
    Check(!pal4::ConfigCompositeCollision::CheckThirdList(
              state, &start, &position, movement, blocked, 1u, 2u,
              runtime) && blocked == 0u && runtime.events.empty(),
          "empty third list clears hard-block output");
}

void TestThirdListImmediateHardBlock() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    LinkList(runtime, state, {{0x4000u, true}}, 8u);
    runtime.process_results = {true};
    const pal4::ConfigCollisionPosition start{1.0f, 2.0f, 3.0f, 4.0f};
    const pal4::ConfigCollisionPosition position{5.0f, 6.0f, 7.0f, 8.0f};
    pal4::CollisionPoint3 movement{1.0f, 2.0f, 3.0f};
    std::uint32_t blocked = 0u;
    Check(pal4::ConfigCompositeCollision::CheckThirdList(
              state, &start, &position, movement, blocked, 0u, 0u,
              runtime) && blocked == 1u &&
              movement == pal4::CollisionPoint3{0.0f, 0.0f, 0.0f} &&
              runtime.events == std::vector<std::string>{"process:16640"} &&
              runtime.segments[0u].start == pal4::CollisionPoint3{5.0f, 6.0f, 7.0f} &&
              runtime.segments[0u].end == pal4::CollisionPoint3{6.0f, 8.0f, 10.0f},
          "pre-check hard block clears movement and sets output");
}

void TestThirdListNoIntersections() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    LinkList(runtime, state, {{0x4000u, false}, {0x5000u, false}}, 8u);
    runtime.check_output_pointers = false;
    runtime.check_results = {false, false, false, false};
    const pal4::ConfigCollisionPosition start{1.0f, 2.0f, 3.0f, 4.0f};
    const pal4::ConfigCollisionPosition position{5.0f, 6.0f, 7.0f, 8.0f};
    pal4::CollisionPoint3 movement{1.0f, 2.0f, 3.0f};
    std::uint32_t blocked = 17u;
    Check(!pal4::ConfigCompositeCollision::CheckThirdList(
              state, &start, &position, movement, blocked, 0u, 0u,
              runtime) && blocked == 0u && runtime.check_index == 4u &&
              runtime.normalize_calls == 0u && runtime.process_index == 0u,
          "third-list misses scan all nodes without forward probe");
}

void TestThirdListForwardProbeStopsMovement() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    LinkList(runtime, state, {{0x4000u, false}}, 8u);
    runtime.check_output_pointers = false;
    runtime.check_results = {false, true};
    runtime.process_results = {true};
    runtime.normalized_result = {0.0f, 1.0f, 0.0f};
    const pal4::ConfigCollisionPosition start{1.0f, 2.0f, 3.0f, 2.0f};
    const pal4::ConfigCollisionPosition position{5.0f, 6.0f, 7.0f, 8.0f};
    pal4::CollisionPoint3 movement{1.0f, 2.0f, 3.0f};
    std::uint32_t blocked = 17u;
    Check(pal4::ConfigCompositeCollision::CheckThirdList(
              state, &start, &position, movement, blocked, 0u, 0u,
              runtime) && blocked == 0u &&
              movement == pal4::CollisionPoint3{0.0f, 0.0f, 0.0f} &&
              runtime.normalize_calls == 1u &&
              runtime.events == std::vector<std::string>{
                  "check:16640", "check:16640", "normalize", "process:16640"} &&
              runtime.segments[0u].start == pal4::CollisionPoint3{1.0f, 2.0f, 3.0f} &&
              runtime.segments[0u].end == pal4::CollisionPoint3{1.0f, 5.0f, 3.0f},
          "forward probe uses 1.5 times start radius and normalized movement");
}

void TestThirdListForwardProbeTraverses() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    LinkList(runtime, state, {{0x4000u, true}, {0x5000u, false}}, 8u);
    runtime.check_output_pointers = false;
    runtime.check_results = {true, false, false, false};
    runtime.process_results = {false, false, false};
    runtime.normalized_result = {1.0f, 0.0f, 0.0f};
    const pal4::ConfigCollisionPosition start{1.0f, 2.0f, 3.0f, 2.0f};
    const pal4::ConfigCollisionPosition position{5.0f, 6.0f, 7.0f, 8.0f};
    pal4::CollisionPoint3 movement{1.0f, 2.0f, 3.0f};
    std::uint32_t blocked = 17u;
    Check(pal4::ConfigCompositeCollision::CheckThirdList(
              state, &start, &position, movement, blocked, 0u, 0u,
              runtime) && blocked == 0u &&
              movement == pal4::CollisionPoint3{1.0f, 2.0f, 3.0f} &&
              runtime.events == std::vector<std::string>{
                  "process:16640", "check:16640", "check:16640",
                  "check:20736", "check:20736", "normalize",
                  "process:16640", "process:20736"},
          "post-hit forward probe walks every node when no block occurs");
}

void TestFourthListInputAndFilterGates() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    LinkFourthList(runtime, state, {});
    const pal4::ConfigCollisionPosition start{};
    const pal4::ConfigCollisionPosition position{};
    pal4::CollisionPoint3 movement{1.0f, 2.0f, 3.0f};
    std::uint32_t blocked = 17u;
    state.words[22u] = 9u;
    Check(!pal4::ConfigCompositeCollision::CheckFourthList(
              state, nullptr, &position, 4u, movement, blocked, 0u, 0u,
              runtime) && blocked == 17u && state.words[22u] == 9u,
          "null fourth-list start leaves outputs unchanged");
    Check(!pal4::ConfigCompositeCollision::CheckFourthList(
              state, &start, &position, 4u, movement, blocked, 0u, 0u,
              runtime) && blocked == 0u && state.words[22u] == 0u,
          "empty fourth list clears output and abort flag");

    LinkFourthList(runtime, state,
                   {{0x4000u, false}, {0x5000u, false}, {0x6000u, false}});
    runtime.memory[0x4100u + 60u] = 4u;
    runtime.patch_mesh[0x5100u] = true;
    runtime.plugin_data[0x6100u] = false;
    Check(!pal4::ConfigCompositeCollision::CheckFourthList(
              state, &start, &position, 4u, movement, blocked, 0u, 0u,
              runtime) && runtime.check_index == 0u &&
              runtime.process_index == 0u &&
              runtime.events == std::vector<std::string>{
                  "patch:20736", "hierarchy:20736",
                  "patch:24832", "plugin:24832"},
          "type, patch-without-hierarchy and missing-plugin gates skip nodes");
}

void TestFourthListImmediateHardBlock() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    LinkFourthList(runtime, state, {{0x4000u, true}});
    runtime.process_results = {true};
    const pal4::ConfigCollisionPosition start{1.0f, 2.0f, 3.0f, 4.0f};
    const pal4::ConfigCollisionPosition position{5.0f, 6.0f, 7.0f, 8.0f};
    pal4::CollisionPoint3 movement{1.0f, 2.0f, 3.0f};
    std::uint32_t blocked = 17u;
    Check(pal4::ConfigCompositeCollision::CheckFourthList(
              state, &start, &position, 4u, movement, blocked, 0u, 0u,
              runtime) && blocked == 1u &&
              movement == pal4::CollisionPoint3{0.0f, 0.0f, 0.0f} &&
              runtime.check_index == 0u && runtime.process_index == 1u &&
              runtime.segments[0u].start == pal4::CollisionPoint3{5.0f, 6.0f, 7.0f} &&
              runtime.segments[0u].end == pal4::CollisionPoint3{6.0f, 8.0f, 10.0f},
          "fourth-list hard block precedes segment checks");
}

void TestFourthListDirectEnterAndAbort() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    LinkFourthList(runtime, state,
                   {{0x4000u, false}, {0x5000u, false}});
    runtime.memory[0x4000u + 16u] = 7u;
    runtime.memory[0x4000u + 28u] = 0x9000u;
    runtime.check_results = {true, false};
    runtime.check_output_pointers = false;
    runtime.abort_on_enter = true;
    const pal4::ConfigCollisionPosition start{};
    const pal4::ConfigCollisionPosition position{};
    pal4::CollisionPoint3 movement{};
    std::uint32_t blocked = 17u;
    Check(pal4::ConfigCompositeCollision::CheckFourthList(
              state, &start, &position, 4u, movement, blocked, 0u, 0u,
              runtime) && blocked == 0u && state.words[22u] == 0u &&
              runtime.memory[0x4000u + 16u] == 1u &&
              runtime.last_previous_state == 7u && runtime.check_index == 2u &&
              runtime.events == std::vector<std::string>{
                  "patch:16640", "plugin:16640",
                  "check:16640", "check:16640", "enter:36864", "write:16400"},
          "enter callback receives previous state and aborts before next node");
}

void TestFourthListProximityAndLeave() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    LinkFourthList(runtime, state, {{0x4000u, false}});
    runtime.memory[0x4000u + 24u] = 1u;
    runtime.memory[0x4000u + 28u] = 0x9000u;
    runtime.check_results = {false, false};
    runtime.process_results = {true};
    runtime.check_output_pointers = false;
    const pal4::ConfigCollisionPosition start{};
    const pal4::ConfigCollisionPosition position{5.0f, 20.0f, 7.0f, 2.0f};
    pal4::CollisionPoint3 movement{};
    std::uint32_t blocked = 17u;
    Check(pal4::ConfigCompositeCollision::CheckFourthList(
              state, &start, &position, 4u, movement, blocked, 0u, 0u,
              runtime) && blocked == 0u &&
              runtime.memory[0x4000u + 16u] == 1u &&
              runtime.segments[0u].start == pal4::CollisionPoint3{5.0f, 20.0f, 7.0f} &&
              runtime.segments[0u].end == pal4::CollisionPoint3{5.0f, 15.0f, 7.0f},
          "proximity hit probes down by 2.5 times height and enters");

    runtime.events.clear();
    runtime.check_index = 0u;
    runtime.process_index = 0u;
    runtime.process_results = {false};
    Check(!pal4::ConfigCompositeCollision::CheckFourthList(
              state, &start, &position, 4u, movement, blocked, 0u, 0u,
              runtime) && runtime.memory[0x4000u + 16u] == 0u &&
              runtime.events == std::vector<std::string>{
                  "patch:16640", "plugin:16640", "check:16640",
                  "check:16640", "process:16640", "leave:36864", "write:16400"},
          "proximity miss invokes leave and clears entered state");
}

void TestFourthListWithoutCallbackRetainsState() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    LinkFourthList(runtime, state, {{0x4000u, false}});
    runtime.memory[0x4000u + 16u] = 7u;
    runtime.check_results = {false, false};
    runtime.check_output_pointers = false;
    const pal4::ConfigCollisionPosition start{};
    const pal4::ConfigCollisionPosition position{};
    pal4::CollisionPoint3 movement{};
    std::uint32_t blocked = 0u;
    Check(!pal4::ConfigCompositeCollision::CheckFourthList(
              state, &start, &position, 4u, movement, blocked, 0u, 0u,
              runtime) && runtime.memory[0x4000u + 16u] == 7u,
          "missing callback leaves entered state unchanged");
}

pal4::ConfigCollisionTriangle MakeTriangle(Runtime& runtime) {
    runtime.vertex_positions[0x100u] = {0.0f, 0.0f, 0.0f};
    runtime.vertex_positions[0x200u] = {2.0f, 0.0f, 0.0f};
    runtime.vertex_positions[0x300u] = {0.0f, 2.0f, 0.0f};
    runtime.transformed_normal = {0.0f, 0.0f, 2.0f};
    runtime.normalized_result = {0.0f, 0.0f, 1.0f};
    pal4::ConfigCollisionTriangle triangle{};
    triangle.normal = {0.0f, 0.0f, 1.0f};
    triangle.vertex_references = {0x100u, 0x200u, 0x300u};
    return triangle;
}

void TestTriangleCallbackInsideAndStrictDistance() {
    Runtime runtime;
    const auto triangle = MakeTriangle(runtime);
    pal4::ConfigCollisionClosestPoint closest{
        {9.0f, 9.0f, 9.0f}, 10.0f, 0x4400u};
    const pal4::CollisionPoint3 query{0.5f, 0.5f, 3.0f};
    Check(&pal4::ConfigCompositeCollision::RecordClosestTrianglePoint(
              query, triangle, 0u, closest, runtime) == &triangle &&
              closest.point == pal4::CollisionPoint3{0.5f, 0.5f, 0.0f} &&
              closest.distance == 3.0f &&
              runtime.last_transform == 0x4400u &&
              runtime.magnitude_deltas.empty() &&
              runtime.events == std::vector<std::string>{
                  "vertex:256", "vertex:512", "vertex:768",
                  "transform", "normalize"},
          "inside projection chooses absolute plane distance and identity");

    runtime.events.clear();
    closest.point = {9.0f, 9.0f, 9.0f};
    Check(&pal4::ConfigCompositeCollision::RecordClosestTrianglePoint(
              query, triangle, 1u, closest, runtime) == &triangle &&
              closest.point == pal4::CollisionPoint3{9.0f, 9.0f, 9.0f} &&
              closest.distance == 3.0f,
          "equal plane distance does not replace existing closest point");
}

void TestTriangleCallbackOutsideUsesThreeEdges() {
    Runtime runtime;
    const auto triangle = MakeTriangle(runtime);
    runtime.magnitude_results = {5.0f, 4.0f, 6.0f};
    pal4::ConfigCollisionClosestPoint closest{
        {9.0f, 9.0f, 9.0f}, 10.0f, 0x5500u};
    const pal4::CollisionPoint3 query{2.0f, 2.0f, 4.0f};
    Check(&pal4::ConfigCompositeCollision::RecordClosestTrianglePoint(
              query, triangle, 0u, closest, runtime) == &triangle &&
              closest.point == pal4::CollisionPoint3{1.0f, 1.0f, 0.0f} &&
              closest.distance == 4.0f &&
              runtime.magnitude_deltas ==
                  std::vector<pal4::CollisionPoint3>{
                      {0.0f, 2.0f, 4.0f},
                      {1.0f, 1.0f, 4.0f},
                      {2.0f, 0.0f, 4.0f}},
          "outside projection checks all three edges against original query");
}

void TestAtomicIntersectionGatesAndQuery() {
    Runtime runtime;
    runtime.memory[0x8004u] = 0x7000u;
    const pal4::ConfigCollisionPosition start{1.0f, 2.0f, 3.0f, 5.0f};
    pal4::CollisionPoint3 movement{2.0f, 3.0f, 4.0f};
    pal4::CollisionPoint3 normal{9.0f, 9.0f, 9.0f};
    float distance = 12.0f;
    Check(!pal4::ConfigCompositeCollision::CheckLineSegmentIntersection(
              nullptr, 0x8000u, movement, &normal, &distance, 1u,
              runtime) &&
              !pal4::ConfigCompositeCollision::CheckLineSegmentIntersection(
                  &start, 0u, movement, &normal, &distance, 1u,
                  runtime) && runtime.events.empty(),
          "null start and atomic bypass traversal");
    Check(!pal4::ConfigCompositeCollision::CheckLineSegmentIntersection(
              &start, 0x8000u, movement, &normal, &distance, 1u,
              runtime) && runtime.last_frame == 0x7000u &&
              runtime.last_intersection_callback == 0x5ffcb0u &&
              runtime.last_query.center == pal4::CollisionPoint3{3.0f, 5.0f, 7.0f} &&
              runtime.last_query.radius == 5.0f &&
              runtime.last_query.unused == std::array<std::uint32_t, 2u>{0u, 0u} &&
              runtime.last_query.type == 3u &&
              movement == pal4::CollisionPoint3{2.0f, 3.0f, 4.0f} &&
              normal == pal4::CollisionPoint3{9.0f, 9.0f, 9.0f} &&
              distance == 12.0f,
          "type-three sphere query and maximum-distance miss leave outputs intact");
}

void TestAtomicIntersectionModeAndResponse() {
    Runtime runtime;
    runtime.memory[0x8004u] = 0x7000u;
    runtime.candidate_enabled = true;
    runtime.candidate_point = {1.0f, 2.0f, 3.0f};
    runtime.candidate_distance = 2.0f;
    runtime.normalized_result = {1.0f, 0.0f, 0.0f};
    const pal4::ConfigCollisionPosition start{1.0f, 2.0f, 3.0f, 5.0f};
    pal4::CollisionPoint3 movement{1.0f, 0.0f, 0.0f};
    pal4::CollisionPoint3 normal{0.0f, 1.0f, 0.0f};
    float distance = 12.0f;
    Check(pal4::ConfigCompositeCollision::CheckLineSegmentIntersection(
              &start, 0x8000u, movement, &normal, &distance, 0u,
              runtime) && runtime.normalize_calls == 0u &&
              movement == pal4::CollisionPoint3{1.0f, 0.0f, 0.0f} &&
              normal == pal4::CollisionPoint3{0.0f, 1.0f, 0.0f} &&
              distance == 12.0f,
          "mode zero reports hit without response or output changes");
    runtime.events.clear();
    Check(pal4::ConfigCompositeCollision::CheckLineSegmentIntersection(
              &start, 0x8000u, movement, &normal, &distance, 1u,
              runtime) && runtime.last_normalize_input ==
                  pal4::CollisionPoint3{1.0f, 0.0f, 0.0f} &&
              movement == pal4::CollisionPoint3{4.0f, 3.0f, 0.0f} &&
              normal == pal4::CollisionPoint3{1.0f, 1.0f, 0.0f} &&
              distance == 3.0f &&
              runtime.events == std::vector<std::string>{
                  "object-transform", "intersections", "normalize"},
          "response adds previous normal and scales by radius minus closest distance");
}

void TestAtomicIntersectionOptionalOutputs() {
    Runtime runtime;
    runtime.memory[0x8004u] = 0x7000u;
    runtime.candidate_enabled = true;
    runtime.candidate_point = {1.0f, 2.0f, 3.0f};
    runtime.candidate_distance = 2.0f;
    runtime.normalized_result = {1.0f, 0.0f, 0.0f};
    const pal4::ConfigCollisionPosition start{1.0f, 2.0f, 3.0f, 5.0f};
    pal4::CollisionPoint3 movement{1.0f, 0.0f, 0.0f};
    Check(pal4::ConfigCompositeCollision::CheckLineSegmentIntersection(
              &start, 0x8000u, movement, nullptr, nullptr, 1u,
              runtime) &&
              movement == pal4::CollisionPoint3{4.0f, 0.0f, 0.0f},
          "response works with absent optional normal and distance");
}

void TestSharedIntersectionReturn() {
    Check(pal4::ConfigCompositeCollision::SharedIntersectionReturn(0u) == 0u &&
              pal4::ConfigCompositeCollision::SharedIntersectionReturn(1u) == 1u &&
              pal4::ConfigCompositeCollision::SharedIntersectionReturn(
                  0xdeadbeefu) == 0xdeadbeefu,
          "shared return tail preserves EAX without side effects");
}

void TestFloorClumpIntersectionGatesAndOutputs() {
    Runtime runtime;
    pal4::ConfigCompositeState state;
    const pal4::CollisionPoint3 point{1.0f, 10.0f, 3.0f};
    pal4::CollisionPoint3 adjusted{7.0f, 7.0f, 7.0f};
    std::uint32_t first = 11u;
    std::uint32_t second = 12u;
    Check(pal4::ConfigCompositeCollision::CheckFloorClumpIntersection(
              state, point, 2.0f, 3.0f, &adjusted, &second, &first,
              runtime) == 0u && runtime.clump_calls == 0u &&
              adjusted == pal4::CollisionPoint3{7.0f, 7.0f, 7.0f},
          "missing floor clump bypasses collision and output writes");

    state.words[13u] = 0x9000u;
    runtime.clump_scratch[0u] = 0xaaaau;
    runtime.clump_scratch[1u] = 0xbbbbu;
    runtime.clump_scratch[4u] = std::bit_cast<std::uint32_t>(8.5f);
    Check(pal4::ConfigCompositeCollision::CheckFloorClumpIntersection(
              state, point, 2.0f, 3.0f, &adjusted, &second, &first,
              runtime) == 0u && runtime.last_clump == 0x9000u &&
              runtime.last_clump_segment.start ==
                  pal4::CollisionPoint3{1.0f, 12.0f, 3.0f} &&
              runtime.last_clump_segment.end ==
                  pal4::CollisionPoint3{1.0f, 7.0f, 3.0f} &&
              adjusted == pal4::CollisionPoint3{7.0f, 7.0f, 7.0f} &&
              first == 11u && second == 12u,
          "miss forwards vertical segment and leaves outputs unchanged");

    runtime.clump_collision_result = 0x1234u;
    Check(pal4::ConfigCompositeCollision::CheckFloorClumpIntersection(
              state, point, 2.0f, 3.0f, nullptr, &second, &first,
              runtime) == 0x1234u && first == 11u && second == 12u,
          "null adjusted point gates both auxiliary output writes");
    Check(pal4::ConfigCompositeCollision::CheckFloorClumpIntersection(
              state, point, 2.0f, 3.0f, &adjusted, &second, &first,
              runtime) == 0x1234u &&
              adjusted == pal4::CollisionPoint3{1.0f, 8.5f, 3.0f} &&
              first == 0xaaaau && second == 0xbbbbu,
          "hit copies source position, replaces Y and forwards two result words");

    adjusted = {7.0f, 7.0f, 7.0f};
    Check(pal4::ConfigCompositeCollision::CheckFloorClumpIntersection(
              state, point, 2.0f, 3.0f, &adjusted, nullptr, nullptr,
              runtime) == 0x1234u &&
              adjusted == pal4::CollisionPoint3{1.0f, 8.5f, 3.0f},
          "optional auxiliary pointers may both be absent");
}

}  // namespace

int main() {
    TestInputGatesAndEmptyList();
    TestFirstHitAdvancesPosition();
    TestSecondHitReturnsWithoutHardBlock();
    TestHardBlockTraversesAndStopsMovement();
    TestUnflaggedMissTraversesWithoutWorldCall();
    TestThirdListInputGates();
    TestThirdListImmediateHardBlock();
    TestThirdListNoIntersections();
    TestThirdListForwardProbeStopsMovement();
    TestThirdListForwardProbeTraverses();
    TestFourthListInputAndFilterGates();
    TestFourthListImmediateHardBlock();
    TestFourthListDirectEnterAndAbort();
    TestFourthListProximityAndLeave();
    TestFourthListWithoutCallbackRetainsState();
    TestTriangleCallbackInsideAndStrictDistance();
    TestTriangleCallbackOutsideUsesThreeEdges();
    TestAtomicIntersectionGatesAndQuery();
    TestAtomicIntersectionModeAndResponse();
    TestAtomicIntersectionOptionalOutputs();
    TestSharedIntersectionReturn();
    TestFloorClumpIntersectionGatesAndOutputs();
}
