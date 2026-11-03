#include "pal4/grid_manager_members.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <stdexcept>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::IGridManagerMembersRuntime {
public:
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        allocations.push_back(bytes);
        if (fail_first_allocation && allocations.size() == 1u)
            return 0u;
        const auto address = next_address;
        next_address += 0x10000u;
        return address;
    }
    std::uint32_t Free(const std::uint32_t address) override {
        frees.push_back(address);
        return address + 1u;
    }
    pal4::SpatialGridState& GridAt(const std::uint32_t address) override {
        Check(address == 0x20000u, "spatial grid header address");
        return grid;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words.at(address);
    }
    void DestroyGridWrapper(const std::uint32_t address) override {
        Check(address == 0x10000u, "wrapper address");
        events.push_back("destroy_wrapper");
    }
    void WriteGridWrapper(const std::uint32_t wrapper_address,
                          const std::uint32_t grid_address) override {
        Check(wrapper_address == 0x10000u && grid_address == 0x20000u,
              "wrapper stores created spatial grid");
        words[wrapper_address + 4u] = grid_address;
    }
    void ReleaseElement(const std::uint32_t element) override {
        released.push_back(element);
    }
    void UpdateElement(const std::uint32_t element,
                       pal4::GridManagerMembersState& state,
                       const std::uint32_t argument) override {
        Check(&state == active_state && argument == 0x55u,
              "update forwards manager member and argument");
        updated.push_back(element);
    }
    bool fail_first_allocation = false;
    std::uint32_t next_address = 0x10000u;
    pal4::SpatialGridState grid{};
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::vector<std::string> events;
    pal4::GridManagerMembersState* active_state = nullptr;
    std::vector<std::uint32_t> released;
    std::vector<std::uint32_t> updated;
};

void InitializeAndGrid() {
    Runtime runtime;
    pal4::GridManagerMembersState state{};
    state.first_flag = 4u;
    state.elements_begin = 9u;
    state.secondary_begin = 8u;
    state.grid_wrapper = 7u;
    Check(&pal4::GridManagerMembers::Initialize(state) == &state &&
              state.first_flag == 0u && state.second_flag == 0u &&
              state.elements_begin == 0u && state.secondary_begin == 0u &&
              state.grid_wrapper == 0u && state.grid_ready == 0u &&
              state.update_disabled == 0u,
          "member initializer clears both vectors and grid state");
    Check(pal4::GridManagerMembers::InitializeGrid(state, runtime) ==
              0x20000u &&
              runtime.allocations ==
                  std::vector<std::uint32_t>{8u, 44u, 1600u} &&
              runtime.words[0x10000u] == 0x84b6b4u &&
              runtime.words[0x10004u] == 0x20000u &&
              state.grid_wrapper == 0x10000u && state.grid_ready == 1u &&
              state.update_disabled == 0u &&
              runtime.grid.dimensions ==
                  std::array<std::uint32_t, 3u>{20u, 1u, 20u} &&
              runtime.grid.origin_bits[0] ==
                  std::bit_cast<std::uint32_t>(-40.0f),
          "grid initializer preserves assembly X20/Y1/Z20 dimensions and 400 bucket heads");
    pal4::GridManagerMembers::ResetGrid(state, runtime);
    Check(runtime.events == std::vector<std::string>{"destroy_wrapper"} &&
              state.grid_wrapper == 0u && state.grid_ready == 0u &&
              state.update_disabled == 1u,
          "reset dispatches wrapper destructor and marks update disabled");
}

void CleanupAndAllocationFailure() {
    Runtime runtime;
    pal4::GridManagerMembersState state{};
    (void)pal4::GridManagerMembers::Initialize(state);
    state.secondary_begin = 0x40000u;
    state.secondary_end = 0x40004u;
    state.elements_begin = 0x50000u;
    state.elements_end = 0x50004u;
    Check(pal4::GridManagerMembers::Cleanup(state, runtime) == 0x50001u &&
              runtime.frees ==
                  std::vector<std::uint32_t>{0x40000u, 0x50000u} &&
              state.secondary_begin == 0u && state.elements_begin == 0u &&
              state.update_disabled == 1u,
          "cleanup resets grid then frees secondary and primary vectors");
    Runtime failed;
    failed.fail_first_allocation = true;
    (void)pal4::GridManagerMembers::Initialize(state);
    Check(pal4::GridManagerMembers::InitializeGrid(state, failed) == 0u &&
              state.grid_wrapper == 0u && state.grid_ready == 1u &&
              state.update_disabled == 0u,
          "wrapper allocation failure retains native flags without dereference");
}

