#pragma once

#include "pal4/native_base_object_memory.h"
#include "pal4/trade_system_lifecycle.h"

namespace pal4 {
// The original 0x54-byte palTradeSystem. Construct leaves word0 untouched;
// SetTradeItemInfo/ClearTradeItemInfo/OpenShop establish its state before use.
class NativeTradeSystemOwner final : private ITradeSystemLifecycleRuntime {
public:
    NativeTradeSystemOwner() = default;
    NativeTradeSystemOwner(const NativeTradeSystemOwner&) = delete;
    NativeTradeSystemOwner& operator=(const NativeTradeSystemOwner&) = delete;
    ~NativeTradeSystemOwner();
    std::uint32_t Construct();
    void Destroy();
    std::uint32_t Address() const noexcept { return address_; }
    std::uint32_t& State();
    std::uint32_t ReadWord(std::uint32_t offset);
    std::size_t OutstandingAllocations() const noexcept { return memory_.OutstandingAllocations(); }
private:
    std::uint32_t Allocate(std::uint32_t bytes) override;
    void Clear(std::uint32_t address, std::uint32_t bytes) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    NativeBaseObjectMemory memory_;
    std::uint32_t address_{};
};
}
