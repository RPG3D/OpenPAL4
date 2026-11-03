#pragma once

#include "pal4/monster_behavior_tree.h"
#include "pal4/string_buffer.h"

namespace pal4 {

class NativeMonsterBehaviorTreeRuntime final :
    public IMonsterBehaviorTreeRuntime {
public:
    explicit NativeMonsterBehaviorTreeRuntime(
        IStringBufferStorageRuntime& memory) : memory_(memory) {}

    void InitializeRedBlackTree(MonsterBehaviorTreeState& state,
                                std::uint32_t tree_address) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t AllocateMemory(std::uint32_t bytes) override;
    void DestroySubtree(std::uint32_t tree_address,
                        std::uint32_t root_address) override;
    void GetFirstElement(std::uint32_t tree_address) override;
    void RemoveNode(std::uint32_t tree_address,
                    std::uint32_t node_address) override;
    std::uint32_t SharedNil() override;
    void SetSharedNil(std::uint32_t address) override;
    std::uint32_t SharedReferenceCount() override;
    void SetSharedReferenceCount(std::uint32_t count) override;
    std::uint32_t FreeMemory(std::uint32_t address) override;

private:
    IStringBufferStorageRuntime& memory_;
    std::uint32_t nil_ = 0u;
    std::uint32_t references_ = 0u;
};

}  // namespace pal4
