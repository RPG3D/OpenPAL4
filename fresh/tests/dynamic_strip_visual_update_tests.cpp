#include "pal4/dynamic_strip_object_update.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void SetFloat(pal4::DynamicStripObjectState& state,
              std::uint32_t offset, float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}

float GetFloat(const pal4::DynamicStripObjectState& state,
               std::uint32_t offset) {
    return std::bit_cast<float>(state.words[offset / 4u]);
}

struct Runtime final : pal4::IDynamicStripVisualUpdateRuntime {
    std::array<float, 3u> first_position{1.0f, 2.0f, 3.0f};
    std::array<float, 3u> second_position{4.0f, 5.0f, 6.0f};
    std::vector<std::uint32_t> nodes;
    std::map<std::uint32_t, pal4::DynamicStripSegment> segments;
    std::uint32_t next_id = 0x701u;
    std::uint32_t frame_reads{};
    std::uint32_t removals{};

    std::array<float, 3u> FramePosition(std::uint32_t frame) override {
        ++frame_reads;
        Check(frame == 0x100u || frame == 0x101u,
              "reads one of the two endpoint frames");
        return frame == 0x100u ? first_position : second_position;
    }
    std::uint32_t FirstNode(std::uint32_t sentinel) override {
        Check(sentinel == 0x700u, "uses object list sentinel");
        return nodes.empty() ? sentinel : nodes.front();
    }
    std::uint32_t NextNode(std::uint32_t node) override {
        const auto it = std::find(nodes.begin(), nodes.end(), node);
        Check(it != nodes.end(), "walks existing list node");
        const auto next = it + 1;
        return next == nodes.end() ? 0x700u : *next;
    }
    float RemainingLife(std::uint32_t node) override {
        return segments.at(node).remaining;
    }
    void SetRemainingLife(std::uint32_t node, float value) override {
        segments.at(node).remaining = value;
    }
    void RemoveNode(std::uint32_t node) override {
        ++removals;
        nodes.erase(std::find(nodes.begin(), nodes.end(), node));
        segments.erase(node);
    }
    void InsertFront(std::uint32_t sentinel,
                     const pal4::DynamicStripSegment& segment) override {
        Check(sentinel == 0x700u, "inserts at native list head");
        nodes.insert(nodes.begin(), next_id);
        segments.emplace(next_id++, segment);
    }
};

pal4::DynamicStripObjectState MakeState() {
    pal4::DynamicStripObjectState state;
    state.words[0u] = 1u;
    state.words[25u] = 0x100u;
    state.words[26u] = 0x101u;
    state.words[28u] = 0x700u;
    SetFloat(state, 60u, 2.0f);
    return state;
}

void InactiveAndInitial() {
    auto state = MakeState();
    Runtime runtime;
    state.words[0u] = 0u;
    Check(pal4::DynamicStripObjectUpdater::UpdateVisual(
              state, 0.25, 0u, runtime) == 0u &&
              runtime.frame_reads == 0u && runtime.nodes.empty(),
          "inactive object does not read frames or list");

    state.words[0u] = 0x00010001u;
    Check(pal4::DynamicStripObjectUpdater::UpdateVisual(
              state, 0.25, 1u, runtime) == 0u,
          "first update with skip mode avoids second insertion");
    Check(state.words[0u] == 1u && state.words[29u] == 1u &&
              runtime.nodes.size() == 1u && runtime.frame_reads == 2u,
          "first update clears initial flag and inserts segment");
    const auto& segment = runtime.segments.at(runtime.nodes.front());
    Check(segment.first == runtime.first_position &&
              segment.second == runtime.second_position &&
              segment.remaining == 1.75f,
          "first segment is aged on insertion frame");
    Check(GetFloat(state, 76u) == 1.0f &&
              GetFloat(state, 96u) == 6.0f,
          "initial positions are cached at both object offsets");

    Check(pal4::DynamicStripObjectUpdater::UpdateVisual(
              state, 1.75, 1u, runtime) == 0u &&
              state.words[29u] == 0u && runtime.nodes.empty() &&
              runtime.removals == 1u && runtime.frame_reads == 2u,
          "expired segment unlinks at exact lifetime and skip mode reads no frames");
}

