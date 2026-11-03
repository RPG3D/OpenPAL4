#pragma once
namespace pal4 {
struct NativeCombatSystemImage;
class NativeCombatTimedEvents;
class NativeCombatGameEventHandler;
class NativeCombatSystemManager;
// Execute the existing Start reset sequence against actual published owners.
// Call before new stage events populate the scheduler. This does not reset
// ActorManager, create actors, enter the state or commit battle readiness.
void ResetNativeCombatStartCollections(NativeCombatSystemImage&,
    NativeCombatTimedEvents&,NativeCombatGameEventHandler&,NativeCombatSystemManager&);
}
