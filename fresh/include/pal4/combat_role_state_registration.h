#pragma once

#include "pal4/event_subscription_set.h"

#include <array>
#include <cstdint>

namespace pal4 {

class CombatRoleStateSubscriptions final {
public:
    static constexpr std::uint32_t kCount = 9u;

    // IDA 0x54f410/0x54f550; the nine handle/key pairs begin at +40.
    [[nodiscard]] static std::uint32_t Subscribe(
        std::uint32_t window, IEventSubscriptionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Clear(
        std::uint32_t window, IEventSubscriptionRuntime& runtime);
};

class ICombatRoleStateFactoryRuntime {
public:
    virtual ~ICombatRoleStateFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructRoleState(
        std::uint32_t storage, std::uint32_t layout_name) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t InsertFactory(
        std::uint32_t factory_tree, std::uint32_t create_callback) = 0;
    virtual void InsertEvent(std::uint32_t event_tree,
                             std::uint32_t event_key,
                             std::uint32_t& callback_record) = 0;
    virtual void DeleteCallback(std::uint32_t callback_record,
                                 std::uint32_t flags) = 0;
};

class CombatRoleStateFactory final {
public:
    static constexpr std::uint32_t kFactorySource = 0x8b6918u;
    static constexpr std::uint32_t kFactoryCache = 0x8f95b0u;
    static constexpr std::uint32_t kCreateCallback = 0x54f2d0u;
    static constexpr std::uint32_t kEventKey = 0x8b68ecu;
    static constexpr std::uint32_t kEventCallbackVtable = 0x843494u;
    static constexpr std::uint32_t kEventSentinel = 0x8f95acu;

    // IDA 0x54f2b0-0x54f370.
    [[nodiscard]] static std::uint32_t GetInstance(
        ICombatRoleStateFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetInstanceThunk(
        ICombatRoleStateFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t layout_name, ICombatRoleStateFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactory(
        ICombatRoleStateFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactoryThunk(
        ICombatRoleStateFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterEvent(
        ICombatRoleStateFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterEventThunk(
        ICombatRoleStateFactoryRuntime& runtime);
};

}  // namespace pal4
