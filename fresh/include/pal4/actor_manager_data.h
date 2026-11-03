#pragma once

#include <cstdint>

namespace pal4 {

struct ActorManagerTreeGlobals {
    std::uint32_t nil_node = 0;
    std::uint32_t references = 0;
};

class IActorManagerDataRuntime {
public:
    virtual ~IActorManagerDataRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    virtual void InitializeScriptBase(std::uint32_t object) = 0;
    virtual void InitializeLocaleFacet(std::uint32_t facet) = 0;
    virtual void InitializeComplex(std::uint32_t complex) = 0;
    virtual void ConstructNameNode(std::uint32_t node) = 0;
    virtual void DestroyTreeRoot(std::uint32_t tree,
                                 std::uint32_t root) = 0;
    virtual void GetFirstTreeElement(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t AdvanceTreeCursor(
        std::uint32_t cursor) = 0;
    virtual void EraseTreeNode(std::uint32_t tree,
                               std::uint32_t node) = 0;
    virtual void HandleEffect(std::uint32_t handle) = 0;
    virtual void ClearTreeRange(std::uint32_t tree,
                                std::uint32_t first,
                                std::uint32_t head) = 0;
    virtual void ResetNameNode(std::uint32_t node) = 0;
    virtual void CleanupOtherTree(std::uint32_t tree) = 0;
    virtual void DestroyOwnedProperty(std::uint32_t property,
                                      std::uint32_t flags) = 0;
    virtual void CleanupComplex(std::uint32_t complex) = 0;
    virtual void CleanupLocaleFacet(std::uint32_t facet) = 0;
    virtual void ResetScriptBase(std::uint32_t object) = 0;
};

class ActorManagerDataLifecycle final {
public:
    static constexpr std::uint32_t kMinimumSize = 892u;

    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t property,
        std::uint32_t owned_property, ActorManagerTreeGlobals& globals,
        IActorManagerDataRuntime& runtime);
    [[nodiscard]] static std::uint32_t Property(
        std::uint32_t object, IActorManagerDataRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t object, std::uint8_t flags,
        IActorManagerDataRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupTree(
        std::uint32_t tree, ActorManagerTreeGlobals& globals,
        IActorManagerDataRuntime& runtime);
    static void Destroy(std::uint32_t object,
                        IActorManagerDataRuntime& runtime);
};

}  // namespace pal4
