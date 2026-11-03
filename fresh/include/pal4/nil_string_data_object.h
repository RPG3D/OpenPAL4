#pragma once

#include "pal4/callback_collection_cursor.h"

namespace pal4 {

class INilStringDataRuntime : public ICallbackCollectionRuntime,
                              public ICallbackCollectionCursorRuntime {
public:
    ~INilStringDataRuntime() override = default;
};

class NilStringDataObject final {
public:
    [[nodiscard]] NilStringDataObject& Initialize(
        std::uint32_t self_address, INilStringDataRuntime& runtime);
    void Destroy(INilStringDataRuntime& runtime);

    [[nodiscard]] CallbackCollectionState& Collection() noexcept;
    [[nodiscard]] const CallbackCollectionState& Collection() const noexcept;
    [[nodiscard]] std::uint32_t TrailingOwnedObject() const noexcept;
    void SetTrailingOwnedObject(std::uint32_t address) noexcept;

    [[nodiscard]] static bool InitializeSentinel(
        CallbackCollectionState& collection, std::uint32_t self_address,
        ICallbackCollectionRuntime& runtime);

    static constexpr std::uint32_t kAllocateCallback = 0x401d40;
    static constexpr std::uint32_t kDestroyCallback = 0x401da0;
    static constexpr std::uint32_t kInitializeBuffersCallback = 0x401dc0;

private:
    CallbackCollectionState collection_{};
    std::uint32_t trailing_owned_object_{};
};

}  // namespace pal4
