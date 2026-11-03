#pragma once

#include <cstdint>

namespace pal4 {

class ICastMagicTaskRuntime {
public:
    virtual ~ICastMagicTaskRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteScale(std::uint32_t address, float value) = 0;
    virtual void SetActorTarget(std::uint32_t actor, std::uint32_t magic,
                                std::uint32_t target) = 0;
    [[nodiscard]] virtual int ChangeActorValue(std::uint32_t actor,
                                               std::uint32_t field,
                                               std::int32_t delta) = 0;
    [[nodiscard]] virtual std::uint32_t MagicMode(std::uint32_t magic) = 0;
    [[nodiscard]] virtual float DamageScale(std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t TargetIterator(std::uint32_t target) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual int ReleaseIterator(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool ActorEligible(std::uint32_t actor) = 0;
    [[nodiscard]] virtual int ActorProperty3(std::uint32_t actor) = 0;
    [[nodiscard]] virtual int MagicRejectsSelf(std::uint32_t magic) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool CheckDodge(std::uint32_t attacker_data,
                                          std::uint32_t target_data,
                                          float bonus) = 0;
    [[nodiscard]] virtual int CallActor(std::uint32_t actor,
                                        std::uint32_t slot,
                                        std::uint32_t first,
                                        std::uint32_t second) = 0;
};

class ICastMagicCreationRuntime {
public:
    virtual ~ICastMagicCreationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t object) = 0;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void DestroyBase(std::uint32_t object) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class CastMagicTask final {
public:
    static constexpr std::uint32_t kSize = 76u;
    static constexpr std::uint32_t kVtable = 0x845a64u;

    // IDA 0x566130: factory behind QuickCastMagic child wrapper 0x845a60.
    [[nodiscard]] static std::uint32_t Create(
        ICastMagicCreationRuntime& runtime);
    // IDA 0x558e30-0x559080.
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t task, ICastMagicTaskRuntime& runtime);
    [[nodiscard]] static int SetTarget(
        std::uint32_t task, ICastMagicTaskRuntime& runtime);
    [[nodiscard]] static int HandleActor(
        std::uint32_t task, int mode, ICastMagicTaskRuntime& runtime);
    [[nodiscard]] static int HandleActorInternal(
        std::uint32_t task, std::uint32_t target, int mode,
        ICastMagicTaskRuntime& runtime);
    [[nodiscard]] static int NotifyCastProperties(
        std::uint32_t task, std::uint32_t target,
        ICastMagicTaskRuntime& runtime);
};

}  // namespace pal4
