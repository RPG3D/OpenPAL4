#pragma once

#include "pal4/monster_manager_finder.h"

namespace pal4 {

class INpcManagerDeleteRuntime : public MonsterManagerFindRuntimeStub {
public:
    virtual void CleanupNpc(std::uint32_t npc) = 0;
    virtual void DestroyNpc(std::uint32_t npc,
                            std::uint32_t deleting_mode) = 0;
    virtual void RemoveNode(std::uint32_t tree,
                            std::uint32_t node) = 0;
};

class NpcManagerDeleteRuntimeStub : public INpcManagerDeleteRuntime {
public:
    void CleanupNpc(std::uint32_t) override;
    void DestroyNpc(std::uint32_t, std::uint32_t) override;
    void RemoveNode(std::uint32_t, std::uint32_t) override;
};

class NpcManagerDeleter final {
public:
    // IDA 0x43cca0: locate the node, destroy its NPC, then remove the node.
    [[nodiscard]] static bool DeleteByName(
        std::uint32_t manager, std::uint32_t name_address,
        INpcManagerDeleteRuntime& runtime);
};

}  // namespace pal4