void IntervalAndMovement() {
    auto state = MakeState();
    Runtime runtime;
    SetFloat(state, 8u, 0.5f);
    for (std::uint32_t i = 0u; i < 3u; ++i) {
        SetFloat(state, 76u + i * 4u, runtime.first_position[i]);
        SetFloat(state, 88u + i * 4u, runtime.second_position[i]);
    }
    runtime.first_position[0] += 0.1f;
    Check(pal4::DynamicStripObjectUpdater::UpdateVisual(
              state, 0.2, 0u, runtime) == 0u &&
              runtime.frame_reads == 0u && runtime.nodes.empty() &&
              std::abs(GetFloat(state, 12u) - 0.2f) < 0.000001f,
          "interval gate accumulates time before reading frames");
    Check(pal4::DynamicStripObjectUpdater::UpdateVisual(
              state, 0.3, 0u, runtime) == 1u &&
              state.words[29u] == 1u && runtime.nodes.size() == 1u &&
              runtime.frame_reads == 2u && GetFloat(state, 12u) == 0.0f &&
              GetFloat(state, 76u) == runtime.first_position[0],
          "threshold crossing inserts current segment and resets timer");

    runtime.first_position[0] += 0.1f;
    Check(pal4::DynamicStripObjectUpdater::UpdateVisual(
              state, 0.5, 1u, runtime) == 0u &&
              state.words[29u] == 1u && runtime.frame_reads == 2u &&
              GetFloat(state, 12u) == 0.0f,
          "mode byte suppresses insertion after interval reset");
}

void MovementTolerance() {
    auto state = MakeState();
    Runtime runtime;
    for (std::uint32_t i = 0u; i < 3u; ++i) {
        SetFloat(state, 76u + i * 4u, runtime.first_position[i]);
        SetFloat(state, 88u + i * 4u, runtime.second_position[i]);
    }
    runtime.second_position[2] += 0.00005f;
    Check(pal4::DynamicStripObjectUpdater::UpdateVisual(
              state, 0.0, 0u, runtime) == 0u &&
              runtime.nodes.empty(),
          "sub-threshold movement does not add segment");
    runtime.second_position[2] += 0.0002f;
    Check(pal4::DynamicStripObjectUpdater::UpdateVisual(
              state, 0.0, 0u, runtime) == 1u &&
              runtime.nodes.size() == 1u &&
              GetFloat(state, 96u) == runtime.second_position[2],
          "second endpoint movement adds segment and refreshes cache");
}

void UnroundedLifeAndInterval() {
    auto state = MakeState();
    Runtime runtime;
    runtime.InsertFront(0x700u, {{}, {}, 1.0e-38f});
    state.words[29u] = 1u;
    const double stored = static_cast<double>(1.0e-38f);
    const double nearly_expired = stored - 1.0e-46;
    Check(pal4::DynamicStripObjectUpdater::UpdateVisual(
              state, nearly_expired, 1u, runtime) == 0u &&
              runtime.nodes.size() == 1u && runtime.removals == 0u &&
              runtime.segments.at(runtime.nodes.front()).remaining == 0.0f,
          "unrounded positive life keeps node even when stored float is zero");
    Check(pal4::DynamicStripObjectUpdater::UpdateVisual(
              state, 0.0, 1u, runtime) == 0u &&
              runtime.nodes.empty() && runtime.removals == 1u,
          "next frame removes stored zero-life node");

    SetFloat(state, 8u, 0.5f);
    SetFloat(state, 12u, 0.25f);
    Check(pal4::DynamicStripObjectUpdater::UpdateVisual(
              state, 0.25, 1u, runtime) == 0u &&
              GetFloat(state, 12u) == 0.0f,
          "exact interval boundary resets accumulator");
}

void EmbeddedBlockBoundary() {
    struct Guarded {
        std::uint32_t before = 0x12345678u;
        std::array<std::uint32_t, 34u> words{};
        std::uint32_t after = 0x87654321u;
    } state;
    const auto source = MakeState();
    std::copy_n(source.words.begin(), state.words.size(), state.words.begin());
    state.words[0u] = 0x00010001u;
    Runtime runtime;
    const pal4::StripVisualBlockView view{state.words};
    (void)pal4::DynamicStripObjectUpdater::UpdateVisual(view, 0.25, 0u, runtime);
    Check(state.words[29u] == 1u && runtime.segments.size() == 1u,
          "136-byte block captures initial segment");
    runtime.first_position[0u] += 1.0f;
    (void)pal4::DynamicStripObjectUpdater::UpdateVisual(view, 0.25, 0u, runtime);
    Check(state.words[29u] == 2u, "136-byte block captures moving blade");
    (void)pal4::DynamicStripObjectUpdater::UpdateVisual(view, 3.0, 1u, runtime);
    Check(state.words[29u] == 0u && runtime.nodes.empty(),
          "136-byte block drains without sampling new endpoints");
    Check(state.before == 0x12345678u && state.after == 0x87654321u,
          "embedded update preserves adjacent blocks");
}

}  // namespace

int main() {
    InactiveAndInitial();
    IntervalAndMovement();
    MovementTolerance();
    UnroundedLifeAndInterval();
    EmbeddedBlockBoundary();
    std::cout << "dynamic strip visual update tests passed\n";
}
