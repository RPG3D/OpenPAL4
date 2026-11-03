#pragma once

#include "pal4/event_subscription_set.h"

#include <cstdint>

namespace pal4 {

class ICombatMessageSubscriptionRuntime : public IEventSubscriptionRuntime {
public:
    [[nodiscard]] virtual std::uint32_t MessageEventKey() = 0;
};

class CombatMessageSubscriptions final {
public:
    static constexpr std::uint32_t kCallback = 0x54ef80u;

    // IDA 0x54e690 and 0x54e6c0; the subscription pair begins at +40.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t message, ICombatMessageSubscriptionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Clear(
        std::uint32_t message, ICombatMessageSubscriptionRuntime& runtime);
};

class ICombatMessageFactoryRuntime {
public:
    virtual ~ICombatMessageFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructMessage(
        std::uint32_t storage, std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t InsertFactory(
        std::uint32_t factory_tree, std::uint32_t create_callback) = 0;
    virtual void InsertEvent(std::uint32_t event_tree,
                             std::uint32_t event_key,
                             std::uint32_t& callback_record) = 0;
    virtual void DeleteCallback(std::uint32_t callback_record,
                                 std::uint32_t flags) = 0;
};

class CombatMessageFactory final {
public:
    static constexpr std::uint32_t kFactorySource = 0x8b6920u;
    static constexpr std::uint32_t kFactoryCache = 0x8f95a8u;
    static constexpr std::uint32_t kCreateCallback = 0x54e760u;
    static constexpr std::uint32_t kEventKey = 0x8b68f4u;
    static constexpr std::uint32_t kEventCallbackVtable = 0x8435dcu;
    static constexpr std::uint32_t kShowCallback = 0x54eef0u;
    static constexpr std::uint32_t kEventSentinel = 0x8f95a4u;

    [[nodiscard]] static std::uint32_t GetInstance(
        ICombatMessageFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetInstanceThunk(
        ICombatMessageFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t argument, ICombatMessageFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactory(
        ICombatMessageFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactoryThunk(
        ICombatMessageFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterEvent(
        ICombatMessageFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterEventThunk(
        ICombatMessageFactoryRuntime& runtime);
};

}  // namespace pal4
