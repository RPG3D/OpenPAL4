#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IEventSubscriptionRuntime {
public:
    virtual ~IEventSubscriptionRuntime() = default;
    // Native host routers retire their owned tokens without legacy tree images.
    [[nodiscard]] virtual bool TryClearOwnedSlots(std::uint32_t,std::uint32_t,std::uint32_t) { return false; }
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t EventKey(
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t EventRouter() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t router, std::uint32_t mode,
        std::uint32_t key, std::uint32_t owner,
        std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t FindEventGroup(
        std::uint32_t router, std::uint32_t key) = 0;
    virtual void ReleaseOwnedSlot(std::uint32_t slot) = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
};

class EventSubscriptionSet final {
public:
    static void Clear(std::uint32_t owner, std::uint32_t first_offset,
                      std::uint32_t count,
                      IEventSubscriptionRuntime& runtime);
};

}  // namespace pal4
