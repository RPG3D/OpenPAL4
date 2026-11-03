#include "pal4/camera_visibility_callbacks.h"

#include "pal4/camera_render_pass.h"

#include <cstddef>

namespace pal4 {

std::uint32_t CameraVisibilityCallback::Process(
    const std::uint32_t sector, CameraRenderState& state,
    ICameraVisibilityRuntime& runtime) {
    auto candidate = sector;
    runtime.TraverseSectors(state.camera, candidate,
                            &CameraRenderPass::RemoveFromList);
    bool overlaps = candidate == 0u;
    if (overlaps) {
        for (std::uint32_t index = 0u; index < 3u; ++index) {
            const auto low = runtime.ReadFloat(sector + 108u + 4u * index);
            if (!(low < state.maximum[index])) {
                overlaps = false;
                break;
            }
            const auto high = runtime.ReadFloat(sector + 96u + 4u * index);
            if (!(high > state.minimum[index])) {
                overlaps = false;
                break;
            }
        }
    }
    if (overlaps) {
        const auto old = runtime.ReadWord(sector + 124u);
        runtime.WriteWord(sector + 124u, state.second_pipeline);
        try {
            runtime.ProcessGlobal();
        } catch (...) {
            runtime.WriteWord(sector + 124u, old);
            throw;
        }
        runtime.WriteWord(sector + 124u, old);
    } else {
        runtime.ProcessGlobal();
    }
    return sector;
}

namespace {

std::uint32_t ProcessAtomic(
    const std::uint32_t atomic, const bool set_bits,
    ICameraAtomicUserDataRuntime& runtime) {
    const auto geometry = runtime.ReadWord(atomic + 24u);
    const auto original_flags = runtime.ReadWord(geometry + 8u);
    const auto count = runtime.UsedCount(geometry);
    const auto new_flags = set_bits ? original_flags | 0x60u
                                    : original_flags & ~0x60u;
    for (std::int32_t index = 0; index < count; ++index) {
        const auto item = runtime.GetItem(geometry, index);
        if (runtime.ReadWord(item + 4u) != 3u) continue;
        const auto text = runtime.GetText(item, index);
        const auto temporary = runtime.InitializeString(text);
        std::uint32_t found;
        try {
            runtime.LowercaseString(temporary);
            found = runtime.FindClump(runtime.GetClumpManager(), temporary);
        } catch (...) {
            runtime.CleanupString(temporary);
            throw;
        }
        runtime.CleanupString(temporary);
        if (found != 0u) return atomic;
    }
    runtime.WriteWord(geometry + 8u, new_flags);
    return atomic;
}

}  // namespace

std::uint32_t CameraAtomicUserData::ClearUnmatchedFlags(
    const std::uint32_t atomic, ICameraAtomicUserDataRuntime& runtime) {
    return ProcessAtomic(atomic, false, runtime);
}

std::uint32_t CameraAtomicUserData::SetUnmatchedFlags(
    const std::uint32_t atomic, ICameraAtomicUserDataRuntime& runtime) {
    return ProcessAtomic(atomic, true, runtime);
}

}  // namespace pal4
