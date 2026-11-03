#pragma once

#include "pal4/player_manager_object.h"
#include "pal4/player_manager_slot.h"

#include <cstdint>

namespace pal4 {

// Runs the four game slot constructors over caller-owned x86 manager storage.
// The original constructor reads an uninitialized stack byte; the native host
// uses a deterministic zero unless its caller supplies another value.
// A constructed manager must be destroyed before the slot runtime is released.
class NativePlayerManagerVectorRuntime final :
    public IPlayerManagerVectorRuntime {
public:
    explicit NativePlayerManagerVectorRuntime(
        IPlayerManagerSlotRuntime& slots, std::uint8_t initial_byte = 0u)
        : slots_(slots), initial_byte_(initial_byte) {}
    NativePlayerManagerVectorRuntime(
        const NativePlayerManagerVectorRuntime&) = delete;
    NativePlayerManagerVectorRuntime& operator=(
        const NativePlayerManagerVectorRuntime&) = delete;

    void ConstructVector(std::uint32_t object, std::uint32_t element_bytes,
                         std::uint32_t count, std::uint32_t constructor,
                         std::uint32_t destructor) override;
    void DestroyVector(std::uint32_t object, std::uint32_t element_bytes,
                       std::uint32_t count,
                       std::uint32_t destructor) override;

private:
    void DestroyConstructed();

    IPlayerManagerSlotRuntime& slots_;
    std::uint8_t initial_byte_{};
    std::uint32_t object_{};
    std::uint32_t constructed_{};
};

}  // namespace pal4
