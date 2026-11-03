#pragma once

#include "pal4/event_handler_array.h"
#include "pal4/native_base_object_memory.h"

#include <array>
#include <cstdint>
#include <functional>

namespace pal4 {

// Owns array storage, but borrows handlers. Dispatchers resolve rewritten game
// methods; original EXE vtable addresses must never be called on host objects.
class NativeEventHandlerArray final : private IEventHandlerArrayRuntime {
public:
    using Virtual = std::function<std::uint32_t(std::uint32_t, std::uint32_t)>;
    using Predicate = std::function<std::uint8_t(
        std::uint32_t, std::uint32_t, std::uint32_t)>;

    NativeEventHandlerArray(Virtual invoke, Predicate predicate);
    ~NativeEventHandlerArray() override = default;
    NativeEventHandlerArray(const NativeEventHandlerArray&) = delete;
    NativeEventHandlerArray& operator=(const NativeEventHandlerArray&) = delete;

    [[nodiscard]] std::uint32_t Insert(std::uint32_t handler);
    [[nodiscard]] std::uint32_t Remove(std::uint32_t handler);
    [[nodiscard]] bool Contains(std::uint32_t event);
    [[nodiscard]] std::uint32_t GetLast();
    // Borrow the live array prefix for original global input routing. No copy
    // of begin/end is retained across callbacks that can change the activity.
    [[nodiscard]] std::uint32_t Address() const noexcept;
    [[nodiscard]] std::uint8_t DispatchKeyDown(std::uint32_t handler,
                                              std::uint32_t event);
    void Update();
    // Shutdown releases only the pointer buffer, never borrowed game handlers.
    void ReleaseStorage();
    [[nodiscard]] std::uint32_t Size() const noexcept;
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept {
        return memory_.OutstandingAllocations();
    }

private:
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t Allocate(std::uint32_t bytes) override;
    void Free(std::uint32_t address) override;
    std::uint32_t InvokeVirtual(std::uint32_t object, std::uint32_t slot) override;
    std::uint8_t InvokePredicate(std::uint32_t object, std::uint32_t slot,
                                 std::uint32_t argument) override;

    std::array<std::uint32_t, 4> image_{};
    NativeBaseObjectMemory memory_;
    Virtual invoke_;
    Predicate predicate_;
};

}  // namespace pal4
