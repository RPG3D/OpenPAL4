#pragma once

#include "pal4/animation_event_queue.h"
#include "pal4/combat_console_event_subscriptions.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatConsoleRuntime : public IAnimationEventQueueRuntime,
                              public ICombatConsoleEventRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) override = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) override = 0;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::string_view name,
        std::uint32_t id, std::uint32_t factory,
        std::uint32_t destroy_callback) = 0;
};

class CombatConsoleLifecycle final {
public:
    static constexpr std::uint32_t kObjectSize = 0xfcu;
    static constexpr std::uint32_t kSingletonId = 0x406u;
    static constexpr std::uint32_t kSingletonSlot = 0x8f94c8u;
    static constexpr std::uint32_t kSingletonGuard = 0x8f94ccu;
    static constexpr std::uint32_t kFactory = 0x53ff40u;
    static constexpr std::uint32_t kDestroyCallback = 0x53ffa0u;

    [[nodiscard]] static std::uint32_t Initialize(
        ICombatConsoleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateInstance(
        ICombatConsoleRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyInstance(
        std::uint32_t console, ICombatConsoleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t console, ICombatConsoleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t console, IAnimationEventQueueRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyQueueThunk(
        std::uint32_t console, ICombatConsoleRuntime& runtime);
    [[nodiscard]] static std::uint32_t SubscribeThunk(
        std::uint32_t console, ICombatConsoleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t console, ICombatConsoleRuntime& runtime);
};

}  // namespace pal4
