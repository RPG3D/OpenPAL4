# M06 current-player walk and object facing

2026-10-27. Scripts are read directly as GBK from the provided original
gamepatch directory; runtime CSB is unchanged.

## Actual failure

live179 had passed the actual M05 vine->M06/1A transition with Scene/control1,
matching pose/camera bits/fullMT624. The actual `light01_f` GOB script field
points to `func7001`. Shared authorized setup182.5,1.6,-108 and one commonSpace
started this mechanism. Native exited on unimplemented callback-259; original
completed the mechanism and recovered control. This is a new failure after the
accepted geometry-section correction.

Original ending position182.4689026,0.5999985,-107.9917984 and final GOB samples
are retained in `H:/PAL4/objs/live179-fire-finished-original.json` and
`live179-fire-original-gob-result.json`. The latter's flags24 are raw fields,
not a verified visibility interpretation. No story/HP/RNG/time was written.
Original was then closed normally; native had already exited.

## Existing implementation reuse

Ownership classifier5E2280/5E8310 required review; both were checked in live IDA.
The first resolves the current Player, snaps a target to floor, starts45-speed
C02/Z02 movement and queues the84957C wait. The second resolves a named GOB,
uses target-minus-source position and queues the849570 turn wait.

Existing ScriptPlayerWalk::WalkCurrent and
ScriptPlayerTurn::FaceCurrentToObject already implement these algorithms.
Only callbacks-259 and-312 are added. NativeScriptPlayerTurnRuntime now borrows
the existing scene GOB owner for its real manager/name lookup and owned target
position reads. Player/NPC paths still use their existing checked adapter.
No new movement, math, string, model or wait algorithm is introduced.

All three context construction paths bind the same scene owner: initial,
reloaded and event/concurrent/timed contexts. The scene owner remains alive
while its contexts are retired. Missing names return0; foreign manager IDs and
unbound scene access fail explicitly. The obsolete UnsupportedTurn stub is removed.

## Validation

Build1 succeeded. SDK callback/application tests2/2 passed2.05s; shared existing
walk/turn tests2/2 passed0.24s including current selection/name order, absent
targets, aligned angles and allocation failure. SDK additions distinguish
missing services from unimplemented callbacks. Real-resource regression checks
GOB lookup, position reads without player animation, absent names and foreign
manager rejection. Both full resource gates passed; actual continuation and
remaining failure are recorded below.

Logs: `H:/PAL4/objs/m06-current-move-facing-20261027-*`.
The mechanism also uses object fades and timed GOB movement; this record does
not claim that the entire mechanism or those later services are accepted.


## live180 continuation and current action callbacks

Resource1 passed with GUI11554/11554 and normal shutdown. Live180 loaded the
same genuine30500 source, repeated the vine and mechanism setup/commonSpace.
It crossed the previous walk/face points and exited later on callback-247
(giPlayerCurrentDoAction). This proves forward script progression, not matching
intermediate pose or wait timing. Original naturally completed and was closed
normally; no game processes remain from live180.

Ownership5E0570/5E07F0 was classified and both functions rechecked in IDA.
Callbacks-247/-248 now reuse existing ScriptPlayerAction::StartCurrent/EndCurrent
and the bound native action owner. No animation algorithm was rewritten.
Build2 and SDK/application2/2 passed2.04s; shared action1/1 passed0.20s.
Resource2 passed with GUI10674/10674 and normal shutdown. Evidence:
`H:/PAL4/objs/live180-work-state.json`. Later object fade/move services remain open.


## live181 outcome

The same genuine30500 load/vine/mechanism setup and commonSpace now progresses
through all four current-player callbacks. Native then exits on unimplemented
callback-302 (giOBJBlendOut). Original naturally finishes and recovers control1;
its final pose and normal close are retained. No intermediate numeric pose or
wait-timing equivalence is claimed from this progression alone.

Current authoritative state: `H:/PAL4/objs/live181-work-state.json`.
There are no remaining live game processes or input drivers. Next work is the
native binding of existing ScriptObjectBlend, followed by ScriptObjectTimedMove,
with actual GOB ownership, material SDK calls and per-context wait cleanup.
The full mechanism remains unaccepted; no forced door/story/completion writes
were made. Resume from the archived30500 checkpoint and shared prepared position.


Subsequent object fade/move binding and functional completion are recorded in
[m06_object_effects_native.md](m06_object_effects_native.md). Current state is
`H:/PAL4/objs/live184-work-state.json`; the intermediate camera timing discrepancy
remains open. Prior missing-callback failures are retained.
