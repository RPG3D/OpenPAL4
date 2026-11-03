#include "pal4/combat_system_start.h"
namespace pal4 {
void CombatSystemStart::ResetCollections(std::uint32_t object,
    ICombatSystemStartCollectionsRuntime& runtime) {
    runtime.ProcessCollections(runtime.ReadWord(object+412u));
    runtime.InitializeGameStats(runtime.ReadWord(object+416u));
    runtime.UpdateSystemManager(runtime.ReadWord(object+396u));
}
}
