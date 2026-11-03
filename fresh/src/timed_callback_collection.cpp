#include "pal4/timed_callback_collection.h"

#include <algorithm>
#include <bit>

namespace pal4 {

void TimedCallbackCollection::ReleaseSlot(std::uint32_t slot) {
    const auto owned = runtime_.ReadWord(slot + 4u);
    if (owned == 0u) return;
    if (runtime_.ReadByte(slot) != 0u)
        runtime_.DeleteOwned(owned, 1u);
    runtime_.WriteByte(slot, 0u);
    runtime_.WriteWord(slot + 4u, 0u);
}

void TimedCallbackCollection::ReleaseEvent(std::uint32_t payload) {
    ReleaseSlot(payload + 0x14u);
}

void TimedCallbackCollection::ReleaseTimer(std::uint32_t payload) {
    ReleaseSlot(payload + 0x18u);
}

void TimedCallbackCollection::ReleaseAll(TimedCallbackCollectionState& state) {
    auto node = runtime_.ReadWord(state.event_head);
    while (node != state.event_head) {
        ReleaseEvent(node + 8u);
        node = runtime_.ReadWord(node);
    }
    node = runtime_.ReadWord(state.timer_head);
    while (node != state.timer_head) {
        ReleaseTimer(node + 8u);
        node = runtime_.ReadWord(node);
    }
}

std::uint32_t TimedCallbackCollection::ReleaseByName(
    std::uint32_t head, const TimedCallbackName& name,
    bool event_list) {
    std::uint32_t matched = 0u;
    auto node = runtime_.ReadWord(head);
    while (node != head) {
        const auto data = runtime_.ReadWord(node + 12u);
        const auto length = runtime_.ReadWord(node + 16u);
        const auto requested_data = name.data == 0u ? 0x841964u : name.data;
        const auto shared = std::min(length, name.length);
        if (runtime_.CompareBytes(data, requested_data, shared) == 0 &&
            length == name.length) {
            ++matched;
            if (event_list)
                ReleaseEvent(node + 8u);
            else
                ReleaseTimer(node + 8u);
        }
        node = runtime_.ReadWord(node);
    }
    return matched;
}

std::uint32_t TimedCallbackCollection::ReleaseTimerByName(
    TimedCallbackCollectionState& state, const TimedCallbackName& name) {
    return ReleaseByName(state.timer_head, name, false);
}

std::uint32_t TimedCallbackCollection::ReleaseEventByName(
    TimedCallbackCollectionState& state, const TimedCallbackName& name) {
    return ReleaseByName(state.event_head, name, true);
}

float TimedCallbackCollection::ReadFloat(std::uint32_t address) {
    return std::bit_cast<float>(runtime_.ReadWord(address));
}

void TimedCallbackCollection::WriteFloat(std::uint32_t address, float value) {
    runtime_.WriteWord(address, std::bit_cast<std::uint32_t>(value));
}

void TimedCallbackCollection::Unlink(std::uint32_t node) {
    const auto previous = runtime_.ReadWord(node + 4u);
    const auto next = runtime_.ReadWord(node);
    runtime_.WriteWord(previous, next);
    runtime_.WriteWord(next + 4u, previous);
}

std::uint32_t TimedCallbackCollection::DestroyEventPayload(
    std::uint32_t payload, std::uint32_t flags) {
    runtime_.CleanupEventData(payload);
    if ((flags & 1u) != 0u) runtime_.FreeNode(payload);
    return payload;
}

void TimedCallbackCollection::Update(TimedCallbackCollectionState& state,
                                     float delta) {
    if (delta == 0.0f) return;

    auto node = runtime_.ReadWord(state.event_head);
    while (node != state.event_head) {
        const auto next = runtime_.ReadWord(node);
        const auto callback = runtime_.ReadWord(node + 0x20u);
        if (callback == 0u) {
            Unlink(node);
            static_cast<void>(DestroyEventPayload(node + 8u, 0u));
            runtime_.FreeNode(node);
            --state.event_count;
        } else {
            const auto remaining = ReadFloat(node + 0x18u);
            if (remaining < delta) {
                runtime_.InvokeEvent(callback);
                ReleaseEvent(node + 8u);
            } else {
                WriteFloat(node + 0x18u, remaining - delta);
            }
        }
        node = next;
    }

    node = runtime_.ReadWord(state.timer_head);
    while (node != state.timer_head) {
        const auto next = runtime_.ReadWord(node);
        const auto callback = runtime_.ReadWord(node + 0x24u);
        if (callback == 0u) {
            Unlink(node);
            runtime_.CleanupTimerData(node + 8u);
            runtime_.FreeNode(node);
            --state.timer_count;
        } else {
            const auto remaining = ReadFloat(node + 0x18u);
            if (remaining < delta) {
                const float overrun = delta - remaining;
                runtime_.InvokeTimer(callback);
                const float next_remaining =
                    ReadFloat(node + 0x1cu) - overrun;
                WriteFloat(node + 0x18u,
                           next_remaining < 0.0f ? 0.0f : next_remaining);
            } else {
                WriteFloat(node + 0x18u, remaining - delta);
            }
        }
        node = next;
    }
}

}  // namespace pal4

