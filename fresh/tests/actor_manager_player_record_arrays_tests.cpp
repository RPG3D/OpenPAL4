#include "pal4/actor_manager_player_record_arrays.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kDestination = 0x1000u;
constexpr std::uint32_t kSource = 0x2000u;
constexpr std::uint32_t kOld = 0x3000u;
constexpr std::uint32_t kInput = 0x4000u;
constexpr std::uint32_t kReplacement = 0x5000u;

struct Runtime final : pal4::IActorManagerPlayerRecordArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> writes;
    std::vector<std::uint32_t> frees;
    std::vector<std::uint32_t> allocations;
    std::uint32_t allocation_result = kReplacement;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        writes.push_back(address);
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        allocations.push_back(bytes);
        return allocation_result;
    }
    void Free(std::uint32_t address) override {
        frees.push_back(address);
    }
};

using AssignFunction = std::uint32_t (*)(
    std::uint32_t, std::uint32_t,
    pal4::IActorManagerPlayerRecordArrayRuntime&);

void Prepare(Runtime& runtime, std::uint32_t stride,
             std::uint32_t source_count, std::uint32_t old_count,
             std::uint32_t old_capacity) {
    runtime.words[kSource] = 0xaau;
    runtime.words[kDestination] = 0xbbu;
    runtime.words[kSource + 4u] = kInput;
    runtime.words[kSource + 8u] = kInput + source_count * stride;
    runtime.words[kSource + 12u] = kInput + source_count * stride;
    runtime.words[kDestination + 4u] = kOld;
    runtime.words[kDestination + 8u] = kOld + old_count * stride;
    runtime.words[kDestination + 12u] = kOld + old_capacity * stride;
    for (std::uint32_t i = 0u; i < source_count * stride; i += 4u)
        runtime.words[kInput + i] = 100u + i / 4u;
    for (std::uint32_t i = 0u; i < old_count * stride; i += 4u)
        runtime.words[kOld + i] = 900u + i / 4u;
}

void CheckElements(const Runtime& runtime, std::uint32_t base,
                   std::uint32_t stride, std::uint32_t count) {
    for (std::uint32_t i = 0u; i < count * stride; i += 4u)
        Check(runtime.words.at(base + i) == 100u + i / 4u,
              "all record words copied in order");
}

void Exercise(std::uint32_t stride, AssignFunction assign) {
    Runtime self;
    Prepare(self, stride, 3u, 2u, 4u);
    Check(assign(kDestination, kDestination, self) == kDestination &&
              self.writes.empty() && self.frees.empty() &&
              self.allocations.empty(),
          "self assignment leaves the array untouched");

    Runtime shorter;
    Prepare(shorter, stride, 2u, 3u, 4u);
    Check(assign(kDestination, kSource, shorter) == kDestination &&
              shorter.words[kDestination + 8u] == kOld + 2u * stride &&
              shorter.words[kDestination + 12u] == kOld + 4u * stride &&
              shorter.frees.empty() && shorter.allocations.empty() &&
              shorter.words[kDestination] == 0xbbu,
          "shorter source reuses storage and preserves marker and capacity");
    CheckElements(shorter, kOld, stride, 2u);

    Runtime spare;
    Prepare(spare, stride, 3u, 1u, 4u);
    Check(assign(kDestination, kSource, spare) == kDestination &&
              spare.words[kDestination + 8u] == kOld + 3u * stride &&
              spare.words[kDestination + 12u] == kOld + 4u * stride &&
              spare.frees.empty() && spare.allocations.empty(),
          "longer source fills spare capacity without reallocating");
    CheckElements(spare, kOld, stride, 3u);

    Runtime growth;
    Prepare(growth, stride, 3u, 1u, 2u);
    Check(assign(kDestination, kSource, growth) == kDestination &&
              growth.frees == std::vector<std::uint32_t>{kOld} &&
              growth.allocations ==
                  std::vector<std::uint32_t>{3u * stride} &&
              growth.words[kDestination + 4u] == kReplacement &&
              growth.words[kDestination + 8u] ==
                  kReplacement + 3u * stride &&
              growth.words[kDestination + 12u] ==
                  kReplacement + 3u * stride &&
              growth.words[kDestination] == 0xbbu,
          "insufficient capacity allocates exactly the source size");
    CheckElements(growth, kReplacement, stride, 3u);

    Runtime empty;
    Prepare(empty, stride, 0u, 2u, 3u);
    empty.words[kSource + 4u] = 0u;
    empty.words[kSource + 8u] = 0u;
    Check(assign(kDestination, kSource, empty) == kDestination &&
              empty.words[kDestination + 8u] == kOld &&
              empty.words[kDestination + 12u] == kOld + 3u * stride &&
              empty.frees.empty() && empty.allocations.empty(),
          "empty source clears length but retains allocated capacity");

    Runtime failed;
    Prepare(failed, stride, 3u, 1u, 2u);
    failed.allocation_result = 0u;
    Check(assign(kDestination, kSource, failed) == kDestination &&
              failed.frees == std::vector<std::uint32_t>{kOld} &&
              failed.allocations ==
                  std::vector<std::uint32_t>{3u * stride} &&
              failed.words[kDestination + 4u] == 0u &&
              failed.words[kDestination + 8u] == 0u &&
              failed.words[kDestination + 12u] == 0u,
          "allocation failure leaves an empty valid array");
}

}  // namespace

int main() {
    try {
        Exercise(8u, &pal4::ActorManagerPlayerRecordArrays::AssignPairs);
        Exercise(12u, &pal4::ActorManagerPlayerRecordArrays::AssignTriples);
        std::cout << "actor_manager_player_record_arrays_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
