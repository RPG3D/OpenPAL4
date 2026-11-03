#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatPropertyActionCreationRuntime;

struct PropertyTaskEffectHandle {
    std::uint32_t first = 0u;
    std::uint32_t second = 0u;
};

class IUsePropertyTaskRuntime {
public:
    virtual ~IUsePropertyTaskRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void UseProperty(std::uint32_t source,
                             std::uint32_t property,
                             std::uint32_t target) = 0;
    virtual void ConsumeProperty(std::uint32_t source,
                                 std::uint32_t type,
                                 std::int32_t amount) = 0;
    [[nodiscard]] virtual std::uint32_t TargetProperty(
        std::uint32_t target, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyKind(
        std::uint32_t property) = 0;
    [[nodiscard]] virtual std::uint32_t TargetIterator(
        std::uint32_t target) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    virtual void DestroyIterator(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool CandidateEligible(
        std::uint32_t actor) = 0;
    virtual void ApplyToTarget(std::uint32_t target,
                               std::uint32_t source,
                               std::uint32_t property) = 0;
    virtual void NotifySource(std::uint32_t source,
                              std::uint32_t value,
                              std::uint32_t target) = 0;
    [[nodiscard]] virtual std::array<float, 3> ActorScale(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual PropertyTaskEffectHandle CreateChestEffect(
        std::uint32_t actor, std::string_view effect_name,
        std::string_view hook_name) = 0;
    virtual void ResetTemporaryEffect(
        PropertyTaskEffectHandle& effect) = 0;
    virtual void SetEffectScale(std::uint32_t task_handle,
                                const std::array<float, 3>& scale,
                                std::uint8_t mode) = 0;
    virtual void NotifyTarget(std::uint32_t target,
                              std::uint32_t source) = 0;
    [[nodiscard]] virtual bool NeedsUpdate(
        std::uint32_t task_handle) = 0;
};

class IUsePropertyTargetRuntime {
public:
    virtual ~IUsePropertyTargetRuntime() = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class UsePropertyTask final {
public:
    static constexpr std::uint32_t kVtable = 0x845dc4u;
    static constexpr std::uint32_t kSize = 80u;

    // IDA 0x56b0b0: standalone embedded UseProperty task factory.
    [[nodiscard]] static std::uint32_t Create(
        ICombatPropertyActionCreationRuntime& runtime);
    // IDA 0x56a970: assign source, target and property to an embedded task.
    [[nodiscard]] static std::uint32_t SetActorsAndProperty(
        std::uint32_t task, std::uint32_t source,
        std::uint32_t target, std::uint32_t property,
        IUsePropertyTargetRuntime& runtime);
    // IDA 0x55cfe0-0x55d000.
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t task, IUsePropertyTaskRuntime& runtime);
    [[nodiscard]] static bool CheckStatus(
        std::uint32_t task, IUsePropertyTaskRuntime& runtime);
    static void ProcessInternal(
        std::uint32_t task, IUsePropertyTaskRuntime& runtime);
};

}  // namespace pal4
