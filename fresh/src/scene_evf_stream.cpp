#include "pal4/scene_evf_record.h"

#include <algorithm>
#include <bit>
#include <cstdint>

namespace pal4 {

std::int32_t SceneEvfRecord::PropertyBufferSize(
    const SceneEvfRecordState& state) {
    const auto begin = state.words[21u];
    if (begin == 0u) return 0;
    return std::bit_cast<std::int32_t>(state.words[22u] - begin) / 80;
}

void SceneEvfRecord::ClearPropertyRange(
    const std::uint32_t begin, const std::uint32_t end,
    ISceneEvfPropertyBufferRuntime& runtime) {
    for (auto address = begin; address != end; address += 80u)
        EffectPropertyLifecycle::Clear(runtime.PropertyAt(address));
}

std::uint32_t SceneEvfRecord::CopyPropertyRange(
    const std::uint32_t begin, const std::uint32_t end,
    std::uint32_t destination, ISceneEvfStreamRuntime& runtime) {
    for (auto source = begin; source != end; source += 80u) {
        if (destination != 0u)
            runtime.WriteProperty(destination, runtime.PropertyAt(source));
        destination += 80u;
    }
    return destination;
}

std::uint32_t SceneEvfRecord::FillPropertyRange(
    std::uint32_t destination, std::uint32_t count,
    const EffectPropertyState& property, ISceneEvfStreamRuntime& runtime) {
    if (count == 0u) return 0u;
    do {
        if (destination != 0u) runtime.WriteProperty(destination, property);
        destination += 80u;
    } while (--count != 0u);
    return destination;
}

std::uint32_t SceneEvfRecord::CopyPropertyRangeUnchecked(
    const std::uint32_t begin, const std::uint32_t end,
    std::uint32_t destination, ISceneEvfStreamRuntime& runtime) {
    for (auto source = begin; source != end; source += 80u) {
        runtime.WriteProperty(destination, runtime.PropertyAt(source));
        destination += 80u;
    }
    return destination;
}

int SceneEvfRecord::CleanupPropertyBuffer(
    SceneEvfRecordState& state, ISceneEvfPropertyBufferRuntime& runtime) {
    const auto begin = state.words[21u];
    ClearPropertyRange(begin, state.words[22u], runtime);
    runtime.FreePropertyBuffer(begin);
    state.words[21u] = 0u;
    state.words[22u] = 0u;
    state.words[23u] = 0u;
    return 0;
}

bool SceneEvfRecord::ReadFromStream(
    SceneEvfRecordState& state, const std::uint32_t base_address,
    const std::uint32_t stream, ISceneEvfStreamRuntime& stream_runtime,
    ISceneEvfRecordTypeRuntime& type_runtime,
    ISceneEvfGeometryRuntime& geometry_runtime,
    ISceneEvfAtomicRuntime& atomic_runtime) {
    if (stream == 0u) return false;

    const auto old_begin = state.words[21u];
    ClearPropertyRange(old_begin, state.words[22u], stream_runtime);
    state.words[22u] = old_begin;

    stream_runtime.ReadName(stream, state, 104u);
    state.words[0u] = stream_runtime.ReadWord(stream);
    stream_runtime.ReadEffectFields(stream, state);
    (void)SetTypeAndColor(state, base_address, state.words[1u], type_runtime);

    const auto property_count =
        static_cast<std::int32_t>(stream_runtime.ReadWord(stream));
    for (std::int32_t i = 0; i < property_count; ++i) {
        EffectPropertyState property{};
        (void)EffectPropertyLifecycle::Reset(property);
        stream_runtime.ReadProperty(stream, property);

        auto begin = state.words[21u];
        auto end = state.words[22u];
        const auto capacity_end = state.words[23u];
        if (capacity_end >= end && (capacity_end - end) / 80u != 0u) {
            (void)FillPropertyRange(end, 1u, property, stream_runtime);
            state.words[22u] = end + 80u;
        } else {
            const auto old_count =
                static_cast<std::uint32_t>(PropertyBufferSize(state));
            const auto new_capacity = old_count + std::max(old_count, 1u);
            const auto new_begin = stream_runtime.AllocatePropertyBytes(
                80u * new_capacity);
            const auto copied_end =
                CopyPropertyRange(begin, end, new_begin, stream_runtime);
            (void)FillPropertyRange(copied_end, 1u, property, stream_runtime);
            ClearPropertyRange(begin, end, stream_runtime);
            stream_runtime.FreePropertyBuffer(begin);
            state.words[21u] = new_begin;
            state.words[22u] = new_begin + 80u * (old_count + 1u);
            state.words[23u] = new_begin + 80u * new_capacity;
        }
        EffectPropertyLifecycle::Clear(property);
    }

    state.words[36u] = stream_runtime.ReadWord(stream);
    state.words[37u] = stream_runtime.ReadWord(stream);
    if (stream_runtime.FindClumpChunk(stream, 16u)) {
        const auto old_clump = state.words[17u];
        if (old_clump != 0u) {
            stream_runtime.RemoveClump(stream_runtime.ClumpWorld(old_clump),
                                       old_clump);
            stream_runtime.DestroyClump(old_clump);
        }
        const auto clump = stream_runtime.ReadClump(stream);
        state.words[17u] = clump;
        for (const auto atomic : stream_runtime.Atomics(clump))
            (void)SetAtomicRenderCallback(atomic, atomic_runtime);
    }

    switch (state.words[0u]) {
    case 0u:
        (void)GenerateStripGeometry(state, state.words[36u] != 0u,
                                    state.words[37u] != 0u, geometry_runtime);
        break;
    case 1u:
        (void)GeneratePolygonGeometry(state, geometry_runtime);
        break;
    case 2u:
        (void)GenerateVolumeGeometry(state, geometry_runtime);
        break;
    default:
        break;
    }
    return true;
}

}  // namespace pal4
