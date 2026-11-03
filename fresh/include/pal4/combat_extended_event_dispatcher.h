#pragma once

#include <cstdint>

namespace pal4 {

class ICombatExtendedEventDispatchRuntime {
public:
    virtual ~ICombatExtendedEventDispatchRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void SelectMagic(std::uint32_t event) = 0;
    virtual void SelectStunt(std::uint32_t event) = 0;
    virtual void UseProperty(std::uint32_t event) = 0;
    virtual void SelectAttack(std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptEventRouter() = 0;
    virtual void ProcessByteEvent(std::uint32_t router,
                                  std::uint32_t descriptor,
                                  std::uint8_t value,
                                  std::uint32_t mode) = 0;
    virtual void ProcessRecordEvent(std::uint32_t router,
                                    std::uint32_t descriptor,
                                    std::uint32_t event,
                                    std::uint32_t mode) = 0;
};

class CombatExtendedEventDispatcher final {
public:
    // IDA 0x57c440, including its tail at 0x57cc40.
    static void Dispatch(std::uint32_t owner, std::uint32_t event,
                         ICombatExtendedEventDispatchRuntime& runtime);
};

}  // namespace pal4
