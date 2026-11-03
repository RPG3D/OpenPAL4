#include "pal4/memory_heap_extend.h"

#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kHead = 0x8dd840u;
constexpr std::uint32_t kFailure = 0xffffffffu;

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Allocation {
    std::uint32_t address;
    std::uint32_t size;
    std::uint32_t kind;
    std::uint32_t protection;
};

struct Release {
    std::uint32_t address;
    std::uint32_t size;
    std::uint32_t kind;
};

class Memory final : public pal4::IMemoryHeapExtendRuntime {
public:
    [[nodiscard]] std::uint32_t Read(const std::uint32_t address) const override {
        const auto found = words.find(address);
        return found == words.end() ? 0u : found->second;
    }

    void Write(const std::uint32_t address,
               const std::uint32_t value) override {
        words[address] = value;
    }

    [[nodiscard]] std::uint32_t PageSize() override {
        ++page_calls;
        return 0x1000u;
    }

    [[nodiscard]] std::uint32_t AllocationGranularity() override {
        ++granularity_calls;
        return 0x1000u;
    }

    [[nodiscard]] bool InitializeStructure(
        const std::uint32_t base,
        const std::uint32_t reserve_size) override {
        ++initialize_calls;
        if (!initialize_succeeds) {
            return false;
        }
        const std::uint32_t node = 0x900000u +
                                   static_cast<std::uint32_t>(initialize_calls) *
                                       0x20u;
        Write(node, base);
        Write(node + 4u, base);
        Write(node + 8u, base + reserve_size);
        Write(node + 12u, reserve_size);
        Write(node + 16u, Read(kHead));
        Write(kHead, node);
        return true;
    }

    [[nodiscard]] bool FreeStructure() override {
        ++free_structure_calls;
        if (!free_structure_succeeds) {
            return false;
        }
        Write(kHead, Read(Read(kHead) + 16u));
        return true;
    }

    [[nodiscard]] std::uint32_t VirtualAllocate(
        const std::uint32_t address, const std::uint32_t size,
        const std::uint32_t kind,
        const std::uint32_t protection) override {
        allocations.push_back({address, size, kind, protection});
        if (allocation_results.empty()) {
            return address;
        }
        const std::uint32_t result = allocation_results.front();
        allocation_results.pop_front();
        return result;
    }

    [[nodiscard]] bool VirtualRelease(const std::uint32_t address,
                                      const std::uint32_t size,
                                      const std::uint32_t kind) override {
        releases.push_back({address, size, kind});
        if (release_results.empty()) {
            return true;
        }
        const bool result = release_results.front();
        release_results.pop_front();
        return result;
    }

    [[nodiscard]] bool VirtualQuery(
        const std::uint32_t address,
        pal4::VirtualMemoryRegion& region) override {
        queries.push_back(address);
        const auto found = regions.find(address);
        if (found == regions.end()) {
            return false;
        }
        region = found->second;
        return true;
    }

