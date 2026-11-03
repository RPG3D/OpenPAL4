#include "pal4/pal_movement_secondary_collection.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPalMovementSecondaryCollectionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::vector<std::uint32_t> clump_lookups;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::vector<std::uint32_t> left;
    std::uint32_t allocation_result = 0x2000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        writes.emplace_back(address, value);
        words[address] = value;
    }
    std::uint32_t GetClump(std::uint32_t element) override {
        clump_lookups.push_back(element);
        return element / 10u;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        allocations.push_back(bytes);
        return allocation_result;
    }
    void Free(std::uint32_t address) override {
        frees.push_back(address);
    }
    void Leave(std::uint32_t element) override {
        left.push_back(element);
    }
};

void TestDedupAndInPlaceAppend() {
    pal4::PalMovementSecondaryChildState state;
    state.words[28] = 0x1000u;
    state.words[29] = 0x1008u;
    state.words[30] = 0x1010u;
    state.words[31] = 0x55u;
    Runtime runtime;
    runtime.words = {{0x1000u, 11u}, {0x1004u, 21u}};
    Check(pal4::PalMovementSecondaryChild::AddClump(
              state, 19u, runtime) == 0u &&
              runtime.clump_lookups ==
                  std::vector<std::uint32_t>{11u, 19u} &&
              runtime.writes.empty() && runtime.allocations.empty() &&
              state.words[29] == 0x1008u,
          "duplicate clump short-circuits without array changes");
    runtime.clump_lookups.clear();
    Check(pal4::PalMovementSecondaryChild::AddClump(
              state, 31u, runtime) == 1u &&
              runtime.clump_lookups ==
                  std::vector<std::uint32_t>{11u, 31u, 21u, 31u} &&
              runtime.writes ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x1008u, 31u}} &&
              runtime.allocations.empty() &&
              state.words[28] == 0x1000u &&
              state.words[29] == 0x100cu &&
              state.words[30] == 0x1010u &&
              state.words[31] == 0x55u,
          "available capacity appends one pointer and preserves active field");
}

void TestGrowth() {
    pal4::PalMovementSecondaryChildState state;
    state.words[28] = 0x1000u;
    state.words[29] = 0x1008u;
    state.words[30] = 0x1008u;
    state.words[31] = 0x77u;
    Runtime runtime;
    runtime.words = {{0x1000u, 11u}, {0x1004u, 21u}};
    Check(pal4::PalMovementSecondaryChild::AddClump(
              state, 31u, runtime) == 1u &&
              runtime.allocations == std::vector<std::uint32_t>{16u} &&
              runtime.writes ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x2000u, 11u}, {0x2004u, 21u}, {0x2008u, 31u}} &&
              runtime.frees == std::vector<std::uint32_t>{0x1000u} &&
              state.words[28] == 0x2000u &&
              state.words[29] == 0x200cu &&
              state.words[30] == 0x2010u && state.words[31] == 0x77u,
          "full array doubles capacity, copies elements and frees old block");

    pal4::PalMovementSecondaryChildState single;
    single.words[28] = 0x1100u;
    single.words[29] = 0x1104u;
    single.words[30] = 0x1104u;
    Runtime one_runtime;
    one_runtime.words[0x1100u] = 11u;
    Check(pal4::PalMovementSecondaryChild::AddClump(
              single, 21u, one_runtime) == 1u &&
              one_runtime.allocations == std::vector<std::uint32_t>{8u} &&
              single.words[29] == 0x2008u &&
              single.words[30] == 0x2008u,
          "single-element growth allocates two pointer slots");

    pal4::PalMovementSecondaryChildState empty;
    Runtime empty_runtime;
    empty_runtime.allocation_result = 0x3000u;
    Check(pal4::PalMovementSecondaryChild::AddClump(
              empty, 11u, empty_runtime) == 1u &&
              empty_runtime.allocations ==
                  std::vector<std::uint32_t>{4u} &&
              empty_runtime.writes ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x3000u, 11u}} &&
              empty_runtime.frees == std::vector<std::uint32_t>{0u} &&
              empty.words[28] == 0x3000u &&
              empty.words[29] == 0x3004u &&
              empty.words[30] == 0x3004u,
          "empty array allocates one slot and still calls free on old null");

    pal4::PalMovementSecondaryChildState failed;
    Runtime failed_runtime;
    failed_runtime.allocation_result = 0u;
    Check(pal4::PalMovementSecondaryChild::AddClump(
              failed, 11u, failed_runtime) == 1u &&
              failed_runtime.writes.empty() &&
              failed.words[28] == 0u && failed.words[29] == 4u &&
              failed.words[30] == 4u,
          "allocation failure preserves the binary's unchecked state update");
}

