#include "pal4/combat_system_start_render.h"
namespace pal4 {
void CombatSystemStartRender::Install(std::uint32_t object,ICombatSystemStartRenderRuntime& runtime){
    runtime.WriteWord(object+428u,runtime.GetRenderCallback());
    runtime.SetRenderCallback(kRenderCallback);
}
}
