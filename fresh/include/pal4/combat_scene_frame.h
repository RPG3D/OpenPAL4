#pragma once

#include "pal4/combat_system_render.h"

#include <cstdint>

namespace pal4 {

class ICombatSceneFrameRuntime {
public:
    virtual ~ICombatSceneFrameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual bool IsCombatSystemInactive(std::uint32_t system) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    virtual void ProcessArenaFrame(std::uint32_t manager,
                                   std::uint32_t frame_value) = 0;
};

class CombatSceneFrame final {
public:
    // IDA 0x58af70. Native EAX on return is a render-call artifact.
    static void Render(std::uint32_t frame,
                       ICombatSceneFrameRuntime& runtime,
                       ICombatSystemRenderRuntime& render_runtime);
    static void Render(std::uint32_t frame,ICombatSceneFrameRuntime&,ICombatSystemFrameRenderer&);
private:
    static void RenderArenaIfNeeded(std::uint32_t frame,ICombatSceneFrameRuntime&);
};

}  // namespace pal4
