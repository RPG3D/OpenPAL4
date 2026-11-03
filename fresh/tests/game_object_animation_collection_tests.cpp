#include "pal4/game_object_animation_collection.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::IGameObjectAnimationCollectionRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> slots;
    std::map<std::uint32_t, std::uint32_t> keys{
        {1, 10}, {2, 20}, {3, 10}, {4, 30}, {5, 10}, {6, 40}};
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x2000u;
    std::uint32_t allocated_bytes = 0;

    std::uint32_t ReadPointer(std::uint32_t address) override {
        return slots.at(address);
    }
    void WritePointer(std::uint32_t address,
                      std::uint32_t value) override {
        slots[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        allocated_bytes = bytes;
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
    }
    bool SameObjectKey(std::uint32_t stored,
                       std::uint32_t incoming) override {
        calls.emplace_back("compare");
        return keys.at(stored) == keys.at(incoming);
    }
    bool MatchesKey(std::uint32_t stored,
                    std::uint32_t key) override {
        calls.emplace_back("match");
        return keys.at(stored) == key;
    }
};

void ReplaceWithinCapacity() {
    Runtime runtime;
    pal4::GameObjectEmbeddedAnimationState state;
    state.words[7] = 0x1000u;
    state.words[8] = 0x1010u;
    state.words[9] = 0x1014u;
    runtime.slots = {{0x1000u, 1}, {0x1004u, 2},
                     {0x1008u, 3}, {0x100cu, 4}};
    Check(pal4::GameObjectAnimationCollection::AddOrReplace(
              state, 5, runtime) &&
              state.words[7] == 0x1000u && state.words[8] == 0x100cu &&
              state.words[9] == 0x1014u &&
              runtime.slots.at(0x1000u) == 2 &&
              runtime.slots.at(0x1004u) == 4 &&
              runtime.slots.at(0x1008u) == 5 &&
              runtime.allocated_bytes == 0,
          "replace removes all equivalent entries and appends in place");
    Check(runtime.calls.front() == "compare" &&
              runtime.calls.back() == "write:4104",
          "comparison precedes in-place append");
}

void AllocateAndGrow() {
    Runtime runtime;
    pal4::GameObjectEmbeddedAnimationState empty;
    Check(pal4::GameObjectAnimationCollection::AddOrReplace(
              empty, 6, runtime) &&
              runtime.allocated_bytes == 4 &&
              empty.words[7] == 0x2000u &&
              empty.words[8] == 0x2004u &&
              empty.words[9] == 0x2004u &&
              runtime.slots.at(0x2000u) == 6 &&
              runtime.calls.back() == "free:0",
          "empty collection allocates one pointer and frees null old base");

    runtime.calls.clear();
    runtime.slots[0x1000u] = 2;
    runtime.slots[0x1004u] = 4;
    pal4::GameObjectEmbeddedAnimationState full;
    full.words[7] = 0x1000u;
    full.words[8] = 0x1008u;
    full.words[9] = 0x1008u;
    Check(pal4::GameObjectAnimationCollection::AddOrReplace(
              full, 5, runtime) &&
              runtime.allocated_bytes == 16 &&
              full.words[7] == 0x2000u &&
              full.words[8] == 0x200cu &&
              full.words[9] == 0x2010u &&
              runtime.slots.at(0x2000u) == 2 &&
              runtime.slots.at(0x2004u) == 4 &&
              runtime.slots.at(0x2008u) == 5 &&
              runtime.calls.back() == "free:4096",
          "full two-item collection doubles capacity and copies in order");
}

void RemoveAndAllocationFailure() {
    Runtime runtime;
    pal4::GameObjectEmbeddedAnimationState state;
    state.words[7] = 0x1000u;
    state.words[8] = 0x1010u;
    state.words[9] = 0x1010u;
    runtime.slots = {{0x1000u, 1}, {0x1004u, 2},
                     {0x1008u, 3}, {0x100cu, 4}};
    Check(pal4::GameObjectAnimationCollection::RemoveByKey(
              state, 10, runtime) &&
              state.words[8] == 0x1008u && state.words[9] == 0x1010u &&
              runtime.slots.at(0x1000u) == 2 &&
              runtime.slots.at(0x1004u) == 4 &&
              runtime.allocated_bytes == 0,
          "remove compacts all matching slots without reallocating");
    runtime.calls.clear();
    Check(pal4::GameObjectAnimationCollection::RemoveByKey(
              state, 99, runtime) && state.words[8] == 0x1008u &&
              runtime.calls.size() == 2,
          "missing key still compares each element and returns true");

    state.words[9] = state.words[8];
    runtime.allocation = 0;
    runtime.calls.clear();
    Check(!pal4::GameObjectAnimationCollection::AddOrReplace(
              state, 5, runtime) &&
              state.words[7] == 0x1000u &&
              state.words[8] == 0x1008u &&
              state.words[9] == 0x1008u &&
              runtime.slots.at(0x1000u) == 2 &&
              runtime.slots.at(0x1004u) == 4 &&
              runtime.calls.back() == "allocate",
          "allocation failure preserves the existing pointer triplet");
}

}  // namespace

int main() {
    ReplaceWithinCapacity();
    AllocateAndGrow();
    RemoveAndAllocationFailure();
    std::cout << "game_object_animation_collection_tests passed\n";
}
