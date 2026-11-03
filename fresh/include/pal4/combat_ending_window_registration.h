#pragma once

#include "pal4/ui_frame_registry.h"
#include "pal4/window_registration_runtime.h"

#include <cstdint>

namespace pal4 {

class ICombatEndingWindowRegistrationRuntime {
public:
    virtual ~ICombatEndingWindowRegistrationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class CombatEndingWindowRegistration final {
public:
    static constexpr std::uint32_t kNameSource = 0x8b6a98u;
    static constexpr std::uint32_t kNameSlot = 0x8f9590u;
    static constexpr std::uint32_t kEventKey = 0x8b690cu;
    static constexpr std::uint32_t kEventRegistrationSlot = 0x8f958cu;

    // IDA 0x542ce0/0x542cf0.
    [[nodiscard]] static std::uint32_t InitializeNameThunk(
        ICombatEndingWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeName(
        ICombatEndingWindowRegistrationRuntime& runtime);
    // IDA 0x542d90/0x542da0.
    [[nodiscard]] static std::uint32_t RegisterEventThunk(
        IWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterEvent(
        IWindowRegistrationRuntime& runtime);
};

class ICombatEndingWindowFactoryRuntime {
public:
    virtual ~ICombatEndingWindowFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Construct(
        std::uint32_t object, std::uint32_t name) = 0;
};

class CombatEndingWindowFactory final {
public:
    static constexpr std::uint32_t kObjectSize = 0x2d0u;
    // IDA 0x542d00.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, ICombatEndingWindowFactoryRuntime& runtime);
};

class ICombatEndingWindowFactoryRegistrationRuntime
    : public IUiFrameFactoryTreeRuntime {
public:
    [[nodiscard]] virtual std::uint32_t UiFrameManagerInstance() = 0;
    [[nodiscard]] virtual std::uint8_t& FactoryGuard() = 0;
    [[nodiscard]] virtual std::uint32_t& FactorySingleton() = 0;
};

class CombatEndingWindowFactoryRegistration final {
public:
    static constexpr std::uint32_t kNameSlot = 0x8f9590u;
    static constexpr std::uint32_t kCreateCallback = 0x542d00u;

    // IDA 0x542d60/0x542d70.
    [[nodiscard]] static std::uint32_t RegisterThunk(
        ICombatEndingWindowFactoryRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t Register(
        ICombatEndingWindowFactoryRegistrationRuntime& runtime);
};

}  // namespace pal4
