#pragma once

#include "pal4/event_subscription_set.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatEndingWindowConstructorRuntime : public IEventSubscriptionRuntime {
public:
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void LoadLayout(std::uint32_t window, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t CreateListNode() = 0;
    virtual void ConstructStream(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FindImage(
        std::string_view imageset, std::string_view image) = 0;
    [[nodiscard]] virtual std::uint32_t FindIcon(std::uint32_t index,
                                                  std::uint32_t variant,
                                                  std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateHint(std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructHint(
        std::uint32_t address, std::string_view name) = 0;
    virtual void FreeHint(std::uint32_t address) = 0;
    virtual void SetupElements(std::uint32_t window) = 0;
};

class CombatEndingWindowConstructor final {
public:
    static constexpr std::uint32_t kObjectSize = 0x2d0u;
    static constexpr std::uint32_t kVtable = 0x845370u;
    static constexpr std::uint32_t kEmbeddedVtable = 0x845358u;
    static constexpr std::uint32_t kHintSize = 0x48u;

    // IDA 0x5433a0. Addresses and native framework objects are runtime-owned.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t window, std::string_view name,
        ICombatEndingWindowConstructorRuntime& runtime);
};

}  // namespace pal4
