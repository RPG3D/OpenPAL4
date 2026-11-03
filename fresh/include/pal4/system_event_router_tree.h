#pragma once

#include <cstdint>

namespace pal4 {

class ISystemEventRouterTreeRuntime {
public:
    virtual ~ISystemEventRouterTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) {
        return static_cast<std::uint8_t>(ReadWord(address));
    }
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CleanupPayload(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DeleteCallback(
        std::uint32_t address, std::uint32_t flags) = 0;
};

class ScriptEventPayload final {
public:
    // IDA 0x54e670.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t owner, std::uint32_t callback,
        std::uint32_t argument, std::uint32_t& callback_slot);
    // IDA 0x54e620; the virtual deleting destructor receives flag 1.
    [[nodiscard]] static std::uint32_t CleanupSlot(
        std::uint32_t slot, ISystemEventRouterTreeRuntime& runtime);
};

class SystemEventRouterTree final {
public:
    // IDA 0x528820; 32-byte node, integer key at +12, group at +16.
    [[nodiscard]] static std::uint32_t FindOrCreateGroup(
        std::uint32_t tree, std::uint32_t key, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t LowerBound(
        std::uint32_t tree, std::uint32_t key, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t InsertNode(
        std::uint32_t tree, std::uint32_t key, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t AllocateNode(
        std::uint32_t parent, std::uint32_t color,
        ISystemEventRouterTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateLeft(
        std::uint32_t tree, std::uint32_t node, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateRight(
        std::uint32_t tree, std::uint32_t node, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
    // IDA 0x5289a0 and 0x528a00 use different list-field offsets.
    static void CleanupNestedList(
        std::uint32_t list, ISystemEventRouterTreeRuntime& runtime);
    static void CleanupGroupList(
        std::uint32_t group, ISystemEventRouterTreeRuntime& runtime);
    static void RemoveListRange(
        std::uint32_t holder, std::uint32_t result_slot,
        std::uint32_t first, std::uint32_t sentinel,
        ISystemEventRouterTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateEmptyList(
        std::uint32_t first_link, std::uint32_t second_link,
        ISystemEventRouterTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Predecessor(
        std::uint32_t node, std::uint32_t head, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindExact(
        std::uint32_t tree, std::uint32_t key, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterScriptEvent(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback,
        std::uint32_t argument, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t CloneCallback(
        std::uint32_t callback, ISystemEventRouterTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetPayload(
        std::uint32_t payload, ISystemEventRouterTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t LowerBoundMode(
        std::uint32_t first, std::uint32_t end, std::uint32_t mode,
        ISystemEventRouterTreeRuntime& runtime);
    static bool CopyListRange(
        std::uint32_t destination, std::uint32_t first,
        std::uint32_t end, bool move_payload,
        ISystemEventRouterTreeRuntime& runtime);
    static bool CopyGroupRecord(
        std::uint32_t destination, std::uint32_t source,
        bool move_payload, ISystemEventRouterTreeRuntime& runtime);
};

class ActorManagerEventRouter final {
public:
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterLate(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterCombatHandler(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
};

class ActorManagerPlayerEventRouter final {
public:
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
};

class WorldManagerEventRouter final {
public:
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
};

class CombatConsoleEventRouter final {
public:
    // IDA 0x540d90.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
};

class CombatActionConsoleWindowEventRouter final {
public:
    // IDA 0x542670; script_createEvent stores owner + argument and callback.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback,
        std::uint32_t argument, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
};

class CombatMessageEventRouter final {
public:
    // IDA 0x54e470.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback,
        std::uint32_t argument, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
    // IDA 0x54f050; plain owner and callback fields.
    [[nodiscard]] static std::uint32_t RegisterPlain(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
};

class CombatRoleStateEventRouter final {
public:
    // IDA 0x551fd0.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
};

class CombatEventOutputEventRouter final {
public:
    // IDA 0x555de0.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
};

class CombatProcessMessageEventRouter final {
public:
    // IDA 0x557180.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
};

class MagicActionEventRouter final {
public:
    // IDA 0x55a190.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
};

class FinalHitEventRouter final {
public:
    // IDA 0x55b660.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
};

class StuntActionEventRouter final {
public:
    // IDA 0x55ccc0.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
};

class PropertyActionEventRouter final {
public:
    // IDA 0x55f280.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
};

class ThrowPropertyActionEventRouter final {
public:
    // IDA 0x55f410.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
};

class CastPropertyActionEventRouter final {
public:
    // IDA 0x55f5a0.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
};

class CombatEndSoundEventRouter final {
public:
    // IDA 0x579fe0.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
};

class CombatActorEventRouter final {
public:
    // IDA 0x57bcf0.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
};

class CombatExtendedEventRouter final {
public:
    // IDA 0x57cc80; same registration algorithm as 0x57bcf0.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t tree, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback, std::uint32_t nil,
        ISystemEventRouterTreeRuntime& runtime);
};

}  // namespace pal4
