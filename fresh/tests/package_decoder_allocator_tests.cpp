#include "pal4/package_decoder_allocator.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IPackageDecoderAllocatorRuntime {
public:
    std::uint32_t Malloc(std::uint32_t bytes) override {
        allocated_sizes.push_back(bytes);
        return fail_on_call == allocated_sizes.size() ? 0u :
            static_cast<std::uint32_t>(0x1000u * allocated_sizes.size());
    }
    std::uint32_t HeapCreate(std::uint32_t options,
                             std::uint32_t initial_bytes,
                             std::uint32_t maximum_bytes) override {
        ++heap_calls;
        last_options = options;
        last_initial = initial_bytes;
        last_maximum = maximum_bytes;
        return heap_result;
    }
    std::uint32_t HeapAlloc(std::uint32_t heap, std::uint32_t flags,
                            std::uint32_t bytes) override {
        last_heap = heap;
        last_options = flags;
        last_bytes = bytes;
        return heap_alloc_result;
    }
    std::uint32_t HeapFree(std::uint32_t heap, std::uint32_t flags,
                           std::uint32_t address) override {
        last_heap = heap;
        last_options = flags;
        last_address = address;
        return heap_free_result;
    }
    std::uint32_t VirtualReserve(std::uint32_t address,
                                 std::uint32_t bytes,
                                 std::uint32_t allocation_type,
                                 std::uint32_t protection) override {
        ++reserve_calls;
        last_address = address;
        last_bytes = bytes;
        last_allocation_type = allocation_type;
        last_protection = protection;
        return reserve_result;
    }
    void ReportHeapCreateFailure() override { ++reports; }
    void Free(std::uint32_t address) override {
        freed.push_back(address);
    }
    std::uint32_t HeapDestroy(std::uint32_t heap) override {
        ++destroy_calls;
        last_heap = heap;
        return destroy_result;
    }
    std::uint32_t VirtualRelease(std::uint32_t address, std::uint32_t bytes,
                                 std::uint32_t free_type) override {
        ++release_calls;
        last_address = address;
        last_bytes = bytes;
        last_free_type = free_type;
        return release_result;
    }
    void ReportHeapDestroyFailure() override { ++destroy_reports; }
    void ReportAllocationFailure(std::uint32_t bytes) override {
        ++allocation_reports;
        last_bytes = bytes;
    }
    void ReportFreeFailure() override { ++free_reports; }

    std::vector<std::uint32_t> allocated_sizes;
    std::vector<std::uint32_t> freed;
    std::size_t fail_on_call = 0;
    int heap_calls = 0;
    int reserve_calls = 0;
    int reports = 0;
    int destroy_calls = 0;
    int destroy_reports = 0;
    int release_calls = 0;
    int allocation_reports = 0;
    int free_reports = 0;
    std::uint32_t heap_result = 0xabc000u;
    std::uint32_t heap_alloc_result = 0x456000u;
    std::uint32_t heap_free_result = 1u;
    std::uint32_t release_result = 1u;
    std::uint32_t reserve_result = 0xdef000u;
    std::uint32_t destroy_result = 1u;
    std::uint32_t last_heap = 0;
    std::uint32_t last_free_type = 0;
    std::uint32_t last_options = 0;
    std::uint32_t last_initial = 0;
    std::uint32_t last_maximum = 0;
    std::uint32_t last_address = 0;
    std::uint32_t last_bytes = 0;
    std::uint32_t last_allocation_type = 0;
    std::uint32_t last_protection = 0;
};

}  // namespace

