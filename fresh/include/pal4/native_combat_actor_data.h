#pragma once
namespace pal4 {
class NativeCombatPlayerSceneActors;
class NativeCombatMonsterManager;
class NativeCombatTaskQueue;
// 529A90 composes both complete 53B3E0 passes and then the shared task queue.
// Borrowed owners must remain alive throughout synchronous animation callbacks.
// This is actor-data advancement, not SystemManager/world/UI or Combat Entry.
void UpdateNativeCombatActorData(float elapsed,NativeCombatPlayerSceneActors&,
    NativeCombatMonsterManager&,NativeCombatTaskQueue&);
}
