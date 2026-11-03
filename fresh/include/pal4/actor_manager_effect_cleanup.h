#pragma once

#include <cstdint>

namespace pal4 {

class IActorManagerEffectCleanupRuntime {
public:
    virtual ~IActorManagerEffectCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t NilNode() = 0;
    [[nodiscard]] virtual std::uint32_t AdvanceTreeCursor(
        std::uint32_t cursor) = 0;
    virtual void ResetDynamicEffect(std::uint32_t handle_address) = 0;
    virtual void ClearTreeRange(std::uint32_t tree,
                                std::uint32_t root) = 0;
    virtual void EraseTreeNode(std::uint32_t tree,
                               std::uint32_t node) = 0;
    virtual void UpdateEffect(std::uint32_t handle_address) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeActorSlot(
        std::uint32_t actor, std::uint32_t slot,
        std::uint32_t argument) = 0;
};

class ActorManagerEffectCleanup final {
public:
    [[nodiscard]] static std::uint32_t Release(
        std::uint32_t actor, IActorManagerEffectCleanupRuntime& runtime);
};

}  // namespace pal4
