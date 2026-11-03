#pragma once

#include "pal4/combat_child_text_assignment.h"
#include "pal4/combat_ready_target_scan.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct CastMagicChildFactorySlot {
    std::uint32_t vtable = 0u;
};

class CastMagicChildFactory final {
public:
    static constexpr std::uint32_t kVtable = 0x845a5cu;

    // IDA 0x565c90.
    [[nodiscard]] static CastMagicChildFactorySlot* Initialize(
        CastMagicChildFactorySlot* slot);
};

class ICastMagicSequenceRuntime : public ICombatChildTextRuntime {
public:
    virtual ~ICastMagicSequenceRuntime() = default;
    [[nodiscard]] virtual std::array<float, 3> ActorChildPosition(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::array<float, 3> ActorTargetPosition(
        std::uint32_t actor) = 0;
    virtual void NotifySource(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t MagicMode(
        std::uint32_t magic) = 0;
    [[nodiscard]] virtual std::uint32_t CreateChildTask(
        std::uint32_t factory_vtable) = 0;
    virtual void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t source_text,
        const std::array<std::uint32_t, 3>& timing) = 0;
    virtual void SetApproachTaskTarget(
        std::uint32_t child, std::uint32_t source,
        const std::array<float, 3>& position) = 0;
    [[nodiscard]] virtual std::uint32_t ActorCastText(
        std::uint32_t source, std::uint32_t magic) = 0;
};

class CastMagicSequenceTask final {
public:
    static constexpr std::uint32_t kVtable = 0x845aa4u;

    // IDA 0x566bb0/0x566bd0: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        class IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, class IScriptTaskBaseRuntime& runtime);

    // IDA 0x565a00: schedule optional movement and the magic child.
    [[nodiscard]] static std::uint8_t Start(
        std::uint32_t task, ICastMagicSequenceRuntime& runtime);
    // IDA 0x565ca0: choose a follow-up combat action and its target.
    [[nodiscard]] static std::uint32_t CreateNext(
        std::uint32_t task, std::uint32_t source,
        class ICastMagicTransitionRuntime& runtime);
    // IDA 0x565e50: end the sequence or handle a defeated source.
    [[nodiscard]] static std::uint32_t Complete(
        std::uint32_t task, class ICastMagicCompletionRuntime& runtime);
};

class ICastMagicCompletionRuntime {
public:
    virtual ~ICastMagicCompletionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool CombatActive() = 0;
    [[nodiscard]] virtual bool SourceStateThree(std::uint32_t source) = 0;
    virtual void NotifyDefeatedSource(std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessDead(
        std::uint32_t source, std::uint32_t target) = 0;
    [[nodiscard]] virtual std::uint32_t EndAction(
        std::uint32_t source) = 0;
};

class ICastMagicTransitionRuntime : public ICombatReadyTargetRuntime {
public:
    virtual ~ICastMagicTransitionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool CanCastMagic(std::uint32_t actor,
                                             std::uint32_t magic) = 0;
    [[nodiscard]] virtual bool ActorPropertySlot8(
        std::uint32_t actor, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t MagicAction(
        std::uint32_t magic, std::uint32_t source,
        std::uint32_t target) = 0;
    [[nodiscard]] virtual std::uint32_t NormalAction(
        std::uint32_t source, std::uint32_t target,
        std::uint32_t mode) = 0;
};

}  // namespace pal4
