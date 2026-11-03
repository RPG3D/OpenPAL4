#include "pal4/gom_task_base.h"

#include <bit>
#include <cstring>
#include <new>

namespace pal4 {

GomTaskBaseState& GomTaskBase::Construct(GomTaskBaseState& state) {
    state.vtable = kVtable;
    state.active = 0u;
    state.name = 0u;
    state.wait_mode = 0u;
    state.wait_time = 0.0f;
    state.expect_time = 0.0f;
    state.total_elapsed = 0.0f;
    state.wait_elapsed = 0.0f;
    state.run_elapsed = 0.0f;
    state.finish_latch = 0.0f;
    state.phase = 0u;
    state.reserved = 0u;
    state.previous = 0u;
    state.next = 0u;
    state.context = 0u;
    return state;
}

std::uint32_t GomTaskBase::Destroy(GomTaskBaseState& state) {
    state.vtable = kVtable;
    std::uint32_t result = 0u;
    if (state.name != 0u) {
        result = runtime_.FreeMemory(state.name);
        state.name = 0u;
    }
    state.previous = 0u;
    state.next = 0u;
    state.context = 0u;
    return result;
}

std::uint32_t GomTaskBase::DestroyDeleting(
    GomTaskBaseState& state, std::uint32_t object,
    std::uint32_t flags) {
    static_cast<void>(Destroy(state));
    if ((flags & 1u) != 0u)
        static_cast<void>(runtime_.FreeMemory(object));
    return object;
}

std::uint32_t GomTaskBase::InitializeTimingAndLinks(
    GomTaskBaseState& state, const char* name,
    const GomTaskTimingConfig& timing, std::uint32_t previous,
    std::uint32_t next, std::uint32_t context) {
    if (state.name != 0u) {
        static_cast<void>(runtime_.FreeMemory(state.name));
        state.name = 0u;
    }
    const auto bytes = static_cast<std::uint32_t>(std::strlen(name) + 1u);
    const auto allocated = runtime_.AllocateMemory(bytes);
    if (allocated == 0u) throw std::bad_alloc();
    state.name = allocated;
    runtime_.CopyCString(allocated, name, bytes);
    state.wait_mode = timing.wait_mode;
    state.wait_time = timing.wait_time;
    state.previous = previous;
    state.expect_time = timing.expect_time;
    state.next = next;
    state.context = context;
    state.total_elapsed = 0.0f;
    state.wait_elapsed = 0.0f;
    state.run_elapsed = 0.0f;
    state.finish_latch = 0.0f;
    state.active = 1u;
    return next;
}

void GomTaskBase::AccumulateTotal(GomTaskBaseState& state, float delta) {
    state.total_elapsed += delta;
}

void GomTaskBase::AccumulateWait(GomTaskBaseState& state, float delta) {
    state.wait_elapsed += delta;
}

void GomTaskBase::AccumulateRun(GomTaskBaseState& state, float delta) {
    state.run_elapsed += delta;
}

std::uint32_t GomTaskBase::LatchFinishTime(GomTaskBaseState& state) {
    const auto bits = std::bit_cast<std::uint32_t>(state.total_elapsed);
    state.finish_latch = std::bit_cast<float>(bits);
    return bits;
}

bool GomTaskBase::IsWaitComplete(const GomTaskBaseState& state) {
    if (state.phase != 2u) return false;
    switch (state.wait_mode) {
    case 1u:
        if (state.previous != 0u) {
            if (runtime_.ReadWord(state.previous + 0x28u) != 4u)
                return false;
            const auto finish = std::bit_cast<float>(
                runtime_.ReadWord(state.previous + 0x24u));
            return static_cast<double>(state.wait_elapsed) -
                       static_cast<double>(finish) >
                   static_cast<double>(state.wait_time);
        }
        [[fallthrough]];
    case 2u:
        return state.wait_elapsed >= state.wait_time;
    case 3u:
        return true;
    default:
        return false;
    }
}

bool GomTaskBase::IsDurationComplete(const GomTaskBaseState& state) {
    return state.expect_time >= 0.0f &&
           state.run_elapsed >= state.expect_time;
}

}  // namespace pal4
