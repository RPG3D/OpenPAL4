#include "pal4/heap_reallocator.h"

#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kHeap = pal4::HeapAllocator::kHeapAddress;
constexpr std::uint32_t kAllocated = kHeap + 0x394u;

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Memory final : public pal4::IHeapReallocRuntime {
public:
    [[nodiscard]] std::uint32_t Read(const std::uint32_t address) const override {
        std::uint32_t value = 0u;
        for (std::uint32_t index = 0; index < 4u; ++index) {
            const auto found = bytes.find(address + index);
            if (found != bytes.end()) {
                value |= std::uint32_t{found->second} << (index * 8u);
            }
        }
        return value;
    }

    void Write(const std::uint32_t address,
               const std::uint32_t value) override {
        for (std::uint32_t index = 0; index < 4u; ++index) {
            bytes[address + index] =
                static_cast<std::uint8_t>(value >> (index * 8u));
        }
    }

    void ManageBlocks(std::uint32_t) override {
        Check(false, "reallocator must not consolidate directly");
    }

    [[nodiscard]] std::uint32_t Extend(std::uint32_t,
                                       std::uint32_t) override {
        Check(false, "reallocator must not extend directly");
        return 0u;
    }

    [[nodiscard]] std::uint32_t Allocate(
        const std::uint32_t request) override {
        allocated_requests.push_back(request);
        if (allocations.empty()) {
            return 0u;
        }
        const std::uint32_t result = allocations.front();
        allocations.pop_front();
        if (account_on_allocate && result != 0u) {
            Write(kAllocated,
                  Read(kAllocated) +
                      pal4::HeapAllocator::PayloadSize(result, *this));
        }
        return result;
    }

    void Free(const std::uint32_t payload) override {
        freed.push_back(payload);
        Write(kAllocated,
              Read(kAllocated) -
                  pal4::HeapAllocator::PayloadSize(payload, *this));
    }

    void Copy(const std::uint32_t destination,
              const std::uint32_t source,
              const std::uint32_t size) override {
        copied_size = size;
        for (std::uint32_t index = 0; index < size; ++index) {
            bytes[destination + index] = bytes[source + index];
        }
    }

    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::deque<std::uint32_t> allocations;
    std::vector<std::uint32_t> allocated_requests;
    std::vector<std::uint32_t> freed;
    std::uint32_t copied_size = 0u;
    bool account_on_allocate = false;
};

void SeedBlock(Memory& memory, const std::uint32_t payload,
               const std::uint32_t size, const std::uint32_t flags = 1u) {
    memory.Write(payload - 4u, size | flags);
    memory.Write(payload + size - 4u, 1u);
}

void CheckNullAndOversize() {
    Memory memory;
    memory.allocations.push_back(0x1008u);
    Check(pal4::HeapReallocator::ReallocateWrapper(
              0u, 37u, 0xdeadbeefu, memory) ==
              0x1008u,
          "null pointer delegates exact request to allocator");
    Check(memory.allocated_requests.size() == 1u &&
              memory.allocated_requests[0] == 37u,
          "null path does not normalize before allocator call");
    SeedBlock(memory, 0x2008u, 32u);
    Check(pal4::HeapReallocator::Reallocate(0x2008u, 0xffffffe0u, memory) == 0u,
          "oversize request is rejected");
    Check(memory.freed.empty() && memory.Read(0x2004u) == 33u,
          "oversize rejection keeps original block");
}

void CheckMappedBlock() {
    Memory keep;
    SeedBlock(keep, 0x3008u, 64u, 2u);
    Check(pal4::HeapReallocator::Reallocate(0x3008u, 40u, keep) == 0x3008u,
          "mapped block with enough space stays in place");
    Check(keep.allocated_requests.empty() && keep.freed.empty(),
          "mapped keep path has no allocation or free");

    Memory move;
    SeedBlock(move, 0x4008u, 64u, 2u);
    move.allocations.push_back(0x5008u);
    for (std::uint32_t index = 0; index < 56u; ++index) {
        move.bytes[0x4008u + index] = static_cast<std::uint8_t>(index + 1u);
    }
    Check(pal4::HeapReallocator::Reallocate(0x4008u, 100u, move) == 0x5008u,
          "mapped growth allocates replacement");
    Check(move.copied_size == 56u && move.freed.size() == 1u &&
              move.freed[0] == 0x4008u,
          "mapped growth copies size minus eight then frees old block");
    Check(move.bytes[0x5008u] == 1u && move.bytes[0x503fu] == 56u,
          "mapped payload content is copied");

    Memory failure;
    SeedBlock(failure, 0x5808u, 64u, 2u);
    Check(pal4::HeapReallocator::Reallocate(0x5808u, 100u, failure) == 0u,
          "failed mapped growth returns null");
    Check(failure.freed.empty() && failure.Read(0x5804u) == 66u,
          "failed mapped growth keeps original mapping");
}

