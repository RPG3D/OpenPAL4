#include "pal4/scene_evf_record.h"

#include <cstring>

namespace pal4 {

void SceneEvfRecord::ClearLinkedList(SceneEvfRecordState& state,
                                     ISceneEvfListRuntime& runtime) {
    const auto head = state.words[39u];
    auto node = runtime.ReadListWord(head);
    while (node != head) {
        const auto next = runtime.ReadListWord(node);
        const auto previous = runtime.ReadListWord(node + 4u);
        runtime.WriteListWord(previous, next);
        runtime.WriteListWord(next + 4u, previous);
        runtime.FreeListNode(node);
        --state.words[40u];
        node = next;
    }
}

std::uint32_t SceneEvfRecord::SetAtomicRenderCallback(
    const std::uint32_t atomic, ISceneEvfAtomicRuntime& runtime) {
    const auto identity = runtime.IdentityRenderCallback();
    runtime.WriteCallback(atomic, 72u, identity);
    if (identity == 0u)
        runtime.WriteCallback(atomic, 72u, runtime.DefaultRenderCallback());
    return atomic;
}

std::uint32_t SceneEvfRecord::PropertyAddress(
    const SceneEvfRecordState& state, const std::uint32_t index) {
    const auto begin = state.words[21u];
    if (begin != 0u && index < (state.words[22u] - begin) / 80u)
        return begin + 80u * index;
    return 0u;
}

std::uint32_t SceneEvfRecord::UpdateVertexColors(
    SceneEvfRecordState& state, const std::uint32_t base_address,
    const bool alternate, ISceneEvfColorRuntime& runtime) {
    const auto color_offset = alternate ? 35u : 34u;
    state.words[33u] = base_address + 4u * color_offset;
    const auto rgba = state.words[color_offset];
    const auto argb = (rgba & 0xff000000u) |
                      ((rgba & 0x000000ffu) << 16u) |
                      (rgba & 0x0000ff00u) |
                      ((rgba & 0x00ff0000u) >> 16u);
    const auto count = state.words[32u];
    for (std::uint32_t i = 0u; i < count; ++i)
        runtime.WriteVertexColor(state.words[31u], i, argb);
    return count;
}
namespace {

void WriteByte(SceneEvfRecordState& state,
               const std::uint32_t offset, const std::uint8_t value) {
    auto& word = state.words[offset / 4u];
    const auto shift = 8u * (offset % 4u);
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

}  // namespace

SceneEvfRecordState& SceneEvfRecord::Initialize(
    SceneEvfRecordState& state, const std::uint32_t base_address,
    const std::uint32_t owner, ISceneEvfRecordRuntime& runtime) {
    runtime.InitializeEmptyString(state, 8u);
    runtime.InitializeEmptyString(state, 28u);
    runtime.InitializeEmptyString(state, 40u);
    runtime.InitializeEmptyString(state, 52u);
    state.words[1u] = 0xffffffffu;
    state.words[5u] = 0u;
    state.words[6u] = 0u;
    WriteByte(state, 80u, static_cast<std::uint8_t>(owner));
    state.words[21u] = 0u;
    state.words[22u] = 0u;
    state.words[23u] = 0u;
    runtime.InitializeEmptyString(state, 104u);
    WriteByte(state, 152u, static_cast<std::uint8_t>(owner));
    const auto list_head = runtime.Allocate(12u);
    runtime.WriteWord(list_head, list_head);
    runtime.WriteWord(list_head + 4u, list_head);
    state.words[39u] = list_head;
    state.words[40u] = 0u;
    for (const auto offset : {96u, 100u, 116u, 120u, 124u, 128u,
                              144u, 148u, 164u, 168u, 64u, 68u})
        state.words[offset / 4u] = 0u;
    WriteByte(state, 138u, 0xc8u);
    WriteByte(state, 141u, 0xc8u);
    WriteByte(state, 136u, 0x9bu);
    WriteByte(state, 139u, 0x95u);
    WriteByte(state, 143u, 0x95u);
    state.words[33u] = base_address + 136u;
    WriteByte(state, 137u, 0x37u);
    WriteByte(state, 140u, 0x37u);
    WriteByte(state, 142u, 0x32u);
    state.words[18u] = owner;
    state.words[19u] = 1u;
    return state;
}

int SceneEvfRecord::Cleanup(
    SceneEvfRecordState& state,
    ISceneEvfRecordCleanupRuntime& runtime) {
    ClearLinkedList(state, runtime);
    const auto begin = state.words[21u];
    runtime.ClearBufferRange(begin, state.words[22u]);
    state.words[22u] = begin;

    const auto property_array = state.words[24u];
    state.words[25u] = 0u;
    if (property_array != 0u) {
        const auto count = runtime.ReadWord(property_array - 4u);
        for (auto index = count; index != 0u; --index) {
            auto& property = runtime.PropertyAt(
                property_array + (index - 1u) * 80u);
            EffectPropertyLifecycle::Clear(property);
        }
        runtime.Free(property_array - 4u);
        state.words[24u] = 0u;
    }
    for (const auto offset : {116u, 124u, 164u}) {
        const auto pointer = state.words[offset / 4u];
        state.words[offset / 4u + 1u] = 0u;
        if (pointer != 0u) {
            runtime.Free(pointer);
            state.words[offset / 4u] = 0u;
        }
    }

    const auto frame = state.words[16u];
    if (frame != 0u) {
        runtime.DestroyFrame(frame, 1u);
        state.words[16u] = 0u;
    }
    const auto clump = state.words[17u];
    if (clump != 0u) {
        const auto world = runtime.ClumpWorld(clump);
        if (world != 0u) runtime.RemoveClump(world, clump);
        runtime.DestroyClump(clump);
        state.words[17u] = 0u;
    }
    state.words[18u] = 0u;

    const auto list_head = state.words[39u];
    auto node = runtime.ReadWord(list_head);
    while (node != list_head) {
        const auto current = node;
        node = runtime.ReadWord(node);
        runtime.RemoveListElement(list_head, current);
    }
    runtime.Free(list_head);
    state.words[39u] = 0u;
    state.words[40u] = 0u;

    (void)runtime.CleanupString(state, 104u);
    runtime.Free(begin);
    state.words[21u] = 0u;
    state.words[22u] = 0u;
    state.words[23u] = 0u;
    return CleanupEmbeddedStrings(state, runtime);
}

int SceneEvfRecord::CleanupEmbeddedStrings(
    SceneEvfRecordState& state, ISceneEvfRecordCleanupRuntime& runtime) {
    (void)runtime.CleanupString(state, 52u);
    (void)runtime.CleanupString(state, 40u);
    (void)runtime.CleanupString(state, 28u);
    return runtime.CleanupString(state, 8u);
}

std::uint32_t SceneEvfRecord::SetName(
    SceneEvfRecordState& state, const char* name,
    ISceneEvfRecordStringRuntime& runtime) {
    return runtime.SetString(state, 104u,
                             std::string_view{name, std::strlen(name)});
}

std::uint8_t SceneEvfRecord::SetTypeAndColor(
    SceneEvfRecordState& state, const std::uint32_t base_address,
    const std::uint32_t type, ISceneEvfRecordTypeRuntime& runtime) {
    const auto old = state.words[16u];
    state.words[1u] = type;
    if (old != 0u) {
        runtime.DestroyOld(old, 1u);
        state.words[16u] = 0u;
    }
    auto result = runtime.FindFactory(state.words[18u], type);
    if (result != 0u) {
        result = runtime.Create(result);
        state.words[16u] = result;
        if (result != 0u)
            runtime.BindOwnerField(result, base_address + 4u);
    }
    constexpr std::uint8_t colors[6u][4u]{
        {0xc8u, 20u, 20u, 0x95u},
        {0x9fu, 0x9fu, 20u, 0x95u},
        {50u, 0xdeu, 0xe2u, 0x95u},
        {0xb3u, 85u, 0xbfu, 0x95u},
        {70u, 63u, 0xd6u, 0x95u},
        {0xd9u, 108u, 0u, 0x95u}};
    if (type < 6u) {
        for (std::uint32_t index = 0u; index < 4u; ++index)
            WriteByte(state, 136u + index, colors[type][index]);
    }
    if (type == 0u) return 20u;
    if (type == 1u) return 0x9fu;
    return static_cast<std::uint8_t>(result);
}

}  // namespace pal4
