#pragma once

#include "pal4/effect_system.h"

#include <cstdint>

namespace pal4 {

class IEffectSystemPruneRuntime {
public:
    virtual ~IEffectSystemPruneRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil(
        std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveInactiveInline(
        std::uint32_t tree, std::uint32_t node) = 0;
};

class IEffectSystemCleanupEffectsRuntime : public IEffectSystemPruneRuntime {
public:
    ~IEffectSystemCleanupEffectsRuntime() override = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void BeforeDestroyEffect(std::uint32_t effect) = 0;
    virtual void DestroyEffect(std::uint32_t effect,
                               std::uint32_t flag) = 0;
    virtual void ResetDialog(std::uint32_t effect) = 0;
    [[nodiscard]] virtual EffectSystemHandle CopyHandle(
        std::uint32_t id, std::uint32_t system) = 0;
    virtual void NotifyRemoved(std::uint32_t system,
                               const EffectSystemHandle& handle,
                               std::uint32_t context) = 0;
    virtual void ResetTemporaryHandle(EffectSystemHandle& handle) = 0;
    virtual void ReturnEffect(std::uint32_t effect,
                              std::uint32_t mode) = 0;
    virtual void ClearTree(std::uint32_t tree,
                           std::uint32_t first,
                           std::uint32_t head) = 0;
    virtual void DestroyTreeBranch(std::uint32_t root) = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
    virtual void EraseNode(std::uint32_t tree,
                           std::uint32_t node) = 0;
};

}  // namespace pal4