void CheckShrinkWithoutAndWithSplit() {
    Memory no_split;
    SeedBlock(no_split, 0x6008u, 32u);
    no_split.Write(kAllocated, 100u);
    Check(pal4::HeapReallocator::Reallocate(0x6008u, 17u, no_split) ==
              0x6008u,
          "small shrink stays in place");
    Check(no_split.Read(0x6004u) == 33u && no_split.freed.empty(),
          "remainder below sixteen is not split");
    Check(no_split.Read(kAllocated) == 128u,
          "in-place return adds payload size again as observed");

    Memory split;
    SeedBlock(split, 0x7008u, 64u);
    split.Write(kAllocated, 100u);
    Check(pal4::HeapReallocator::Reallocate(0x7008u, 17u, split) == 0x7008u,
          "large shrink keeps original payload");
    Check(split.Read(0x7004u) == 25u && split.Read(0x701cu) == 41u &&
              split.freed.size() == 1u && split.freed[0] == 0x7020u,
          "large shrink frees a forty-byte remainder");
    Check(split.Read(kAllocated) == 84u,
          "remainder free and final accounting both execute");
}

void CheckGrowIntoTopAndNeighbor() {
    Memory top;
    SeedBlock(top, 0x8008u, 32u);
    top.Write(kHeap + 0x2cu, 0x8020u);
    top.Write(0x8024u, 0x41u);
    Check(pal4::HeapReallocator::Reallocate(0x8008u, 33u, top) == 0x8008u,
          "growth consumes space from top");
    Check(top.Read(kHeap + 0x2cu) == 0x8028u &&
              top.Read(0x802cu) == 0x39u,
          "top pointer and remainder header advance");

    Memory adjacent;
    SeedBlock(adjacent, 0x9008u, 32u);
    const std::uint32_t next = 0x9020u;
    adjacent.Write(next + 4u, 0x21u);
    adjacent.Write(next + 32u + 4u, 0u);
    const std::uint32_t sentinel = 0xa000u;
    adjacent.Write(next + 8u, sentinel);
    adjacent.Write(next + 12u, sentinel);
    adjacent.Write(sentinel + 8u, next);
    adjacent.Write(sentinel + 12u, next);
    Check(pal4::HeapReallocator::Reallocate(0x9008u, 33u, adjacent) ==
              0x9008u,
          "growth consumes free adjacent block");
    Check(adjacent.Read(sentinel + 8u) == sentinel &&
              adjacent.Read(sentinel + 12u) == sentinel,
          "adjacent block is unlinked");
    Check(adjacent.freed.size() == 1u && adjacent.freed[0] == 0x9030u,
          "extra adjacent capacity is split and freed");
}

void CheckFallbackMoveAndFailure() {
    Memory memory;
    SeedBlock(memory, 0xb008u, 32u);
    memory.Write(0xb024u, 0x21u);
    SeedBlock(memory, 0xc008u, 120u);
    for (std::uint32_t index = 0; index < 28u; ++index) {
        memory.bytes[0xb008u + index] = static_cast<std::uint8_t>(index + 1u);
    }
    memory.allocations.push_back(0xc008u);
    memory.account_on_allocate = true;
    memory.Write(kAllocated, 100u);
    Check(pal4::HeapReallocator::Reallocate(0xb008u, 100u, memory) ==
              0xc008u,
          "unavailable neighbor causes allocation and move");
    Check(memory.allocated_requests[0] == 97u &&
              memory.copied_size == 28u && memory.freed[0] == 0xb008u,
          "fallback uses normalized minus seven and copies size minus four");
    Check(memory.Read(kAllocated) == 304u,
          "fallback's observed return tail counts new payload twice");
    Check(memory.bytes[0xc008u] == 1u && memory.bytes[0xc023u] == 28u,
          "fallback copied all expected payload bytes");

    Memory failure;
    SeedBlock(failure, 0xd008u, 32u);
    failure.Write(0xd024u, 0x21u);
    Check(pal4::HeapReallocator::Reallocate(0xd008u, 100u, failure) == 0u,
          "allocation failure returns null");
    Check(failure.freed.empty() && failure.Read(0xd004u) == 33u,
          "failure leaves original allocation intact");
}

void CheckNewAllocationIsAdjacent() {
    Memory memory;
    SeedBlock(memory, 0xe008u, 32u);
    const std::uint32_t adjacent_payload = 0xe028u;
    SeedBlock(memory, adjacent_payload, 48u);
    memory.allocations.push_back(adjacent_payload);
    Check(pal4::HeapReallocator::Reallocate(0xe008u, 33u, memory) ==
              0xe008u,
          "allocator-selected adjacent chunk is absorbed in place");
    Check(memory.copied_size == 0u && memory.freed.size() == 1u &&
              memory.freed[0] == 0xe030u,
          "adjacent special path splits excess without copying old payload");
    Check(memory.Read(0xe004u) == 41u && memory.Read(0xe02cu) == 41u,
          "merged original and adjacent sizes produce a forty-byte remainder");
}

}  // namespace

int main() {
    CheckNullAndOversize();
    CheckMappedBlock();
    CheckShrinkWithoutAndWithSplit();
    CheckGrowIntoTopAndNeighbor();
    CheckFallbackMoveAndFailure();
    CheckNewAllocationIsAdjacent();
    std::cout << "6 heap reallocator test cases passed\n";
}
