#include "pal4/config_composite.h"

#include <initializer_list>
#include <new>

namespace pal4 {

namespace {

std::uint32_t RemoveNode(ConfigCompositeState& state,
                         const std::uint32_t list_index,
                         const std::uint32_t node,
                         IConfigCompositeRuntime& runtime) {
    const auto next = runtime.ReadWord(node);
    const auto previous = runtime.ReadWord(node + 4u);
    runtime.WriteWord(previous, next);
    runtime.WriteWord(next + 4u, previous);
    (void)runtime.FreeNode(node);
    --state.words[list_index * 3u + 3u];
    return next;
}

void DrainList(ConfigCompositeState& state, const std::uint32_t list_index,
               IConfigCompositeRuntime& runtime) {
    const auto head = state.words[list_index * 3u + 2u];
    for (auto cursor = runtime.ReadWord(head); cursor != head;) {
        const auto next_cursor = runtime.ReadWord(cursor);
        (void)RemoveNode(state, list_index, cursor, runtime);
        cursor = next_cursor;
    }
}

std::uint32_t RegisterAtomic(
    ConfigCompositeState& state, const std::uint32_t list_index,
    const std::uint32_t atomic,
    const std::array<std::uint32_t, 5u>& properties,
    IConfigCompositeRuntime& runtime) {
    const auto head = state.words[list_index * 3u + 2u];
    for (auto node = runtime.ReadWord(head); node != head;
         node = runtime.ReadWord(node)) {
        if (runtime.ReadWord(node + 8u) == atomic) return atomic;
    }

    auto previous = runtime.ReadWord(head + 4u);
    const auto node = runtime.AllocateNode(32u);
    if (node == 0u) throw std::bad_alloc{};
    runtime.WriteWord(node, head);
    if (previous == 0u) previous = node;
    runtime.WriteWord(node + 4u, previous);
    runtime.WriteWord(head + 4u, node);
    runtime.WriteWord(previous, node);
    runtime.WriteWord(node + 8u, atomic);
    for (std::uint32_t index = 0u; index < properties.size(); ++index) {
        runtime.WriteWord(node + 12u + index * 4u, properties[index]);
    }
    ++state.words[list_index * 3u + 3u];
    return atomic;
}

}  // namespace

ConfigCompositeState& ConfigComposite::Initialize(
    ConfigCompositeState& state, const std::uint32_t native_object_address,
    IConfigCompositeRuntime& runtime) {
    const auto address_high_byte = native_object_address >> 24u;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto byte_word = index * 3u + 1u;
        state.words[byte_word] =
            (state.words[byte_word] & 0xffffff00u) | address_high_byte;
        const auto node = runtime.AllocateNode(32u);
        if (node == 0u) throw std::bad_alloc{};
        runtime.WriteWord(node, node);
        runtime.WriteWord(node + 4u, node);
        state.words[byte_word + 1u] = node;
        state.words[byte_word + 2u] = 0u;
    }
    state.words[13u] = 0u;
    for (std::uint32_t index = 20u; index <= 26u; ++index) {
        state.words[index] = 0u;
    }
    state.words[0u] = kVtable;
    return state;
}

std::uint32_t ConfigComposite::CleanupResources(
    ConfigCompositeState& state, IConfigCompositeRuntime& runtime) {
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        DrainList(state, index, runtime);
    }
    DrainList(state, 3u, runtime);

    for (const auto index : {13u, 20u}) {
        if (state.words[index] != 0u) {
            (void)runtime.DestroyClump(state.words[index]);
            state.words[index] = 0u;
        }
    }
    auto result = state.words[21u];
    if (result != 0u) {
        result = runtime.DestroyClump(result);
        state.words[21u] = 0u;
    }
    for (std::uint32_t index = 23u; index <= 26u; ++index) {
        state.words[index] = 0u;
    }
    return result;
}

std::uint32_t ConfigComposite::CleanupFull(
    ConfigCompositeState& state, IConfigCompositeRuntime& runtime) {
    state.words[0u] = kVtable;
    (void)CleanupResources(state, runtime);
    for (std::uint32_t index = 4u; index-- > 0u;) {
        DrainList(state, index, runtime);
        const auto head_index = index * 3u + 2u;
        const auto result = runtime.FreeNode(state.words[head_index]);
        state.words[head_index] = 0u;
        state.words[head_index + 1u] = 0u;
        if (index == 0u) return result;
    }
    return 0u;
}

std::uint32_t ConfigComposite::LoadFloorClump(
    ConfigCompositeState& state, const char* source,
    IConfigCompositeRuntime& runtime) {
    const auto texture_context = runtime.GameInstance() + 0x1acu;
    const auto clump = runtime.LoadClump(source, 0u, texture_context);
    state.words[13u] = clump;
    if (clump != 0u) {
        runtime.AddClumpBounds(clump, 0u, state, 14u);
        (void)runtime.ForEachAtomic(clump, 0x5ff9e0u, state);
    }
    return state.words[13u];
}

