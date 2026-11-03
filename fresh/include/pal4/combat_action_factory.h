#pragma once

#include <cstdint>
#include <new>
#include <string_view>

namespace pal4 {

class ICombatActionFactoryRuntime {
public:
    virtual ~ICombatActionFactoryRuntime() = default;
    [[nodiscard]] virtual bool CombatInactive() = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t object) = 0;
    virtual void InitializeContainer(std::uint32_t container) = 0;
    virtual void DestroyContainer(std::uint32_t container) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void SetNameErrorFlag(std::uint32_t header,
                                  std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AllocateName(std::uint32_t header,
                                             std::uint32_t length,
                                             std::uint8_t flag) = 0;
    virtual void CopyName(std::uint32_t header,
                          std::string_view name) = 0;
    virtual void ReallocateName(std::uint32_t header,
                                std::uint32_t length) {
        if (!AllocateName(header, length, 1u)) throw std::bad_alloc{};
    }
    virtual void SetNameSize(std::uint32_t header, std::uint32_t size) {
        WriteWord(header + 8u, size);
    }
    virtual void CleanupName(std::uint32_t header) = 0;
};

class CombatActionFactory final {
public:
    // IDA 0x5667d0: create QuickAttack or CheckProtect by combat state.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t source, std::uint32_t target, std::uint8_t mode,
        ICombatActionFactoryRuntime& runtime);
};

class CombatMagicActionFactory final {
public:
    // IDA 0x5669c0: create QuickCastMagic or CastMagic by combat state.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t magic, std::uint32_t source,
        std::uint32_t target, ICombatActionFactoryRuntime& runtime);
};

class CombatStuntActionFactory final {
public:
    // IDA 0x566d10: create QuickStunt or Stunt by combat state.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t stunt, std::uint32_t source,
        std::uint32_t target, ICombatActionFactoryRuntime& runtime);
};

class CombatUsePropertyActionFactory final {
public:
    // IDA 0x567060: create QuickUseProperty or UseProperty.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t property, std::uint32_t source,
        std::uint32_t target, ICombatActionFactoryRuntime& runtime);
};

class CombatDefendActionFactory final {
public:
    // IDA 0x5673b0: create a Defend task for one actor.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t actor, ICombatActionFactoryRuntime& runtime);
};

class CombatFleeActionFactory final {
public:
    // IDA 0x5675b0: create QuickFlee or Flee for one actor.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t actor, ICombatActionFactoryRuntime& runtime);
};

class CombatDeadActionFactory final {
public:
    // IDA 0x5678c0: create QuickDead or Dead for a source and target.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t source, std::uint32_t target,
        ICombatActionFactoryRuntime& runtime);
};

class CombatFendOffActionFactory final {
public:
    // IDA 0x567ff0: create QuickFendOff or FendOff.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t source, std::uint32_t target,
        ICombatActionFactoryRuntime& runtime);
};

class CombatMissActionFactory final {
public:
    // IDA 0x568320: create quickMiss or MissTask.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t source, std::uint32_t target,
        ICombatActionFactoryRuntime& runtime);
};

class CombatEndActionFactory final {
public:
    // IDA 0x568660: create EndAction for one actor in either combat state.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t actor, ICombatActionFactoryRuntime& runtime);
};

class CombatWinActionFactory final {
public:
    // IDA 0x568860: create WinAction only during active combat.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t actor, ICombatActionFactoryRuntime& runtime);
};

class ICombatLevelUpActionFactoryRuntime
    : public ICombatActionFactoryRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
};

class CombatLevelUpActionFactory final {
public:
    // IDA 0x568a80: create LevelUp for an active actor not in state 7.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t actor, ICombatLevelUpActionFactoryRuntime& runtime);
};

class ICombatReadyActionRuntime
    : public ICombatActionFactoryRuntime {
public:
    [[nodiscard]] virtual bool IsActorReady(std::uint32_t actor) = 0;
};

using ICombatDefendActionInitiatorRuntime = ICombatReadyActionRuntime;
using ICombatFleeActionInitiatorRuntime = ICombatReadyActionRuntime;

class CombatDefendActionInitiator final {
public:
    // IDA 0x568d80: create a Defend action for a ready actor.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t actor, ICombatDefendActionInitiatorRuntime& runtime);
};

class CombatFleeActionInitiator final {
public:
    // IDA 0x5696b0: create Flee only for a ready actor.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t actor, ICombatFleeActionInitiatorRuntime& runtime);
};

}  // namespace pal4
