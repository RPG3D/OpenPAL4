#include "pal4/combat_scene_frame.h"

namespace pal4 {

void CombatSceneFrame::Render(
    std::uint32_t frame, ICombatSceneFrameRuntime& runtime,
    ICombatSystemRenderRuntime& render_runtime) {
    RenderArenaIfNeeded(frame,runtime);
    CombatSystemRender::Render(runtime.CombatSystem(), frame, render_runtime);
}

void CombatSceneFrame::Render(std::uint32_t frame,ICombatSceneFrameRuntime& runtime,ICombatSystemFrameRenderer& renderer){
    RenderArenaIfNeeded(frame,runtime);
    (void)runtime.CombatSystem();
    renderer.Render(frame);
}
void CombatSceneFrame::RenderArenaIfNeeded(std::uint32_t frame,ICombatSceneFrameRuntime& runtime){
    const auto system = runtime.CombatSystem();
    const bool inactive = runtime.IsCombatSystemInactive(system);
    if (inactive || runtime.ReadWord(runtime.CombatSystem() + 508u) == 0u) {
        const auto frame_value = runtime.ReadWord(frame);
        runtime.ProcessArenaFrame(runtime.ArenaManager(), frame_value);
    }
}

}  // namespace pal4
