#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct QuickCastMagicChildFactorySlot {
    std::uint32_t vtable = 0u;
};

class QuickCastMagicChildFactory final {
public:
    static constexpr std::uint32_t kVtable = 0x845a60u;
    // IDA 0x565f20.
    [[nodiscard]] static QuickCastMagicChildFactorySlot* Initialize(
        QuickCastMagicChildFactorySlot* slot);
};

class IQuickCastMagicSequenceRuntime {
public:
    virtual ~IQuickCastMagicSequenceRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void NotifySource(std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t CreateChildTask(
        std::uint32_t factory_vtable) = 0;
    virtual void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text, const std::array<std::uint32_t, 3>& timing) = 0;
};

class QuickCastMagicSequenceTask final {
public:
    static constexpr std::uint32_t kVtable = 0x845ac0u;

    // IDA 0x566c60/0x566c80: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        class IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, class IScriptTaskBaseRuntime& runtime);

    // IDA 0x565ea0: add a quick-cast child with source, target and magic.
    [[nodiscard]] static std::uint32_t Start(
        std::uint32_t task, IQuickCastMagicSequenceRuntime& runtime);
    // IDA 0x565f30: finish quick casting and notify unready targets.
    [[nodiscard]] static std::uint32_t Complete(
        std::uint32_t task, class IQuickCastMagicCompletionRuntime& runtime);
};

class IQuickCastMagicCompletionRuntime {
public:
    virtual ~IQuickCastMagicCompletionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual bool CombatActive() = 0;
    [[nodiscard]] virtual bool SourceStateThree(std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessDead(
        std::uint32_t source, std::uint32_t target) = 0;
    [[nodiscard]] virtual std::uint32_t EndAction(std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t OpenTargetIterator(
        std::uint32_t target) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    virtual void DestroyIterator(std::uint32_t iterator,
                                 std::uint32_t flags) = 0;
    [[nodiscard]] virtual bool IsActorReady(std::uint32_t actor) = 0;
    virtual void NotifyUnreadyActor(std::uint32_t actor,
                                    std::uint32_t source) = 0;
};

}  // namespace pal4
