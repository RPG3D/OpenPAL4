#include "pal4/native_game_object_animation_lifecycle.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class AnimationRuntime final : public pal4::NativeGameObjectAnimationLifecycle {
public:
    using NativeGameObjectAnimationLifecycle::NativeGameObjectAnimationLifecycle;

    std::uint32_t FindHierarchy(std::uint32_t) override {
        throw std::logic_error("hierarchy is outside this lifetime test");
    }
    void ProcessAnimationData(pal4::GameObjectEmbeddedAnimationState&,
                              std::uint32_t, std::int32_t) override {
        throw std::logic_error("render processing is outside this lifetime test");
    }
};

std::uint32_t AllocateText(pal4::NativeBaseObjectMemory& memory,
                           std::uint8_t references) {
    const auto allocation = memory.Allocate(4u);
    memory.WriteByte(allocation, references);
    return allocation + 1u;
}

void TestPopulatedRange() {
    pal4::NativeBaseObjectMemory memory;
    AnimationRuntime runtime(memory);
    pal4::GameObjectEmbeddedAnimationState state;
    const auto records = memory.Allocate(64u);
    std::memset(reinterpret_cast<void*>(records), 0, 64u);
    const auto shared = AllocateText(memory, 1u);
    const auto unique = AllocateText(memory, 0u);
    const auto sentinel = AllocateText(memory, 0xffu);
    memory.WriteDword(records + 20u, shared);
    memory.WriteDword(records + 4u, unique);
    memory.WriteDword(records + 52u, shared);
    memory.WriteDword(records + 36u, sentinel);
    state.words[21u] = records;
    state.words[22u] = records + 64u;
    state.words[23u] = records + 64u;

    runtime.CleanupAnimationDataRange(state);
    Check(state.words[22u] == records &&
              memory.OutstandingAllocations() == 1u,
          "two records release their unique and shared text exactly once");
    for (std::uint32_t record = records; record < records + 64u;
         record += 32u)
        for (const auto offset : {4u, 8u, 12u, 20u, 24u, 28u})
            Check(memory.ReadDword(record + offset) == 0u,
                  "record text state is cleared");

    runtime.CleanupAnimationData(state);
    Check(state.words[21u] == 0u && state.words[22u] == 0u &&
              state.words[23u] == 0u &&
              memory.OutstandingAllocations() == 0u,
          "manager buffer is released after record cleanup");
}

void TestRetainedReferenceAndInvalidRange() {
    pal4::NativeBaseObjectMemory memory;
    AnimationRuntime runtime(memory);
    pal4::GameObjectEmbeddedAnimationState state;
    const auto records = memory.Allocate(32u);
    std::memset(reinterpret_cast<void*>(records), 0, 32u);
    const auto shared = AllocateText(memory, 2u);
    memory.WriteDword(records + 4u, shared);
    state.words[21u] = records;
    state.words[22u] = records + 31u;
    bool rejected = false;
    try {
        runtime.CleanupAnimationDataRange(state);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    Check(rejected && memory.ReadByte(shared - 1u) == 2u,
          "misaligned manager range leaves references unchanged");

    state.words[21u] = 0u;
    state.words[22u] = 32u;
    rejected = false;
    try {
        runtime.CleanupAnimationDataRange(state);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    Check(rejected, "nonempty range cannot start at null");

    state.words[21u] = records;
    state.words[22u] = records + 32u;
    runtime.CleanupAnimationDataRange(state);
    Check(memory.ReadByte(shared - 1u) == 1u &&
              memory.OutstandingAllocations() == 2u,
          "a reference held by another owner remains allocated");
    runtime.CleanupAnimationData(state);
    (void)memory.Free(shared - 1u);
    Check(memory.OutstandingAllocations() == 0u,
          "all owned allocations are released");
}

}  // namespace

int main() {
    TestPopulatedRange();
    TestRetainedReferenceAndInvalidRange();
    return EXIT_SUCCESS;
}
