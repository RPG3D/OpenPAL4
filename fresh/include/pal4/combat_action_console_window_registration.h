#pragma once

#include "pal4/ui_frame_registry.h"
#include "pal4/window_registration_runtime.h"

#include <cstdint>

namespace pal4 {

class ICombatActionConsoleWindowRegistrationRuntime {
public:
    virtual ~ICombatActionConsoleWindowRegistrationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class CombatActionConsoleWindowRegistration final {
public:
    static constexpr std::uint32_t kNameSource = 0x8b6914u;
    static constexpr std::uint32_t kNameSlot = 0x8f94d4u;
    static constexpr std::uint32_t kEventKey = 0x8b68e4u;
    static constexpr std::uint32_t kEventRegistrationSlot = 0x8f94d0u;

    // IDA 0x540f50/0x540f60.
    [[nodiscard]] static std::uint32_t InitializeNameThunk(
        ICombatActionConsoleWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeName(
        ICombatActionConsoleWindowRegistrationRuntime& runtime);

    // IDA 0x541000/0x541010.
    [[nodiscard]] static std::uint32_t RegisterEventThunk(
        IWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterEvent(
        IWindowRegistrationRuntime& runtime);
};

class ICombatActionConsoleWindowFactoryRegistrationRuntime
    : public IUiFrameFactoryTreeRuntime {
public:
    [[nodiscard]] virtual std::uint32_t UiFrameManagerInstance() = 0;
    [[nodiscard]] virtual std::uint8_t& FactoryGuard() = 0;
    [[nodiscard]] virtual std::uint32_t& FactorySingleton() = 0;
};

class CombatActionConsoleWindowFactoryRegistration final {
public:
    static constexpr std::uint32_t kNameSlot = 0x8f94d4u;
    static constexpr std::uint32_t kCreateCallback = 0x540f70u;

    // IDA 0x540fd0/0x540fe0.
    [[nodiscard]] static std::uint32_t RegisterThunk(
        ICombatActionConsoleWindowFactoryRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t Register(
        ICombatActionConsoleWindowFactoryRegistrationRuntime& runtime);
};

}  // namespace pal4