std::uint32_t ConfigComposite::LoadIntriClump(
    ConfigCompositeState& state, const char* source,
    IConfigCompositeRuntime& runtime) {
    const auto texture_context = runtime.GameInstance() + 0x1acu;
    const auto clump = runtime.LoadClump(source, 0u, texture_context);
    state.words[20u] = clump;
    if (clump != 0u) {
        (void)runtime.ForEachAtomic(clump, 0x5ffa80u, state);
    }
    return state.words[20u];
}

std::uint32_t ConfigComposite::LoadWallClump(
    ConfigCompositeState& state, const char* source,
    IConfigCompositeRuntime& runtime) {
    const auto texture_context = runtime.GameInstance() + 0x1acu;
    const auto clump = runtime.LoadClump(source, 0u, texture_context);
    state.words[21u] = clump;
    if (clump != 0u) {
        (void)runtime.ForEachAtomic(clump, 0x5ffb20u, state);
    }
    return state.words[21u];
}

std::uint32_t ConfigComposite::SetClumpProperties(
    const std::uint32_t native_object_address, const std::uint32_t clump,
    const std::uint32_t mode, const std::uint32_t flag1,
    const std::uint32_t flag2, const std::uint32_t callback,
    IConfigCompositeRuntime& runtime) {
    if (clump == 0u) return 0u;
    const std::array<std::uint32_t, 5u> context{
        native_object_address, mode, flag1, flag2, callback};
    return runtime.ForEachAtomicWithContext(clump, 0x5ffbc0u, context);
}

std::uint32_t ConfigComposite::UnregisterClumpAtomics(
    ConfigCompositeState& state, const std::uint32_t clump,
    IConfigCompositeRuntime& runtime) {
    if (clump == 0u) return 0u;
    const auto result = runtime.ForEachAtomic(clump, 0x5ffc60u, state);
    state.words[22u] = 1u;
    return result;
}

std::uint32_t ConfigComposite::CleanupList(
    ConfigCompositeState& state, const std::uint32_t list_index,
    IConfigCompositeRuntime& runtime) {
    DrainList(state, list_index, runtime);
    const auto head_index = list_index * 3u + 2u;
    (void)runtime.FreeNode(state.words[head_index]);
    state.words[head_index] = 0u;
    state.words[head_index + 1u] = 0u;
    return 0u;
}

std::uint32_t& ConfigComposite::RemoveListNode(
    ConfigCompositeState& state, const std::uint32_t list_index,
    const std::uint32_t node, std::uint32_t& next_cursor,
    IConfigCompositeRuntime& runtime) {
    next_cursor = RemoveNode(state, list_index, node, runtime);
    return next_cursor;
}

std::uint32_t ConfigComposite::RegisterFloorAtomic(
    ConfigCompositeState& state, const std::uint32_t atomic,
    IConfigCompositeRuntime& runtime) {
    return RegisterAtomic(state, 0u, atomic, {1u, 0u, 1u, 0u, 0u}, runtime);
}

std::uint32_t ConfigComposite::RegisterIntriAtomic(
    ConfigCompositeState& state, const std::uint32_t atomic,
    IConfigCompositeRuntime& runtime) {
    return RegisterAtomic(state, 1u, atomic, {1u, 0u, 1u, 0u, 0u}, runtime);
}

std::uint32_t ConfigComposite::RegisterWallAtomic(
    ConfigCompositeState& state, const std::uint32_t atomic,
    IConfigCompositeRuntime& runtime) {
    return RegisterAtomic(state, 2u, atomic, {1u, 0u, 1u, 0u, 0u}, runtime);
}

std::uint32_t ConfigComposite::RegisterConfiguredAtomic(
    ConfigCompositeState& state, const std::uint32_t atomic,
    const std::array<std::uint32_t, 5u>& context,
    IConfigCompositeRuntime& runtime) {
    return RegisterAtomic(state, 3u, atomic,
                          {context[1u], 0u, context[2u], context[3u],
                           context[4u]}, runtime);
}

std::uint32_t ConfigComposite::UnregisterAtomic(
    ConfigCompositeState& state, const std::uint32_t atomic,
    IConfigCompositeRuntime& runtime) {
    const auto head = state.words[11u];
    for (auto node = runtime.ReadWord(head); node != head;
         node = runtime.ReadWord(node)) {
        if (runtime.ReadWord(node + 8u) == atomic) {
            (void)RemoveNode(state, 3u, node, runtime);
            break;
        }
    }
    return atomic;
}

}  // namespace pal4
