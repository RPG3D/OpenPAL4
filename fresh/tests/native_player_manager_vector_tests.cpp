#include "pal4/native_player_manager_vector_runtime.h"
#include "pal4/native_player_slot_array_runtime.h"
#include "pal4/native_player_slot_base_runtime.h"
#include "pal4/native_animation_event_queue_runtime.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Address(void* object) {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(object));
}

struct SlotRuntime final : pal4::PlayerManagerSlotRuntimeStub {
    std::uint32_t fail_byte_at{};
    std::uint32_t free_calls{};
    std::vector<std::uint32_t> freed;
    std::uint32_t ReadDword(const std::uint32_t address) override {
        return *reinterpret_cast<const std::uint32_t*>(
            static_cast<std::uintptr_t>(address));
    }
    void WriteDword(const std::uint32_t address,
                    const std::uint32_t value) override {
        *reinterpret_cast<std::uint32_t*>(
            static_cast<std::uintptr_t>(address)) = value;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        if (address == fail_byte_at)
            throw std::runtime_error("injected block element failure");
        *reinterpret_cast<std::uint8_t*>(
            static_cast<std::uintptr_t>(address)) = value;
    }
    void Free(const std::uint32_t address) override {
        ++free_calls;
        if (address != 0u) freed.push_back(address);
    }
};

struct BaseRuntime final : pal4::BaseObjectRuntimeStub,
                           pal4::IBaseObjectActivityRuntime {
    std::uint32_t manager;
    int fail_on = -1;
    std::vector<std::uint32_t> constructed;
    std::vector<std::uint32_t> destroyed;

    explicit BaseRuntime(std::uint32_t manager) : manager(manager) {}

    std::uint32_t Index(pal4::BaseObjectState& state) const {
        const auto slot = Address(state.Data());
        Check(slot >= manager &&
                  (slot - manager) % pal4::PlayerManagerSlot::kObjectBytes == 0u,
              "base object address follows manager layout");
        const auto index = (slot - manager) /
            pal4::PlayerManagerSlot::kObjectBytes;
        Check(index < pal4::PlayerManagerObject::kElementCount,
              "base object index is in range");
        return index;
    }
    void ConstructResource(pal4::BaseObjectState& state,
                           std::uint32_t offset) override {
        if (offset != 0u) return;
        const auto index = Index(state);
        if (static_cast<int>(index) == fail_on)
            throw std::runtime_error("injected base constructor failure");
        constructed.push_back(index);
    }
    void DestroyResource(pal4::BaseObjectState& state,
                         std::uint32_t offset) override {
        if (offset == 0u) destroyed.push_back(Index(state));
    }
};

struct QueueActivity final : pal4::IAnimationEventQueueActivityRuntime {
    std::uint32_t deleted{};
    void DeleteAnimation(std::uint32_t animation) override {
        Check(animation == 0xbeefu, "owned queue animation address");
        ++deleted;
    }
    void AdvanceAnimation(std::uint32_t, float) override { std::abort(); }
    void CallAnimationEvent(std::uint32_t, std::uint32_t) override {
        std::abort();
    }
    std::uint32_t AnimationFinished(std::uint32_t) override {
        std::abort();
    }
};

}  // namespace

