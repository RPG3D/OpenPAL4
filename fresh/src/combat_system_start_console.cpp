#include "pal4/combat_system_start.h"
namespace pal4 {
void CombatSystemStart::ResetConsoleServices(std::uint32_t object,
    ICombatSystemStartConsoleRuntime& runtime) {
    runtime.ResetConsole(runtime.GetConsole());
    runtime.SetupCameraDefaults();
    runtime.UnsubscribeConsoleEvents(runtime.GetConsole());
    if(runtime.ReadByte(object+516u)==0u)
        runtime.SubscribeConsoleEvents(runtime.GetConsole());
    runtime.DispatchScriptEvent("combat_reset",0);
}
}
