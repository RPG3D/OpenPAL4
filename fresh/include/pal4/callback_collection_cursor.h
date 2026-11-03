#pragma once

#include "pal4/callback_collection_element.h"

namespace pal4 {

class ICallbackCollectionCursorRuntime {
public:
    virtual ~ICallbackCollectionCursorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    virtual void InvokeObject(std::uint32_t table_address,
                              std::uint32_t object) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class CallbackCollectionCursor final {
public:
    [[nodiscard]] static std::uint32_t SetInternalPointers(
        CallbackCollectionState& collection,
        ICallbackCollectionCursorRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeReverse(
        CallbackCollectionState& collection,
        ICallbackCollectionCursorRuntime& runtime);
    [[nodiscard]] static std::uint32_t Advance(
        CallbackCollectionState& collection,
        ICallbackCollectionCursorRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetElementByIndexSafe(
        CallbackCollectionState& collection, std::int32_t index,
        ICallbackCollectionCursorRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetElementByIndex(
        CallbackCollectionState& collection, std::int32_t index,
        std::int32_t observed_count,
        ICallbackCollectionCursorRuntime& runtime);
    static void Clear(CallbackCollectionState& collection,
                      ICallbackCollectionCursorRuntime& runtime);
    static void DestroyStorage(CallbackCollectionState& collection,
                               std::uint32_t& trailing_owned_object,
                               ICallbackCollectionCursorRuntime& runtime);
    static void Destroy(CallbackCollectionState& collection,
                        std::uint32_t& trailing_owned_object,
                        ICallbackCollectionCursorRuntime& runtime);
};

}  // namespace pal4
