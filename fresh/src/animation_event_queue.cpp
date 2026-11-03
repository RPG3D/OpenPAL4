#include "pal4/animation_event_queue.h"

#include <bit>
#include <cmath>
#include <initializer_list>

namespace pal4 {

std::uint32_t AnimationEventQueue::Construct(
    const std::uint32_t object, const std::uint8_t seed,
    IAnimationEventQueueRuntime& runtime) {
    runtime.WriteByte(object, seed);
    const std::uint32_t head = runtime.Allocate(16u);
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 4u, head);
    runtime.WriteWord(object + 4u, head);
    runtime.WriteWord(object + 8u, 0u);
    runtime.WriteByte(object + 12u, seed);
    for (const auto offset : {16u, 20u, 24u, 28u, 32u, 36u, 40u,
                              44u, 48u, 52u, 56u, 64u})
        runtime.WriteWord(object + offset, 0u);
    runtime.WriteWord(object + 60u, 0x3e4ccccdu);
    return object;
}

std::uint32_t AnimationEventQueue::RemoveNode(
    const std::uint32_t node, IAnimationEventQueueRuntime& runtime) {
    const std::uint32_t next = runtime.ReadWord(node);
    const std::uint32_t previous = runtime.ReadWord(node + 4u);
    runtime.WriteWord(previous, next);
    runtime.WriteWord(next + 4u, previous);
    if (runtime.ReadByte(node + 8u) != 0u) {
        const std::uint32_t animation = runtime.ReadWord(node + 12u);
        if (animation != 0u) runtime.DeleteAnimation(animation);
    }
    return runtime.Free(node);
}

std::uint32_t AnimationEventQueue::Clear(
    const std::uint32_t object, IAnimationEventQueueRuntime& runtime) {
    const std::uint32_t head = runtime.ReadWord(object + 4u);
    std::uint32_t node = runtime.ReadWord(head);
    while (node != head) {
        const std::uint32_t next = runtime.ReadWord(node);
        (void)RemoveNode(node, runtime);
        runtime.WriteWord(object + 8u,
                          runtime.ReadWord(object + 8u) - 1u);
        node = next;
    }

    std::uint32_t remaining = runtime.ReadWord(object + 56u);
    while (remaining != 0u) {
        const std::uint32_t cursor = runtime.ReadWord(object + 24u);
        runtime.WriteWord(object + 24u, cursor + 8u);
        if (runtime.ReadByte(cursor) != 0u) {
            const std::uint32_t animation = runtime.ReadWord(cursor + 4u);
            if (animation != 0u) runtime.DeleteAnimation(animation);
        }
        --remaining;
        runtime.WriteWord(object + 56u, remaining);
        if (remaining == 0u ||
            runtime.ReadWord(object + 24u) ==
                runtime.ReadWord(object + 20u)) {
            const std::uint32_t map_cursor = runtime.ReadWord(object + 28u);
            runtime.WriteWord(object + 28u, map_cursor + 4u);
            (void)runtime.Free(runtime.ReadWord(map_cursor));
            if (remaining == 0u) {
                for (const auto offset : {16u, 20u, 24u, 28u, 32u,
                                          36u, 40u, 44u})
                    runtime.WriteWord(object + offset, 0u);
                (void)runtime.Free(runtime.ReadWord(object + 48u));
            } else {
                const std::uint32_t next_map =
                    runtime.ReadWord(object + 28u);
                const std::uint32_t block = runtime.ReadWord(next_map);
                runtime.WriteWord(object + 16u, block);
                runtime.WriteWord(object + 20u, block + 4096u);
                runtime.WriteWord(object + 24u, block);
                runtime.WriteWord(object + 28u, next_map);
            }
        }
    }
    return 0u;
}

std::uint32_t AnimationEventQueue::Destroy(
    const std::uint32_t object, IAnimationEventQueueRuntime& runtime) {
    (void)Clear(object, runtime);
    const std::uint32_t result =
        runtime.Free(runtime.ReadWord(object + 4u));
    runtime.WriteWord(object + 4u, 0u);
    runtime.WriteWord(object + 8u, 0u);
    return result;
}

std::uint32_t AnimationEventQueue::Add(
    const std::uint32_t object, const std::uint8_t owned,
    const std::uint32_t animation, IAnimationEventQueueRuntime& runtime) {
    const std::uint32_t head = runtime.ReadWord(object + 4u);
    std::uint32_t previous = runtime.ReadWord(head + 4u);
    const std::uint32_t node = runtime.Allocate(16u);
    runtime.WriteWord(node, head);
    if (previous == 0u) previous = node;
    runtime.WriteWord(node + 4u, previous);
    runtime.WriteWord(head + 4u, node);
    runtime.WriteWord(previous, node);
    runtime.WriteByte(node + 8u, owned);
    runtime.WriteWord(node + 12u, animation);
    runtime.WriteWord(object + 8u,
                      runtime.ReadWord(object + 8u) + 1u);
    return node + 8u;
}

