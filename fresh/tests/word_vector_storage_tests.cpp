#include "pal4/word_vector_storage.h"

#include <cstdint>
#include <iostream>
#include <new>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kVector = 0x1000u;
constexpr std::uint32_t kSource = 0x2000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IWordVectorStorageRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    bool fail_allocation = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        allocations.push_back(bytes);
        return fail_allocation ? 0u :
            0x8000u + static_cast<std::uint32_t>(allocations.size() - 1u) *
                          0x100u;
    }
    void Free(std::uint32_t address) override {
        frees.push_back(address);
    }
};

void ClearRetainsCapacity() {
    Runtime runtime;
    runtime.words[kVector + 4u] = 0x3000u;
    runtime.words[kVector + 8u] = 0x3008u;
    runtime.words[kVector + 12u] = 0x3010u;
    Check(pal4::WordVectorStorage::Clear(kVector, runtime) == 0x3008u &&
              runtime.words[kVector + 8u] == 0x3000u &&
              runtime.words[kVector + 12u] == 0x3010u &&
              runtime.frees.empty(),
          "clear resets end and preserves the buffer");
}

void InsertWithCapacity() {
    Runtime runtime;
    runtime.words[kVector + 4u] = 0x3000u;
    runtime.words[kVector + 8u] = 0x300cu;
    runtime.words[kVector + 12u] = 0x3010u;
    runtime.words[0x3000u] = 10u;
    runtime.words[0x3004u] = 20u;
    runtime.words[0x3008u] = 30u;
    runtime.words[kSource] = 15u;
    Check(pal4::WordVectorStorage::Insert(
              kVector, 0x3004u, kSource, runtime) == 0x3004u &&
              runtime.words[0x3000u] == 10u &&
              runtime.words[0x3004u] == 15u &&
              runtime.words[0x3008u] == 20u &&
              runtime.words[0x300cu] == 30u &&
              runtime.words[kVector + 8u] == 0x3010u,
          "in-place insert shifts the suffix");
    runtime.words[0x3004u] = 99u;
    runtime.words[kVector + 8u] = 0x3008u;
    runtime.words[kVector + 12u] = 0x3010u;
    Check(pal4::WordVectorStorage::Insert(
              kVector, 0x3008u, 0x3004u, runtime) == 0x3008u &&
              runtime.words[0x3008u] == 99u &&
              runtime.words[kVector + 8u] == 0x300cu,
          "end insertion can read an existing element");
}

void GrowthAndAlias() {
    Runtime runtime;
    runtime.words[kSource] = 7u;
    Check(pal4::WordVectorStorage::Insert(
              kVector, 0u, kSource, runtime) == 0x8000u &&
              runtime.allocations == std::vector<std::uint32_t>{4u} &&
              runtime.words[kVector + 4u] == 0x8000u &&
              runtime.words[kVector + 8u] == 0x8004u &&
              runtime.words[kVector + 12u] == 0x8004u &&
              runtime.frees == std::vector<std::uint32_t>{0u},
          "first insertion allocates one slot");
    runtime.words[kSource] = 9u;
    Check(pal4::WordVectorStorage::Insert(
              kVector, 0x8000u, kSource, runtime) == 0x8100u &&
              runtime.allocations.back() == 8u &&
              runtime.words[0x8100u] == 9u &&
              runtime.words[0x8104u] == 7u &&
              runtime.frees.back() == 0x8000u,
          "growth preserves suffix and returns relocated insertion address");
    Check(pal4::WordVectorStorage::Insert(
              kVector, 0x8104u, 0x8100u, runtime) == 0x8204u &&
              runtime.allocations.back() == 16u &&
              runtime.words[0x8200u] == 9u &&
              runtime.words[0x8204u] == 9u &&
              runtime.words[0x8208u] == 7u,
          "growth reads aliased source before freeing old storage");
}

void AllocationFailure() {
    Runtime runtime;
    runtime.words[kVector + 4u] = 0x3000u;
    runtime.words[kVector + 8u] = 0x3004u;
    runtime.words[kVector + 12u] = 0x3004u;
    runtime.words[kSource] = 42u;
    runtime.fail_allocation = true;
    bool threw = false;
    try {
        (void)pal4::WordVectorStorage::Insert(
            kVector, 0x3004u, kSource, runtime);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && runtime.words[kVector + 4u] == 0x3000u &&
              runtime.words[kVector + 8u] == 0x3004u &&
              runtime.frees.empty(),
          "failed allocation leaves the vector intact");
}

}  // namespace

int main() {
    try {
        ClearRetainsCapacity();
        InsertWithCapacity();
        GrowthAndAlias();
        AllocationFailure();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
