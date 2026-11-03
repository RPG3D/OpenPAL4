#pragma once

#include "pal4/actor_message_adapter.h"
#include "pal4/native_base_object_memory.h"

#include <cstddef>
#include <cstdint>

namespace pal4 {

// Owns the Scene state's palActorMessageAdapter and its collection storage.
class NativeActorMessageOwner final : private IActorMessageAdapterRuntime {
public:
    NativeActorMessageOwner() = default;
    ~NativeActorMessageOwner();
    NativeActorMessageOwner(const NativeActorMessageOwner&) = delete;
    NativeActorMessageOwner& operator=(const NativeActorMessageOwner&) = delete;

    [[nodiscard]] std::uint32_t Construct();
    void Clear();
    void Destroy();
    [[nodiscard]] std::uint32_t Address() const noexcept { return object_; }
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept {
        return memory_.OutstandingAllocations();
    }

private:
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t bytes) override;
    void Free(std::uint32_t address) override;
    [[nodiscard]] ActorMessageAdapter& Access(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::uint32_t InvokeFactory(std::uint32_t table) override;
    [[nodiscard]] std::uint32_t InvokeInsert(
        std::uint32_t callback, std::uint32_t ecx,
        std::uint32_t first, std::uint32_t second) override;
    void InvokeObject(std::uint32_t callback,
                      std::uint32_t object) override;

    NativeBaseObjectMemory memory_;
    std::uint32_t object_ = 0u;
};

}  // namespace pal4
