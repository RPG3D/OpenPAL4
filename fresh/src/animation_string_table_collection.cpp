#include "pal4/animation_string_table_collection.h"

#include <algorithm>
#include <limits>

namespace pal4 {

std::uint32_t AnimationStringTableCollection::Clear(
    AnimationStringTableCollectionState& state,
    IAnimationStringTableCollectionRuntime& runtime) {
    const auto old_end = state.end;
    if (state.end >= state.begin &&
        (state.end - state.begin) % sizeof(std::uint32_t) == 0u) {
        for (auto slot = state.begin; slot != state.end;
             slot += sizeof(std::uint32_t)) {
            const auto table = runtime.ReadPointer(slot);
            if (table != 0) {
                (void)AnimationStringTable::Destroy(
                    table, runtime.TableRuntime());
                (void)runtime.TableRuntime().Free(table);
            }
        }
        state.end = state.begin;
    }
    return old_end;
}

std::uint32_t AnimationStringTableCollection::Release(
    AnimationStringTableCollectionState& state,
    IAnimationStringTableCollectionRuntime& runtime) {
    (void)Clear(state, runtime);
    const auto result = runtime.TableRuntime().Free(state.begin);
    state.begin = 0;
    state.end = 0;
    state.limit = 0;
    return result;
}

std::uint32_t AnimationStringTableCollection::Find(
    const AnimationStringTableCollectionState& state,
    const std::uint32_t name,
    IAnimationStringTableCollectionRuntime& runtime) {
    if (state.end < state.begin ||
        (state.end - state.begin) % sizeof(std::uint32_t) != 0u)
        return 0;
    auto& table_runtime = runtime.TableRuntime();
    for (auto slot = state.begin; slot != state.end;
         slot += sizeof(std::uint32_t)) {
        const auto table = runtime.ReadPointer(slot);
        if (table == 0) continue;
        const auto& first = table_runtime.FieldAt(table);
        const auto text = first.text != 0
                              ? first.text : table_runtime.EmptyText();
        if (table_runtime.CompareText(text, name) == 0) return table;
    }
    return 0;
}

bool AnimationStringTableCollection::LoadFromFile(
    AnimationStringTableCollectionState& state,
    const std::uint32_t path,
    IAnimationStringTableCollectionRuntime& runtime) {
    (void)Clear(state, runtime);
    const auto file = runtime.OpenFile(path);
    if (file == 0) return false;

    std::uint32_t magic = 0;
    runtime.ReadWord(file, magic);
    if (magic != 20070323u) return false;

    std::uint32_t count = 0;
    runtime.ReadWord(file, count);
    for (std::uint32_t index = 0; index < count; ++index) {
        const auto table = runtime.AllocateTable();
        if (table != 0) {
            const auto source = runtime.PrepareEmptyTableTemplate(path);
            if (source != 0) {
                (void)AnimationStringTable::CopyConstruct(
                    table, source, runtime.TableRuntime());
                runtime.ReleaseEmptyTableTemplate(source);
            }
        }
        (void)AnimationStringTable::LoadFromFile(
            table, file, runtime.TableRuntime());
        (void)Insert(state, state.end, 1u, table, runtime);
    }
    runtime.CloseFile(file);
    return true;
}

std::uint32_t AnimationStringTableCollection::Insert(
    AnimationStringTableCollectionState& state,
    const std::uint32_t position,
    const std::uint32_t count,
    const std::uint32_t value,
    IAnimationStringTableCollectionRuntime& runtime) {
    constexpr std::uint32_t stride = sizeof(std::uint32_t);
    if (state.begin > state.end || state.end > state.limit ||
        (state.begin == 0 && (state.end != 0 || state.limit != 0)) ||
        position < state.begin || position > state.end ||
        (state.end - state.begin) % stride != 0u ||
        (state.limit - state.begin) % stride != 0u ||
        (position - state.begin) % stride != 0u)
        return state.end;
    if (count == 0u) return state.end;

    const auto size = (state.end - state.begin) / stride;
    const auto spare = (state.limit - state.end) / stride;
    if (count > std::numeric_limits<std::uint32_t>::max() / stride - size)
        return state.end;
    const auto added_bytes = count * stride;

    if (spare >= count) {
        for (auto source = state.end; source != position;) {
            source -= stride;
            runtime.WritePointer(source + added_bytes,
                                 runtime.ReadPointer(source));
        }
        for (auto slot = position; slot != position + added_bytes;
             slot += stride)
            runtime.WritePointer(slot, value);
        state.end += added_bytes;
        return state.end;
    }

    const auto growth = std::max(size, count);
    if (growth > std::numeric_limits<std::uint32_t>::max() / stride - size)
        return state.end;
    const auto capacity = size + growth;
    const auto bytes = capacity * stride;
    const auto replacement = runtime.AllocatePointerArray(bytes);
    if (replacement == 0 || replacement >
                                std::numeric_limits<std::uint32_t>::max() - bytes)
        return state.end;
    const auto prefix = (position - state.begin) / stride;
    for (std::uint32_t index = 0; index < prefix; ++index)
        runtime.WritePointer(replacement + index * stride,
                             runtime.ReadPointer(state.begin + index * stride));
    for (std::uint32_t index = 0; index < count; ++index)
        runtime.WritePointer(replacement + (prefix + index) * stride,
                             value);
    for (std::uint32_t index = prefix; index < size; ++index)
        runtime.WritePointer(replacement + (index + count) * stride,
                             runtime.ReadPointer(state.begin + index * stride));
    (void)runtime.TableRuntime().Free(state.begin);
    state.begin = replacement;
    state.end = replacement + (size + count) * stride;
    state.limit = replacement + bytes;
    return state.end;
}

}  // namespace pal4
