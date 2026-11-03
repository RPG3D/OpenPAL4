#pragma once

#include "pal4/event_subscription_set.h"

#include <cstdint>

namespace pal4 {

class ICombatActionConsoleWindowRuntime {
public:
    virtual ~ICombatActionConsoleWindowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void LoadLayout(std::uint32_t window, std::uint32_t name,
                            std::uint32_t first_flag,
                            std::uint32_t second_flag) = 0;
    virtual void ConstructNavigation(std::uint32_t navigation) = 0;
    virtual void ConstructMagicSelect(std::uint32_t member) = 0;
    virtual void ConstructPropertySelect(std::uint32_t member) = 0;
    virtual void ConstructStuntSelect(std::uint32_t member) = 0;
    virtual void SetupUIElements(std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateVisibility(
        std::uint32_t window, std::uint32_t first_flag,
        std::uint32_t second_flag) = 0;
    virtual void SetupMouseHandlers(std::uint32_t navigation,
                                    std::uint32_t button) = 0;
    virtual void DestroyStuntSelect(std::uint32_t member) = 0;
    virtual void DestroyPropertySelect(std::uint32_t member) = 0;
    virtual void DestroyMagicSelect(std::uint32_t member) = 0;
    virtual void CleanupNavigation(std::uint32_t navigation) = 0;
    [[nodiscard]] virtual std::uint32_t DestructLayout(
        std::uint32_t window) = 0;
    virtual void DestructSelectorLayout(std::uint32_t member) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    virtual void SetParent(std::uint32_t window, std::uint8_t active,
                           std::uint32_t parent) = 0;
};

class ICombatActionFrameServices {
public:
    virtual ~ICombatActionFrameServices()=default;
    virtual void SetParent(std::uint32_t,std::uint8_t,std::uint32_t)=0;
    virtual void RegisterEvents(std::uint32_t)=0;
    virtual void CleanupEvents(std::uint32_t)=0;
    virtual std::uint32_t UpdateVisibility(std::uint32_t,std::uint32_t,std::uint32_t)=0;
};

class CombatActionConsoleWindow final {
public:
    static constexpr std::uint32_t kSize = 0x1e8u;
    static constexpr std::uint32_t kVtable = 0x845338u;
    static constexpr std::uint32_t kNavigationVtable = 0x845310u;

    // IDA 0x5411c0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t window, std::uint32_t name,
        ICombatActionConsoleWindowRuntime& runtime);
    // IDA 0x541390/0x541430.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t window, ICombatActionConsoleWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetParentAndEvents(
        std::uint32_t window,std::uint8_t active,std::uint32_t parent,
        ICombatActionFrameServices& services);
    [[nodiscard]] static std::uint32_t SetParentAndEvents(
        std::uint32_t window, std::uint8_t active, std::uint32_t parent,
        ICombatActionConsoleWindowRuntime& runtime,
        IEventSubscriptionRuntime& event_runtime);
};

}  // namespace pal4