std::uint32_t AnimationEventQueue::AddWithDelay(
    const std::uint32_t object, const std::uint8_t owned,
    const std::uint32_t animation, IAnimationEventQueueRuntime& runtime) {
    if (runtime.ReadWord(object + 8u) != 0u) {
        const std::uint32_t head = runtime.ReadWord(object + 4u);
        const std::uint32_t last = runtime.ReadWord(head + 4u);
        const std::uint32_t previous = runtime.ReadWord(last + 12u);
        const float interval =
            std::bit_cast<float>(runtime.ReadWord(object + 60u));
        const float elapsed =
            std::bit_cast<float>(runtime.ReadWord(previous + 48u));
        runtime.WriteWord(animation + 52u,
                          std::bit_cast<std::uint32_t>(interval - elapsed));
    }
    return Add(object, owned, animation, runtime);
}

std::uint32_t AnimationEventQueue::Update(
    const std::uint32_t object, const float delta,
    IAnimationEventQueueRuntime& runtime) {
    const std::uint32_t head = runtime.ReadWord(object + 4u);
    std::uint32_t result = head;
    std::uint32_t node = runtime.ReadWord(head);
    while (node != head) {
        const std::uint32_t animation = runtime.ReadWord(node + 12u);
        const float delay =
            std::bit_cast<float>(runtime.ReadWord(animation + 52u));
        // IDA4C3189 fcomp0 / fnstsw / and4100 / jnz: less, equal and
        // unordered all enter the active branch, including positive/negative0.
        if (std::isnan(delay) || delay <= 0.0f) {
            const float elapsed =
                std::bit_cast<float>(runtime.ReadWord(animation + 48u));
            runtime.WriteWord(animation + 48u,
                              std::bit_cast<std::uint32_t>(elapsed + delta));
            runtime.AdvanceAnimation(animation, delta);
            result = runtime.AnimationFinished(animation);
            if ((result & 0xffu) != 0u) {
                const std::uint32_t next = runtime.ReadWord(node);
                result = RemoveNode(node, runtime);
                runtime.WriteWord(object + 8u,
                                  runtime.ReadWord(object + 8u) - 1u);
                node = next;
                continue;
            }
        } else {
            runtime.WriteWord(animation + 52u,
                              std::bit_cast<std::uint32_t>(delay - delta));
            result = 0u;
        }
        node = runtime.ReadWord(node);
    }
    return result;
}

std::uint32_t AnimationEventQueue::Dispatch(
    const std::uint32_t object, const std::uint32_t argument,
    IAnimationEventQueueRuntime& runtime) {
    const std::uint32_t head = runtime.ReadWord(object + 4u);
    std::uint32_t node = runtime.ReadWord(head);
    while (node != runtime.ReadWord(object + 4u)) {
        runtime.CallAnimationEvent(runtime.ReadWord(node + 12u), argument);
        node = runtime.ReadWord(node);
    }
    return runtime.ReadWord(object + 4u);
}

std::uint32_t AnimationEventQueue::DestroyList(
    const std::uint32_t object, IAnimationEventQueueRuntime& runtime) {
    const std::uint32_t head = runtime.ReadWord(object + 4u);
    std::uint32_t node = runtime.ReadWord(head);
    while (node != head) {
        const std::uint32_t next = runtime.ReadWord(node);
        (void)RemoveNode(node, runtime);
        runtime.WriteWord(object + 8u,
                          runtime.ReadWord(object + 8u) - 1u);
        node = next;
    }
    (void)runtime.Free(head);
    runtime.WriteWord(object + 4u, 0u);
    runtime.WriteWord(object + 8u, 0u);
    return 0u;
}

std::uint32_t AnimationEventQueue::EraseAt(
    const std::uint32_t object, const std::uint32_t cursor_address,
    const std::uint32_t node, IAnimationEventQueueRuntime& runtime) {
    const std::uint32_t next = runtime.ReadWord(node);
    (void)RemoveNode(node, runtime);
    runtime.WriteWord(object + 8u,
                      runtime.ReadWord(object + 8u) - 1u);
    runtime.WriteWord(cursor_address, next);
    return cursor_address;
}

std::uint32_t AnimationEventQueue::AdvanceCursor(
    const std::uint32_t cursor_address,
    const std::uint32_t output_address,
    IAnimationEventQueueRuntime& runtime) {
    const std::uint32_t old = runtime.ReadWord(cursor_address);
    runtime.WriteWord(cursor_address, runtime.ReadWord(old));
    runtime.WriteWord(output_address, old);
    return output_address;
}

}  // namespace pal4
