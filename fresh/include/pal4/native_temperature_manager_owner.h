#pragma once

#include "pal4/native_base_object_memory.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace pal4 {

class NativeFloorAttributeOwner;
class IUiFrameEventRuntime;

// Owns the 44-byte palTemperatureGame image and its floor color callbacks.
class NativeTemperatureManagerOwner final {
public:
    explicit NativeTemperatureManagerOwner(NativeFloorAttributeOwner& floor);
    ~NativeTemperatureManagerOwner();
    NativeTemperatureManagerOwner(const NativeTemperatureManagerOwner&) = delete;
    NativeTemperatureManagerOwner& operator=(
        const NativeTemperatureManagerOwner&) = delete;

    [[nodiscard]] std::uint32_t Construct();
    // IDA518A63/5B4840: hide temperature UI and disable without resetting state.
    void Disable(IUiFrameEventRuntime& frames);
    void Destroy();
    [[nodiscard]] std::uint32_t Address() const noexcept { return object_; }
    [[nodiscard]] std::uint8_t Enabled() const;
    [[nodiscard]] std::uint32_t State();
    [[nodiscard]] std::uint32_t Color(std::uint32_t index);
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept {
        return memory_.OutstandingAllocations();
    }

private:
    class Runtime;
    NativeFloorAttributeOwner& floor_;
    NativeBaseObjectMemory memory_;
    std::unique_ptr<Runtime> runtime_;
    std::uint32_t object_ = 0u;
};

}  // namespace pal4
