#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatProtectTargetRuntime {
public:
    virtual ~ICombatProtectTargetRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void SetProtectTarget(std::uint32_t actor,
                                  std::uint32_t target,
                                  std::uint32_t argument) = 0;
};

class ICombatScriptTaskRuntime : public ICombatProtectTargetRuntime {
public:
    virtual ~ICombatScriptTaskRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void FireEvent(std::uint32_t callback, std::uint32_t actor,
                           std::uint32_t argument) = 0;
    virtual void ConstructBase(std::uint32_t task) = 0;
};

class INormalAttackCreationRuntime : public ICombatScriptTaskRuntime {
public:
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t task) = 0;
};

class INormalAttackProcessRuntime {
public:
    virtual ~INormalAttackProcessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteScale(std::uint32_t address, float value) = 0;
    virtual void PlayAttackSound(std::uint32_t actor,
                                 std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ActorMode(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual float DamageScale(
        std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t TargetIterator(
        std::uint32_t target) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    virtual void ReleaseIterator(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool ActorEligible(
        std::uint32_t actor) = 0;
    virtual void HandleActor(std::uint32_t task,
                             std::uint32_t actor) = 0;
};

enum class NormalAttackActorBranch : std::uint8_t {
    Item,
    Magic,
    Support,
    Attack,
};

class INormalAttackActorRuntime {
public:
    virtual ~INormalAttackActorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool HasProperty8(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint8_t NoPropertyTarget(
        std::uint32_t target, std::uint32_t attacker) = 0;
    [[nodiscard]] virtual bool CheckHit(std::uint32_t attacker_data,
                                        std::uint32_t target_data) = 0;
    [[nodiscard]] virtual std::uint32_t TargetMode(
        std::uint32_t target) = 0;
    [[nodiscard]] virtual bool CheckFloatProperty(
        std::uint32_t attacker_data, std::uint32_t target_data) = 0;
    [[nodiscard]] virtual bool CheckDodge(std::uint32_t attacker_data,
                                          std::uint32_t target_data,
                                          float bonus) = 0;
    [[nodiscard]] virtual std::uint8_t DispatchActor(
        std::uint32_t task, std::uint32_t actor,
        NormalAttackActorBranch branch) = 0;
};

class INormalAttackCommandRuntime {
public:
    virtual ~INormalAttackCommandRuntime() = default;
    virtual void ProcessAttack(std::uint32_t embedded_task) = 0;
    virtual void ProcessAnimationCommand(
        std::uint32_t task, std::string_view original_name) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupCommand(
        std::string_view lowercase_name) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void NotifyTargetState(std::uint32_t actor,
                                   std::uint32_t event,
                                   std::uint32_t payload) = 0;
};

class CombatScriptTaskOperations final {
public:
    // IDA 0x5574a0 and 0x557780.
    [[nodiscard]] static std::uint8_t FireEvent(
        std::uint32_t task, ICombatScriptTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetProtectTarget(
        std::uint32_t task, ICombatProtectTargetRuntime& runtime);
};

class NormalAttackTask final {
public:
    static constexpr std::uint32_t kVtable = 0x8456ecu;
    static constexpr std::uint32_t kSize = 76u;

    // IDA 0x557930-0x557fb0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t task, ICombatScriptTaskRuntime& runtime);
    // IDA 0x5658e0: factory behind QuickAttack child wrapper 0x845988.
    [[nodiscard]] static std::uint32_t Create(
        INormalAttackCreationRuntime& runtime);
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t task, INormalAttackProcessRuntime& runtime);
    static void ProcessInternal(std::uint32_t task,
                                INormalAttackProcessRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandleActor(
        std::uint32_t task, std::uint32_t actor,
        INormalAttackActorRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessCommand(
        std::uint32_t task, std::string_view message_name,
        INormalAttackCommandRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetTarget(
        std::uint32_t task, INormalAttackCommandRuntime& runtime);
};

}  // namespace pal4
