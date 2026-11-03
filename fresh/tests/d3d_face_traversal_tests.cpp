#include "pal4/d3d_face_traversal.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct QueueRuntime final : pal4::ID3dFaceTraversalQueueRuntime {
    std::vector<pal4::D3dFaceCandidate> entries;
    std::uint32_t appends = 0u;
    std::uint32_t reads = 0u;

    std::span<pal4::D3dFaceCandidate> Append(
        pal4::D3dFaceTraversalState& state,
        const pal4::D3dFaceCandidate& candidate) override {
        ++appends;
        const auto count = (state.words[3u] - state.words[2u]) / 8u;
        if (count == entries.size()) entries.push_back(candidate);
        else entries[count] = candidate;
        state.words[3u] += 8u;
        return {entries.data(), count + 1u};
    }
    std::span<pal4::D3dFaceCandidate> Entries(
        pal4::D3dFaceTraversalState& state) override {
        ++reads;
        const auto count = (state.words[3u] - state.words[2u]) / 8u;
        return {entries.data(), count};
    }
};

}  // namespace

int main() {
    pal4::D3dFaceTraversalState state{};
    state.words.fill(0xccccccccu);
    state.words[2u] = 0x1000u;
    state.words[3u] = 0x1030u;
    const auto before = state;
    Check(pal4::D3dFaceTraversal::ResetSeed(
              state, 0x55u, {1.25f, -2.5f, 3.75f}) == 0x1030u,
          "seed reset returns previous end pointer");
    Check(state.words[3u] == 0x1000u && state.words[2u] == 0x1000u &&
              state.words[5u] == 0x55u,
          "seed reset empties range and stores generation");
    Check(state.words[6u] == std::bit_cast<std::uint32_t>(1.25f) &&
              state.words[7u] == std::bit_cast<std::uint32_t>(-2.5f) &&
              state.words[8u] == std::bit_cast<std::uint32_t>(3.75f),
          "seed reset copies all three vector words");
    for (const auto index : {0u, 1u, 4u}) {
        Check(state.words[index] == before.words[index],
              "seed reset preserves unrelated state words");
    }
    Check(pal4::D3dFaceTraversal::ResetSeed(
              state, 0x56u, {0.0f, 0.0f, 0.0f}) == 0x1000u &&
              state.words[3u] == 0x1000u,
          "repeated reset returns begin pointer after range was emptied");

    QueueRuntime queue;
    pal4::D3dTriangleFaceState face{};
    auto push = [&](const std::uint32_t address, const float priority) {
        face.words[44u] = std::bit_cast<std::uint32_t>(priority);
        face.words[45u] = std::bit_cast<std::uint32_t>(0.0f);
        pal4::D3dFaceTraversal::PushFace(state, address, face, queue);
    };
    push(0xaaaau, 8.0f);
    push(0xbbbbu, 3.0f);
    push(0xccccu, 5.0f);
    push(0xddddu, 1.0f);
    Check(queue.appends == 4u && state.words[3u] == 0x1020u &&
              queue.entries[0u].face == 0xddddu &&
              queue.entries[0u].priority == 1.0f &&
              queue.entries[1u].face == 0xbbbbu &&
              queue.entries[1u].priority == 3.0f &&
              queue.entries[2u].face == 0xccccu &&
              queue.entries[2u].priority == 5.0f &&
              queue.entries[3u].face == 0xaaaau &&
              queue.entries[3u].priority == 8.0f,
          "face insertion bubbles shorter distances into min heap");
    push(0xeeeeu, 3.0f);
    Check(queue.entries[1u].face == 0xbbbbu &&
              queue.entries[4u].face == 0xeeeeu,
          "equal-priority insertion keeps the existing parent");

    state.words[0u] = 0x123456a5u;
    Check(pal4::D3dFaceTraversal::UpdateFacePriority(
              state, 0xffffu, face, queue) == state.words[3u] &&
              queue.reads == 1u,
          "missing face returns queue end without heap adjustment");
    face.words[44u] = std::bit_cast<std::uint32_t>(0.25f);
    face.words[45u] = std::bit_cast<std::uint32_t>(0.5f);
    Check(pal4::D3dFaceTraversal::UpdateFacePriority(
              state, 0xccccu, face, queue) == 0xccccu &&
              queue.entries[0u].face == 0xccccu &&
              queue.entries[0u].priority == 0.75f &&
              queue.entries[2u].face == 0xddddu &&
              state.words[0u] == 0x123456a5u,
          "existing face updates priority and sifts ahead of the old root");

    pal4::D3dFaceTraversalState range{};
    range.words[3u] = 0x2000u;
    Check(!pal4::D3dFaceTraversal::HasCandidates(range),
          "null queue begin is empty regardless of end");
    range.words[2u] = 0x2000u;
    Check(!pal4::D3dFaceTraversal::HasCandidates(range),
          "equal queue pointers are empty");
    range.words[3u] = 0x2007u;
    Check(!pal4::D3dFaceTraversal::HasCandidates(range),
          "partial record below eight bytes remains empty");
    range.words[3u] = 0x2008u;
    Check(pal4::D3dFaceTraversal::HasCandidates(range),
          "one complete record is nonempty");
    range.words[3u] = 0x1fffu;
    Check(pal4::D3dFaceTraversal::HasCandidates(range),
          "native signed shift treats negative range as nonempty");

    pal4::D3dFaceCandidate popped{};
    Check(pal4::D3dFaceTraversal::PopFace(
              state, popped, queue) == 0x1020u &&
              popped.face == 0xccccu && popped.priority == 0.75f &&
              queue.Entries(state).size() == 4u &&
              queue.entries[0u].face == 0xddddu &&
              queue.entries[4u].face == 0xccccu &&
              state.words[3u] == 0x1020u,
          "pop copies root, repairs heap and retains old root past active end");

    Check(pal4::D3dFaceTraversal::FindFace(
              state, 0xbbbbu, queue) == 0x1008u &&
              pal4::D3dFaceTraversal::FindFace(
                  state, 0xffffu, queue) == 0x1020u,
          "face lookup returns first matching entry or queue end");
    Check(pal4::D3dFaceTraversal::SeedVectorAddress(0x3000u) ==
              0x3018u,
          "seed vector accessor adds native byte offset twenty-four");

    float last_priority = popped.priority;
    while (pal4::D3dFaceTraversal::HasCandidates(state)) {
        (void)pal4::D3dFaceTraversal::PopFace(state, popped, queue);
        Check(popped.priority >= last_priority,
              "repeated pop returns candidates in nondecreasing priority");
        last_priority = popped.priority;
    }
    Check(state.words[3u] == state.words[2u],
          "draining the heap restores the empty end pointer");

    std::array<pal4::D3dFaceCandidate, 4u> tied_children{{
        {0x10u, 0.0f}, {0x20u, 2.0f},
        {0x30u, 2.0f}, {0x40u, 5.0f}}};
    Check(pal4::D3dFaceTraversal::ExtractRoot(
              tied_children, 0xffu) == 0x40u &&
              tied_children[0u].face == 0x30u &&
              tied_children[2u].face == 0x40u &&
              tied_children[3u].face == 0x10u,
          "root extraction selects right child on equal priorities");
    std::array<pal4::D3dFaceCandidate, 3u> single_child{{
        {0x10u, 0.0f}, {0x20u, 2.0f}, {0x30u, 5.0f}}};
    Check(pal4::D3dFaceTraversal::ExtractRoot(
              single_child, 0u) == 0x30u &&
              single_child[0u].face == 0x20u &&
              single_child[1u].face == 0x30u &&
              single_child[2u].face == 0x10u,
          "root extraction handles a lone left child");
    std::array<pal4::D3dFaceCandidate, 1u> lone{{{0x10u, 7.0f}}};
    Check(pal4::D3dFaceTraversal::ExtractRoot(lone, 0u) == 0x10u &&
              lone[0u].face == 0x10u,
          "single-record extraction preserves the sole record");
}