    [[nodiscard]] std::uint32_t LastError() override {
        return last_error;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, pal4::VirtualMemoryRegion> regions;
    std::deque<std::uint32_t> allocation_results;
    std::deque<bool> release_results;
    std::vector<Allocation> allocations;
    std::vector<Release> releases;
    std::vector<std::uint32_t> queries;
    std::uint32_t last_error = 0u;
    int page_calls = 0;
    int granularity_calls = 0;
    int initialize_calls = 0;
    int free_structure_calls = 0;
    bool initialize_succeeds = true;
    bool free_structure_succeeds = true;
};

void SeedCaches(Memory& memory) {
    memory.Write(0x8dd81cu, 0x1000u);
    memory.Write(0x8dd820u, 0x1000u);
    memory.Write(0x8dd498u, 0x1000u);
    memory.Write(0x8dd824u, 0x1000u);
}

void SeedNode(Memory& memory, const std::uint32_t node,
              const std::uint32_t current,
              const std::uint32_t committed,
              const std::uint32_t base,
              const std::uint32_t reserved_end,
              const std::uint32_t previous = 0u) {
    memory.Write(kHead, node);
    memory.Write(node, current);
    memory.Write(node + 4u, committed);
    memory.Write(node + 8u, reserved_end);
    memory.Write(node + 12u, reserved_end - base);
    memory.Write(node + 16u, previous);
}

void CheckInitializationAndWithinCommitted() {
    Memory cold;
    Check(pal4::MemoryHeapExtend::Change(0u, cold) == 0u,
          "zero request initializes descriptor and returns current break");
    Check(cold.page_calls == 1 && cold.granularity_calls == 1 &&
              cold.initialize_calls == 1,
          "cold path initializes both caches and descriptor");
    Check(cold.Read(0x8dd820u) == 0x1000u &&
              cold.Read(0x8dd824u) == 0x1000u,
          "alignment copies are cached");

    Memory warm;
    SeedCaches(warm);
    SeedNode(warm, 0x1000u, 0x11000u, 0x12000u, 0x10000u, 0x20000u);
    Check(pal4::MemoryHeapExtend::Change(0x800u, warm) == 0x11000u,
          "growth within committed pages returns old break");
    Check(warm.Read(0x1000u) == 0x11800u && warm.allocations.empty(),
          "growth within committed pages makes no platform allocation");
}

void CheckCommitInsideReservation() {
    Memory memory;
    SeedCaches(memory);
    SeedNode(memory, 0x1000u, 0x11000u, 0x12000u, 0x10000u, 0x20000u);
    Check(pal4::MemoryHeapExtend::Change(0x2000u, memory) == 0x11000u,
          "growth commits available reserved pages");
    Check(memory.allocations.size() == 1u &&
              memory.allocations[0].address == 0x12000u &&
              memory.allocations[0].size == 0x1000u &&
              memory.allocations[0].kind == 0x1000u,
          "only one additional page is committed");
    Check(memory.Read(0x1000u) == 0x13000u &&
              memory.Read(0x1004u) == 0x13000u,
          "current and committed ends advance independently");
}

void CheckContiguousReservation() {
    Memory memory;
    SeedCaches(memory);
    SeedNode(memory, 0x1000u, 0x11000u, 0x11000u, 0x10000u, 0x11000u);
    memory.regions[0x11000u] = {0x11000u, 0x10000u, 0x10000u};
    Check(pal4::MemoryHeapExtend::Change(0x2000u, memory) == 0x11000u,
          "contiguous free region extends heap break");
    Check(memory.allocations.size() == 2u &&
              memory.allocations[0].kind == 0x2000u &&
              memory.allocations[1].kind == 0x1000u,
          "reserve is followed by page commitment");
    const std::uint32_t head = memory.Read(kHead);
    Check(memory.initialize_calls == 1 && memory.Read(head) == 0x13000u &&
              memory.Read(head + 4u) == 0x13000u &&
              memory.Read(head + 16u) == 0x1000u,
          "new descriptor links to prior region and records new end");
}

void CheckNoncontiguousReservation() {
    Memory memory;
    SeedCaches(memory);
    SeedNode(memory, 0x1000u, 0x11000u, 0x11000u, 0x10000u, 0x11000u);
    memory.regions[0x11000u] = {0x11000u, 0xf000u, 0x1000u};
    memory.regions[0x20000u] = {0x20000u, 0x10000u, 0x10000u};
    Check(pal4::MemoryHeapExtend::Change(0x2000u, memory) == 0x20000u,
          "noncontiguous free region returns its own start");
    Check(memory.queries.size() == 2u &&
              memory.allocations[0].address == 0x20000u &&
              memory.Read(memory.Read(kHead)) == 0x22000u,
          "query skips occupied region before reserving new one");
}

void CheckShrinkAndRegionRelease() {
    Memory memory;
    SeedCaches(memory);
    SeedNode(memory, 0x1000u, 0x14000u, 0x15000u, 0x10000u, 0x20000u);
    Check(pal4::MemoryHeapExtend::Change(0xfffff000u, memory) == 0x13000u,
          "negative request decommits full pages");
    Check(memory.releases.size() == 1u &&
              memory.releases[0].address == 0x13000u &&
              memory.releases[0].size == 0x2000u &&
              memory.releases[0].kind == 0x4000u,
          "decommit starts at new committed end");
    Check(memory.Read(0x1000u) == 0x13000u &&
              memory.Read(0x1004u) == 0x13000u,
          "negative request updates current and committed ends");

    Memory chain;
    SeedCaches(chain);
    SeedNode(chain, 0x1000u, 0x18000u, 0x19000u, 0x10000u, 0x20000u);
    SeedNode(chain, 0x2000u, 0x21000u, 0x22000u, 0x20000u, 0x30000u,
             0x1000u);
    Check(pal4::MemoryHeapExtend::Change(0xffffd000u, chain) == 0x16000u,
          "shrink releases exhausted head region then decommits prior one");
    Check(chain.free_structure_calls == 1 && chain.Read(kHead) == 0x1000u &&
              chain.releases.size() == 2u &&
              chain.releases[0].kind == 0x8000u &&
              chain.releases[1].kind == 0x4000u,
          "region release precedes descriptor removal and decommit");
}

void CheckFailurePaths() {
    Memory init_failure;
    init_failure.initialize_succeeds = false;
    Check(pal4::MemoryHeapExtend::Change(0u, init_failure) == kFailure,
          "descriptor allocation failure returns minus one");

    Memory commit_failure;
    SeedCaches(commit_failure);
    SeedNode(commit_failure, 0x1000u, 0x11000u, 0x12000u, 0x10000u,
             0x20000u);
    commit_failure.allocation_results.push_back(0u);
    Check(pal4::MemoryHeapExtend::Change(0x2000u, commit_failure) ==
              kFailure,
          "commit at unexpected address fails");
    Check(commit_failure.Read(0x1000u) == 0x11000u,
          "failed commit leaves current pointer intact");

    Memory release_failure;
    SeedCaches(release_failure);
    SeedNode(release_failure, 0x1000u, 0x14000u, 0x15000u, 0x10000u,
             0x20000u);
    release_failure.release_results.push_back(false);
    Check(pal4::MemoryHeapExtend::Change(0xfffff000u, release_failure) ==
              kFailure,
          "failed decommit returns minus one");
    Check(release_failure.Read(0x1000u) == 0x14000u,
          "failed decommit preserves current pointer");
}

}  // namespace

int main() {
    CheckInitializationAndWithinCommitted();
    CheckCommitInsideReservation();
    CheckContiguousReservation();
    CheckNoncontiguousReservation();
    CheckShrinkAndRegionRelease();
    CheckFailurePaths();
    std::cout << "6 memory heap extend test cases passed\n";
}
