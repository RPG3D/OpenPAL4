#pragma once

#include "pal4/script_entity_callbacks.h"

#include <cstdint>

namespace pal4 {

class NativeArenaNpcSceneLoader;

// Binds giSetNpcVisible to the NPC owner of the current scene.
class NativeScriptNpcVisibilityRuntime final : public IScriptNpcVisibilityRuntime {
public:
    NativeScriptNpcVisibilityRuntime(NativeArenaNpcSceneLoader& npcs,
                                     std::uint32_t arena)
        : npcs_(npcs), arena_(arena) {}

    [[nodiscard]] std::uint32_t NpcManager() override;
    [[nodiscard]] std::uint32_t FindNpc(
        std::uint32_t manager, std::string_view name) override;
    [[nodiscard]] std::uint32_t ArenaManager() override;
    [[nodiscard]] std::uint32_t LeaveNpcScript(
        std::uint32_t arena, std::uint32_t npc) override;
    void SetStateFlag(std::uint32_t npc, std::uint32_t flag) override;
    [[nodiscard]] std::uint32_t EnterNpcScript(
        std::uint32_t arena, std::uint32_t npc) override;

private:
    NativeArenaNpcSceneLoader& npcs_;
    std::uint32_t arena_;
};

}  // namespace pal4
