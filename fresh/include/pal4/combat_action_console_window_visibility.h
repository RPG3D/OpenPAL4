#pragma once

#include "pal4/event_handler_array.h"

#include <cstdint>

namespace pal4 {

class ICombatActionConsoleWindowVisibilityRuntime {
public:
    virtual ~ICombatActionConsoleWindowVisibilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ActorProperty(
        std::uint32_t context, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ActorManager() = 0;
    [[nodiscard]] virtual std::uint32_t AcquireActorIterator(
        std::uint32_t manager_collection) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool ActorHasMagic(
        std::uint32_t magic_collection) = 0;
    virtual void ReleaseIterator(std::uint32_t iterator,
                                 std::uint32_t flags) = 0;
    virtual void SetEnabled(std::uint32_t button,
                            std::uint8_t enabled) = 0;
    virtual void RequestRedraw(std::uint32_t button) = 0;
    [[nodiscard]] virtual bool IsDisabled(std::uint32_t button,
                                          bool recursive) = 0;
    virtual void SetVisible(std::uint32_t panel,
                            std::uint8_t visible) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
};

// Game display algorithm boundary; native activity and roster owners provide
// these operations without exposing private STL/handler-array representations.
class ICombatActionConsoleDisplayRuntime {
public:
    virtual ~ICombatActionConsoleDisplayRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual void WriteWord(std::uint32_t,std::uint32_t)=0;
    virtual void WriteByte(std::uint32_t,std::uint8_t)=0;
    virtual std::uint8_t ActorProperty(std::uint32_t,std::uint32_t)=0;
    virtual bool HasAvailableMagic()=0;
    virtual void SetEnabled(std::uint32_t,std::uint8_t)=0;
    virtual void RequestRedraw(std::uint32_t)=0;
    virtual bool IsDisabled(std::uint32_t,bool)=0;
    virtual void SetVisible(std::uint32_t,std::uint8_t)=0;
    virtual std::uint32_t SetInputActivity(std::uint32_t,bool)=0;
};

class CombatActionConsoleWindowVisibility final {
public:
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t window,std::uint32_t actor_context,std::uint8_t visible,
        ICombatActionConsoleDisplayRuntime& runtime);
    // IDA 0x541740.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t window, std::uint32_t actor_context,
        std::uint8_t visible,
        ICombatActionConsoleWindowVisibilityRuntime& runtime,
        IEventHandlerArrayRuntime& array_runtime);
};

}  // namespace pal4
