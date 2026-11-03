#include "pal4/callback_collection_cursor.h"

#include <cstdint>

namespace pal4 {

std::uint32_t CallbackCollectionCursor::SetInternalPointers(
    CallbackCollectionState& collection,
    ICallbackCollectionCursorRuntime& runtime) {
    collection.Write(0x0c, 0);
    const auto sentinel = collection.Read(0x24);
    const auto first = runtime.ReadDword(sentinel);
    const auto count = collection.Read(0x18);
    collection.Write(0x20, first);
    collection.Write(0x14, count);
    if (count == 0) return 0;
    return runtime.ReadDword(first + 8);
}

std::uint32_t CallbackCollectionCursor::InitializeReverse(
    CallbackCollectionState& collection,
    ICallbackCollectionCursorRuntime& runtime) {
    collection.Write(0x0c, 0);
    const auto sentinel = collection.Read(0x24);
    const auto last = runtime.ReadDword(sentinel + 4);
    collection.Write(0x20, last);
    if (collection.Read(0x18) == 0) {
        collection.Write(0x14, 0);
        return 0;
    }
    collection.Write(0x14, 1);
    return runtime.ReadDword(last + 8);
}

std::uint32_t CallbackCollectionCursor::Advance(
    CallbackCollectionState& collection,
    ICallbackCollectionCursorRuntime& runtime) {
    collection.Write(0x0c, 0);
    const auto current = collection.Read(0x20);
    const auto next = runtime.ReadDword(current + 4);
    const auto sentinel = collection.Read(0x24);
    collection.Write(0x20, next);
    collection.Write(0x14, collection.Read(0x14) + 1U);
    if (next == sentinel) {
        collection.Write(0x14, 0);
        return 0;
    }
    return runtime.ReadDword(next + 8);
}

std::uint32_t CallbackCollectionCursor::GetElementByIndexSafe(
    CallbackCollectionState& collection, const std::int32_t index,
    ICallbackCollectionCursorRuntime& runtime) {
    collection.Write(0x0c, 0);
    const auto count = static_cast<std::int32_t>(collection.Read(0x18));
    if (index >= count) return 0;
    return GetElementByIndex(collection, index, count, runtime);
}

std::uint32_t CallbackCollectionCursor::GetElementByIndex(
    CallbackCollectionState& collection, const std::int32_t index,
    const std::int32_t observed_count,
    ICallbackCollectionCursorRuntime& runtime) {
    collection.Write(0x0c, 0);
    if (index < 0 || index >= observed_count) return 0;

    auto payload = InitializeReverse(collection, runtime);
    while (payload != 0) {
        const auto one_based_index = collection.Read(0x14);
        collection.Write(0x0c, 0);
        if (static_cast<std::int32_t>(one_based_index) - 1 == index) {
            return payload;
        }
        payload = Advance(collection, runtime);
    }
    return 0;
}

void CallbackCollectionCursor::Clear(
    CallbackCollectionState& collection,
    ICallbackCollectionCursorRuntime& runtime) {
    if (static_cast<std::int32_t>(collection.Read(0x18)) <= 0) return;

    static_cast<void>(InitializeReverse(collection, runtime));
    const auto sentinel = collection.Read(0x24);
    auto current = collection.Read(0x20);
    while (current != sentinel) {
        const auto next = runtime.ReadDword(current + 4);
        collection.Write(0x20, next);
        if (current != 0) {
            const auto object = runtime.ReadDword(current + 8);
            const auto table = runtime.ReadDword(current + 0x0c);
            runtime.InvokeObject(table + 4, object);
            runtime.Free(current);
        }
        current = collection.Read(0x20);
    }

    runtime.WriteDword(sentinel, sentinel);
    runtime.WriteDword(sentinel + 4, sentinel);
    collection.Write(0x1c, 0);
    collection.Write(0x18, 0);
    collection.Write(0x14, 0);
    collection.Write(0x10, 0);
}

void CallbackCollectionCursor::DestroyStorage(
    CallbackCollectionState& collection,
    std::uint32_t& trailing_owned_object,
    ICallbackCollectionCursorRuntime& runtime) {
    static_cast<void>(InitializeReverse(collection, runtime));
    auto current = collection.Read(0x20);
    const auto sentinel = collection.Read(0x24);
    while (current != sentinel) {
        const auto next = runtime.ReadDword(current + 4);
        collection.Write(0x20, next);
        if (current != 0) {
            const auto object = runtime.ReadDword(current + 8);
            const auto table = runtime.ReadDword(current + 0x0c);
            runtime.InvokeObject(table + 4, object);
            runtime.Free(current);
        }
        current = collection.Read(0x20);
    }

    if (sentinel != 0) {
        const auto object = runtime.ReadDword(sentinel + 8);
        const auto table = runtime.ReadDword(sentinel + 0x0c);
        runtime.InvokeObject(table + 4, object);
        runtime.Free(sentinel);
    }

    if (trailing_owned_object != 0) {
        runtime.Free(trailing_owned_object);
        trailing_owned_object = 0;
    }
}

void CallbackCollectionCursor::Destroy(
    CallbackCollectionState& collection,
    std::uint32_t& trailing_owned_object,
    ICallbackCollectionCursorRuntime& runtime) {
    try {
        Clear(collection, runtime);
    } catch (...) {
        DestroyStorage(collection, trailing_owned_object, runtime);
        throw;
    }
    DestroyStorage(collection, trailing_owned_object, runtime);
}

}  // namespace pal4