void TestRemoval() {
    pal4::PalMovementSecondaryChildState state;
    state.words[28] = 0x1000u;
    state.words[29] = 0x100cu;
    state.words[30] = 0x1010u;
    state.words[31] = 0x99u;
    Runtime runtime;
    runtime.words = {{0x1000u, 11u}, {0x1004u, 21u},
                     {0x1008u, 31u}};
    Check(pal4::PalMovementSecondaryChild::RemoveClump(
              state, 25u, runtime) == 1u &&
              runtime.clump_lookups ==
                  std::vector<std::uint32_t>{11u, 25u, 21u, 25u} &&
              runtime.writes ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x1004u, 31u}} &&
              state.words[29] == 0x1008u &&
              state.words[30] == 0x1010u && state.words[31] == 0u &&
              runtime.words[0x1008u] == 31u,
          "removal shifts later pointers, retains stale tail and clears active");
    runtime.writes.clear();
    state.words[31] = 0x66u;
    Check(pal4::PalMovementSecondaryChild::RemoveClump(
              state, 99u, runtime) == 0u &&
              runtime.writes.empty() &&
              state.words[29] == 0x1008u && state.words[31] == 0x66u,
          "unmatched clump leaves collection and active field unchanged");
    Check(pal4::PalMovementSecondaryChild::RemoveClump(
              state, 35u, runtime) == 1u &&
              state.words[29] == 0x1004u && state.words[31] == 0u,
          "last-element removal has no shift writes");
    state.words[29] = state.words[28];
    runtime.clump_lookups.clear();
    Check(pal4::PalMovementSecondaryChild::RemoveClump(
              state, 11u, runtime) == 0u &&
              runtime.clump_lookups.empty(),
          "empty collection never dereferences target element");
}

void TestLinkableStub() {
    pal4::PalMovementSecondaryChildState state;
    pal4::PalMovementSecondaryCollectionRuntimeStub runtime;
    Check(pal4::PalMovementSecondaryChild::RemoveClump(
              state, 1u, runtime) == 0u &&
              pal4::PalMovementSecondaryChild::AddClump(
                  state, 1u, runtime) == 1u &&
              state.words[29] == 4u,
          "collection stubs link without external allocator or clump API");
    Check(pal4::PalMovementSecondaryChild::GetActiveClump(
              state, runtime) == 0u &&
              pal4::PalMovementSecondaryChild::ClearActiveClump(
                  state, runtime) == 4u && state.words[31] == 0u,
          "active-clump methods also compose with linkable stubs");
}

void TestActiveClumpMethods() {
    pal4::PalMovementSecondaryChildState state;
    state.words[28] = 0x1000u;
    state.words[29] = 0x100cu;
    Runtime runtime;
    runtime.words = {{0x1000u, 21u}, {0x1004u, 31u},
                     {0x1008u, 31u}};
    Check(pal4::PalMovementSecondaryChild::GetActiveClump(
              state, runtime) == 0u &&
              runtime.clump_lookups.empty(),
          "null active element returns zero without a virtual call");
    state.words[31] = 31u;
    Check(pal4::PalMovementSecondaryChild::GetActiveClump(
              state, runtime) == 3u &&
              runtime.clump_lookups ==
                  std::vector<std::uint32_t>{31u},
          "active element forwards its virtual slot eight return");
    Check(pal4::PalMovementSecondaryChild::ClearActiveClump(
              state, runtime) == 0x100cu &&
              runtime.left == std::vector<std::uint32_t>{31u, 31u} &&
              state.words[31] == 0u,
          "every matching pointer receives leave before active is cleared");
    runtime.left.clear();
    state.words[31] = 99u;
    Check(pal4::PalMovementSecondaryChild::ClearActiveClump(
              state, runtime) == 0x100cu &&
              runtime.left.empty() && state.words[31] == 0u,
          "unmatched active element still clears and returns array end");
    state.words[29] = state.words[28];
    state.words[31] = 99u;
    Check(pal4::PalMovementSecondaryChild::ClearActiveClump(
              state, runtime) == 0x1000u &&
              state.words[31] == 0u,
          "empty array clears active element without reading memory");
}

}  // namespace

int main() {
    TestDedupAndInPlaceAppend();
    TestGrowth();
    TestRemoval();
    TestActiveClumpMethods();
    TestLinkableStub();
}
