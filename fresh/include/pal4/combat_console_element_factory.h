#pragma once

#include <cstdint>

namespace pal4 {

class ICombatConsoleElementFactoryRuntime {
public:
    virtual ~ICombatConsoleElementFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class CombatConsoleElementFactory final {
public:
    static constexpr std::uint32_t kSize = 0x40u;
    static constexpr std::uint32_t kVtable = 0x8453ccu;

    // IDA ui_combatConsoleElement_constructor at 0x5483a0.
    [[nodiscard]] static std::uint32_t Create(
        ICombatConsoleElementFactoryRuntime& runtime);
};

class CombatResultIconFactory final {
public:
    static constexpr std::uint32_t kSize = 0x40u;
    static constexpr std::uint32_t kVtable = 0x8453e8u;

    // IDA CombatResultIconFactory_Create at 0x5484d0.
    [[nodiscard]] static std::uint32_t Create(
        ICombatConsoleElementFactoryRuntime& runtime);
};

class CombatResultSoundFactory final {
public:
    static constexpr std::uint32_t kSize = 0x44u;
    static constexpr std::uint32_t kVtable = 0x845404u;

    // IDA CombatResultSoundFactory_Create at 0x548890.
    [[nodiscard]] static std::uint32_t Create(
        ICombatConsoleElementFactoryRuntime& runtime);
};

class CombatResultMoveFactory final {
public:
    static constexpr std::uint32_t kSize = 0x44u;
    static constexpr std::uint32_t kVtable = 0x845420u;

    // IDA CombatResultMoveFactory_Create at 0x5489e0.
    [[nodiscard]] static std::uint32_t Create(
        ICombatConsoleElementFactoryRuntime& runtime);
};

class CombatResultAudioFactory final {
public:
    static constexpr std::uint32_t kSize = 0x64u;
    static constexpr std::uint32_t kVtable = 0x845440u;

    // IDA CombatResultAudioFactory_Create at 0x548b90.
    [[nodiscard]] static std::uint32_t Create(
        ICombatConsoleElementFactoryRuntime& runtime);
};

class CombatResultStateFactory final {
public:
    static constexpr std::uint32_t kSize = 0x44u;
    static constexpr std::uint32_t kVtable = 0x84545cu;
    // IDA CombatResultStateFactory_Create at 0x549220.
    [[nodiscard]] static std::uint32_t Create(
        ICombatConsoleElementFactoryRuntime& runtime);
};

class CombatResultPanelFactory final {
public:
    static constexpr std::uint32_t kSize = 0x40u;
    static constexpr std::uint32_t kVtable = 0x845478u;
    // IDA CombatResultPanelFactory_Create at 0x549280.
    [[nodiscard]] static std::uint32_t Create(
        ICombatConsoleElementFactoryRuntime& runtime);
};

class CombatResultEndFactory final {
public:
    static constexpr std::uint32_t kSize = 0x40u;
    static constexpr std::uint32_t kVtable = 0x845494u;
    // IDA CombatResultEndFactory_Create at 0x5493d0.
    [[nodiscard]] static std::uint32_t Create(
        ICombatConsoleElementFactoryRuntime& runtime);
};

class ICombatResultElementDestructorRuntime {
public:
    virtual ~ICombatResultElementDestructorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t DestroyBase(
        std::uint32_t object) = 0;
    virtual void Free(std::uint32_t object) = 0;
};

class CombatResultElementDestructor final {
public:
    // IDA CombatResultElement_DestroyBase at 0x549210.
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t object,
        ICombatResultElementDestructorRuntime& runtime);
    // IDA CombatResultElement_Delete at 0x549440.
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t object, std::uint8_t flags,
        ICombatResultElementDestructorRuntime& runtime);
};

}  // namespace pal4