int main() {
    pal4::PackageDecoderAllocatorState state;
    Runtime runtime;
    state.reserved[0] = 0x5au;
    state.slots[39].occupied = 1u;
    runtime.fail_on_call = 2u;
    Check(&pal4::PackageDecoderAllocator::Initialize(
              state, 2u, 1u, runtime) == &state &&
          state.mode == 2u && state.flags == 1u &&
          state.reserved[0] == 0x5au &&
          runtime.allocated_sizes == std::vector<std::uint32_t>{
              0x10000u, 0x10000u, 0x10000u, 0x10000u,
              0x40000u, 0x40000u, 0x100000u, 0x400000u, 0x800000u},
          "mode two initializes nine configured pool allocations");
    Check(state.slots[0].address == 0x1000u &&
          state.slots[0].capacity == 0x10000u &&
          state.slots[0].initialized == 1u &&
          state.slots[1].address == 0u &&
          state.slots[1].initialized == 0u &&
          state.slots[8].capacity == 0x40000u &&
          state.slots[16].capacity == 0x100000u &&
          state.slots[24].capacity == 0x400000u &&
          state.slots[32].capacity == 0x800000u &&
          state.slots[39].occupied == 0u,
          "pool records follow 24-byte slots in five groups of eight");

    Check(&pal4::PackageDecoderAllocator::Initialize(
              state, 3u, 0u, runtime) == &state &&
          runtime.heap_calls == 1 && runtime.last_options == 1u &&
          runtime.last_initial == 4096u && runtime.last_maximum == 0u &&
          state.heap == runtime.heap_result && state.heap_bytes == 4096u &&
          state.slots[0].address == 0u,
          "mode three creates a private heap and clears pool state");
    runtime.heap_result = 0u;
    Check(&pal4::PackageDecoderAllocator::Initialize(
              state, 3u, 1u, runtime) == &state &&
          runtime.heap_calls == 2 && runtime.last_options == 5u &&
          runtime.reports == 1 && state.heap == 0u &&
          state.heap_bytes == 4096u,
          "failed private heap reports while retaining initial size");

    Check(&pal4::PackageDecoderAllocator::Initialize(
              state, 5u, 1u, runtime) == &state &&
          runtime.reserve_calls == 1 && runtime.last_address == 0u &&
          runtime.last_bytes == 0x20000000u &&
          runtime.last_allocation_type == 0x2000u &&
          runtime.last_protection == 1u &&
          state.reserved_region == runtime.reserve_result &&
          state.heap_bytes == 0u,
          "mode five reserves a 512 MiB virtual range");
    const auto prior_allocations = runtime.allocated_sizes.size();
    Check(&pal4::PackageDecoderAllocator::Initialize(
              state, 4u, 7u, runtime) == &state &&
          state.mode == 4u && state.flags == 7u &&
          state.reserved_region == 0u &&
          runtime.allocated_sizes.size() == prior_allocations &&
          runtime.heap_calls == 2 && runtime.reserve_calls == 1,
          "unrecognized mode only resets base state");

    pal4::PackageDecoderAllocator::Cleanup(state, runtime);
    Check(runtime.freed.empty() && runtime.destroy_calls == 0 &&
          runtime.release_calls == 0,
          "unrecognized mode has no cleanup side effect");
    runtime.fail_on_call = 0u;
    runtime.allocated_sizes.clear();
    (void)pal4::PackageDecoderAllocator::Initialize(state, 2u, 1u, runtime);
    pal4::PackageDecoderAllocator::Cleanup(state, runtime);
    Check(runtime.freed == std::vector<std::uint32_t>{
              0x1000u, 0x2000u, 0x3000u, 0x4000u, 0x5000u,
              0x6000u, 0x7000u, 0x8000u, 0x9000u} &&
          state.slots[0].address == 0x1000u,
          "mode two frees all initialized slots without clearing records");
    (void)pal4::PackageDecoderAllocator::Initialize(state, 3u, 0u, runtime);
    pal4::PackageDecoderAllocator::Cleanup(state, runtime);
    Check(runtime.destroy_calls == 1 &&
          runtime.last_heap == runtime.heap_result &&
          state.heap == 0u && runtime.destroy_reports == 0,
          "mode three destroys and clears the heap handle");
    runtime.destroy_result = 0u;
    state.heap = 0x112233u;
    pal4::PackageDecoderAllocator::Cleanup(state, runtime);
    Check(runtime.destroy_calls == 2 && runtime.destroy_reports == 1 &&
          runtime.last_heap == 0x112233u && state.heap == 0u,
          "failed heap destruction still clears the handle");
    (void)pal4::PackageDecoderAllocator::Initialize(state, 5u, 1u, runtime);
    pal4::PackageDecoderAllocator::Cleanup(state, runtime);
    Check(runtime.release_calls == 1 &&
          runtime.last_address == runtime.reserve_result &&
          runtime.last_bytes == 0u && runtime.last_free_type == 0x8000u &&
          state.reserved_region == runtime.reserve_result,
          "mode five releases the reservation without clearing its field");

    (void)pal4::PackageDecoderAllocator::Initialize(state, 2u, 1u, runtime);
    const auto first = pal4::PackageDecoderAllocator::Allocate(
        state, 0x100u, runtime);
    Check(first != 0u && state.slots[0].occupied == 1u &&
          state.slots[0].requested == 0x100u &&
          state.slots[0].high_water == 0x100u &&
          state.slots[0].use_count == 1u && state.slots[0].touched == 1u,
          "pool allocation tracks requested size and first use");
    Check(pal4::PackageDecoderAllocator::Release(state, first, 0x100u,
                                                  runtime) == 1u &&
          state.slots[0].occupied == 0u && state.slots[0].requested == 0u,
          "pool release leaves reusable allocation in place");
    Check(pal4::PackageDecoderAllocator::Allocate(state, 0x200u, runtime) ==
              first && state.slots[0].use_count == 2u &&
          state.slots[0].high_water == 0x200u,
          "pool reuse increments use count and high water");
    const auto allocations_before = runtime.allocated_sizes.size();
    Check(pal4::PackageDecoderAllocator::Allocate(state, 0x900000u, runtime)
              == 0u && runtime.allocation_reports == 1 &&
          runtime.allocated_sizes.size() == allocations_before + 1u &&
          runtime.allocated_sizes.back() == 0x900000u,
          "oversize pool fallback discards CRT allocation result");
    Check(pal4::PackageDecoderAllocator::Release(state, 0xfed000u, 1u,
                                                  runtime) == 1u &&
          runtime.freed.back() == 0xfed000u,
          "unknown pool pointer goes to CRT free");

    (void)pal4::PackageDecoderAllocator::Initialize(state, 1u, 0u, runtime);
    Check(pal4::PackageDecoderAllocator::Allocate(state, 33u, runtime) != 0u &&
          pal4::PackageDecoderAllocator::Release(state, 0x123u, 33u,
                                                  runtime) == 1u,
          "mode one delegates directly to CRT allocation and free");
    (void)pal4::PackageDecoderAllocator::Initialize(state, 3u, 1u, runtime);
    Check(pal4::PackageDecoderAllocator::Allocate(state, 77u, runtime) ==
              runtime.heap_alloc_result && runtime.last_heap == state.heap &&
          runtime.last_options == 5u && runtime.last_bytes == 77u,
          "mode three forwards heap allocation flags and size");
    runtime.heap_free_result = 0u;
    Check(pal4::PackageDecoderAllocator::Release(state, 0x123u, 77u,
                                                  runtime) == 0u &&
          runtime.last_options == 5u && runtime.free_reports == 1,
          "failed heap free reports without changing result");
    (void)pal4::PackageDecoderAllocator::Initialize(state, 5u, 1u, runtime);
    Check(pal4::PackageDecoderAllocator::Allocate(state, 4096u, runtime) ==
              runtime.reserve_result && runtime.last_address ==
              state.reserved_region && runtime.last_allocation_type == 0x1000u &&
          runtime.last_protection == 4u,
          "mode five commits reserved virtual memory");
    runtime.release_result = 0u;
    Check(pal4::PackageDecoderAllocator::Release(state, 0x999u, 4096u,
                                                  runtime) == 0u &&
          runtime.last_address == state.reserved_region &&
          runtime.last_bytes == 4096u && runtime.last_free_type == 0x4000u &&
          runtime.free_reports == 2,
          "mode five decommits at reserved base and reports failure");
    std::cout << "20 package decoder allocator cases passed\n";
}