void ElementOperations() {
    Runtime runtime;
    pal4::GridManagerMembersState state{};
    (void)pal4::GridManagerMembers::Initialize(state);
    Check(!pal4::GridManagerMembers::AddElement(state, 11u, runtime) &&
              !pal4::GridManagerMembers::AddElement(state, 0u, runtime),
          "element insertion requires wrapper and nonzero element");
    state.grid_wrapper = 0x9000u;
    Check(pal4::GridManagerMembers::AddElement(state, 11u, runtime) &&
              pal4::GridManagerMembers::AddElement(state, 22u, runtime) &&
              pal4::GridManagerMembers::AddElement(state, 33u, runtime) &&
              runtime.allocations ==
                  std::vector<std::uint32_t>{4u, 8u, 16u} &&
              runtime.frees ==
                  std::vector<std::uint32_t>{0x10000u, 0x20000u} &&
              state.elements_begin == 0x30000u &&
              state.elements_end == 0x3000cu &&
              state.elements_capacity == 0x30010u &&
              runtime.words[0x30000u] == 11u &&
              runtime.words[0x30004u] == 22u &&
              runtime.words[0x30008u] == 33u,
          "element vector grows 1, 2, 4 and copies order");
    Check(!pal4::GridManagerMembers::RemoveElement(state, 44u, runtime) &&
              pal4::GridManagerMembers::RemoveElement(state, 22u, runtime) &&
              state.elements_end == 0x30008u &&
              runtime.words[0x30004u] == 33u &&
              runtime.released == std::vector<std::uint32_t>{22u},
          "removal shifts tail and invokes element release");
    runtime.active_state = &state;
    pal4::GridManagerMembers::DisableUpdate(state);
    Check(pal4::GridManagerMembers::UpdateElements(state, 0x55u,
                                                    runtime) == 8u &&
              runtime.updated == std::vector<std::uint32_t>{11u, 33u},
          "enabled update visits all live elements in order");
    pal4::GridManagerMembers::EnableUpdate(state);
    Check(pal4::GridManagerMembers::UpdateElements(state, 0x55u,
                                                    runtime) == 1u &&
              runtime.updated.size() == 2u,
          "update flag suppresses traversal");
    Check(pal4::GridManagerMembers::ClearElements(state, runtime) ==
              0x30008u && state.elements_end == state.elements_begin &&
              runtime.released ==
                  std::vector<std::uint32_t>{22u, 11u, 33u},
          "clear returns prior end and releases remaining elements");
    Check(pal4::GridManagerMembers::AddElement(state, 44u, runtime) &&
              runtime.allocations.size() == 3u &&
              runtime.words[0x30000u] == 44u,
          "cleared vector reuses capacity");
    state.secondary_begin = 0x7000u;
    state.secondary_end = 0x7010u;
    Check(pal4::GridManagerMembers::PreviousElement(state) == 0x7010u &&
              state.secondary_end == 0x7000u,
          "previous-element entry returns end then resets it to begin");
}

void SecondaryInputSnapshot() {
    Runtime runtime;
    pal4::GridManagerMembersState state{};
    Check(pal4::GridManagerMembers::SecondaryElements(state, runtime).empty(),
          "initialized secondary vector has no records or memory reads");
    state.secondary_begin = 0x7000u;
    state.secondary_end = 0x700cu;
    state.secondary_capacity = 0x7010u;
    runtime.words[0x7000u] = 99u;
    runtime.words[0x7004u] = 11u;
    runtime.words[0x7008u] = 99u;
    const auto snapshot = pal4::GridManagerMembers::SecondaryElements(state, runtime);
    Check(snapshot == std::vector<std::uint32_t>{99u,11u,99u},
          "secondary snapshot preserves order and duplicate record identity");
    (void)pal4::GridManagerMembers::PreviousElement(state);
    Check(pal4::GridManagerMembers::SecondaryElements(state, runtime).empty() &&
              snapshot.size() == 3u && state.secondary_capacity == 0x7010u,
          "reset changes live range while owned token copy survives");
    const auto reject = [&](std::uint32_t begin, std::uint32_t end,
                            std::uint32_t capacity) {
        state.secondary_begin=begin;state.secondary_end=end;
        state.secondary_capacity=capacity;
        bool failed=false;
        try { (void)pal4::GridManagerMembers::SecondaryElements(state,runtime); }
        catch(const std::logic_error&) { failed=true; }
        Check(failed,"corrupt secondary vector must fail, never become empty");
    };
    reject(0u,0u,4u);
    reject(0x7000u,0x6ffcu,0x7010u);
    reject(0x7000u,0x7014u,0x7010u);
    reject(0x7001u,0x7001u,0x7001u);
    reject(0x7000u,0x7001u,0x7010u);
    reject(0x7000u,0x7000u,0x7001u);
    state.secondary_begin=0x8000u;state.secondary_end=0x8004u;
    state.secondary_capacity=0x8004u;
    bool failed=false;
    try { (void)pal4::GridManagerMembers::SecondaryElements(state,runtime); }
    catch(const std::out_of_range&) { failed=true; }
    Check(failed,"missing mapped record storage propagates resolver failure");
}

}  // namespace

int main() {
    InitializeAndGrid();
    CleanupAndAllocationFailure();
    ElementOperations();
    SecondaryInputSnapshot();
    std::cout << "Grid manager members tests passed\n";
}