int main() {
    pal4::PlayerManagerObjectState state{};
    const auto manager = Address(&state);
    for (std::size_t index = 0u; index < state.slots.size(); ++index) {
        const auto first = index * pal4::PlayerManagerSlot::kObjectBytes / 4u;
        Check(&state.Word(first) == &state.slots[index].base.words[0] &&
                  &state.Word(first + pal4::BaseObjectState::kSize / 4u) ==
                      &state.slots[index].tail[0],
              "word view follows every real base and slot tail boundary");
    }
    Check(&state.Word(4u * pal4::PlayerManagerSlot::kObjectBytes / 4u) ==
              &state.tail[0] &&
              &state.Word(0x2c64u / 4u - 1u) == &state.tail.back(),
          "word view includes the manager tail without crossing its end");
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto slot = index * pal4::PlayerManagerSlot::kObjectBytes;
        for (std::uint32_t offset = 0x920u; offset < 0x938u;
             offset += 4u)
            state.Word((slot + offset) / 4u) = 0xffffffffu;
        state.Word((slot + 0x93cu) / 4u) = 0xffffffffu;
    }
    SlotRuntime slots;
    BaseRuntime base(manager);
    pal4::NativePlayerSlotBaseRuntime bases(state, slots, base, base);
    pal4::NativePlayerSlotCopyRuntime text;
    pal4::NativeBaseObjectMemory queue_memory;
    QueueActivity queue_activity;
    pal4::NativeAnimationEventQueueRuntime queue(
        queue_memory, queue_activity);
    pal4::NativePlayerSlotArrayRuntime arrays(bases, text, queue);
    pal4::NativePlayerManagerVectorRuntime vector(arrays);
    Check(pal4::PlayerManagerObject::Construct(state, manager, vector) ==
              manager &&
              base.constructed ==
                  std::vector<std::uint32_t>{0u, 1u, 2u, 3u} &&
              state.Word(0x2c50u / 4u) == 0u,
          "native manager constructs four rewritten player slots");
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto offset = index * pal4::PlayerManagerSlot::kObjectBytes;
        Check(state.Word(offset / 4u) ==
                  pal4::PlayerManagerSlot::kMainVtable &&
                  state.Word((offset + 0xb0cu) / 4u) == 1u &&
                  state.Word((offset + 0x920u) / 4u) == 0u &&
                  state.Word((offset + 0x934u) / 4u) == 0u &&
                  state.Word((offset + 0x93cu) / 4u) == 0u &&
                  state.Word((offset + 0x9f8u + 80u) / 4u) == 4u &&
                  state.Word((offset + 0x20cu) / 4u) == manager + offset &&
                  state.Word((offset + 0x214u) / 4u) == manager + offset &&
                  state.Word((offset + 0xaacu) / 4u) != 0u,
              "each slot initializes base links, blocks and Role arrays");
    }
    Check(queue_memory.OutstandingAllocations() == 4u,
          "four slot event queues own four list heads");
    (void)pal4::AnimationEventQueue::Add(
        manager + pal4::PlayerManagerSlot::kTailOffset, 0u, 0u, queue);
    (void)pal4::AnimationEventQueue::Add(
        manager + pal4::PlayerManagerSlot::kTailOffset, 1u, 0xbeefu,
        queue);
    Check(queue_memory.OutstandingAllocations() == 6u,
          "native event queue allocates owned and unowned list nodes");
    state.Word(0x93cu / 4u) = 0xdeadbeefu;
    pal4::PlayerManagerObject::Destroy(manager, vector);
    Check(queue_memory.OutstandingAllocations() == 0u &&
              queue_activity.deleted == 1u &&
              base.destroyed ==
              std::vector<std::uint32_t>{3u, 2u, 1u, 0u} &&
              slots.freed == std::vector<std::uint32_t>{0xdeadbeefu} &&
              state.Word(0x93cu / 4u) == 0u,
          "native manager destroys slots and their block storage");

    pal4::PlayerManagerObjectState failed_state{};
    const auto failed_manager = Address(&failed_state);
    SlotRuntime failing_slots;
    BaseRuntime failing_base(failed_manager);
    failing_base.fail_on = 2;
    pal4::NativePlayerSlotBaseRuntime failing_bases(
        failed_state, failing_slots, failing_base, failing_base);
    pal4::NativePlayerSlotArrayRuntime failing_arrays(
        failing_bases, text, queue);
    pal4::NativePlayerManagerVectorRuntime failing(failing_arrays);
    bool threw = false;
    try {
        (void)pal4::PlayerManagerObject::Construct(
            failed_state, failed_manager, failing);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failing_base.constructed ==
                       std::vector<std::uint32_t>{0u, 1u} &&
              failing_base.destroyed ==
                  std::vector<std::uint32_t>{1u, 0u} &&
              queue_memory.OutstandingAllocations() == 0u,
          "third-slot failure rolls back two completed constructors");
    failing_base.fail_on = -1;
    Check(pal4::PlayerManagerObject::Construct(
              failed_state, failed_manager, failing) == failed_manager,
          "failed vector can construct again after rollback");
    pal4::PlayerManagerObject::Destroy(failed_manager, failing);
    Check(queue_memory.OutstandingAllocations() == 0u,
          "reconstructed manager releases all event queue storage");

    pal4::PlayerManagerObjectState partial_state{};
    const auto partial_manager = Address(&partial_state);
    SlotRuntime partial_slots;
    pal4::NativePlayerSlotArrayRuntime partial_arrays(
        partial_slots, text, queue);
    const auto array = partial_manager +
        pal4::PlayerManagerSlot::kFirstBlockArrayOffset;
    partial_slots.fail_byte_at = array + 2u * 16u;
    threw = false;
    try {
        partial_arrays.ConstructBlockArray(array, 5u, 16u);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && partial_slots.free_calls == 2u &&
              partial_state.Word((0x938u + 4u) / 4u) == 0u &&
              partial_state.Word((0x938u + 16u + 4u) / 4u) == 0u,
          "partial block-array construction cleans completed elements");
    const auto foreign_text = partial_manager +
        pal4::PlayerManagerSlot::kResourceArrayOffset;
    partial_state.Word((pal4::PlayerManagerSlot::kResourceArrayOffset +
                        4u) / 4u) = 0xdeadbeefu;
    threw = false;
    try {
        text.ReleaseText(foreign_text);
    } catch (const std::logic_error&) {
        threw = true;
    }
    Check(threw &&
              partial_state.Word((pal4::PlayerManagerSlot::
                  kResourceArrayOffset + 4u) / 4u) == 0xdeadbeefu,
          "native slot owner rejects a foreign text pointer");
    std::cout << "PASS: native player manager vector lifecycle\n";
}
