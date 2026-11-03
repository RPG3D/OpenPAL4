#include "pal4/combat_system_start_ready.h"
namespace pal4 {
bool CombatSystemStartReady::Commit(std::int32_t start_type,ICombatSystemStartReadyRuntime& runtime){
    if(!runtime.HasPlayers())return false;
    runtime.InstallInputCallback();
    runtime.PublishStateOne();
    runtime.DispatchPrepare(start_type);
    runtime.ResetAudioPosition();
    return true;
}
}
