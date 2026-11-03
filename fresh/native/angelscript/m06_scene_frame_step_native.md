# Normal Scene script source-step scheduling

2026-10-28 cold-input fixture follow-up: after eager Scene initialization was
removed, PrepareColdInputFrame still tested V signpost fade before advancing
the queued initializer. Actual players8 log records ArenaReady0/init status2.
The fixture now reuses CompleteNonblockingSceneInitProbe, then requires the
real source to have published ArenaReady1 before rendering HUD input frames.
No raw flag/clock writes or production guard relaxation. Full input-load
passes23.93s, including eleven actual signposts, world GPU1171 pixels,
pause/live fade cleanup and save/load/input. Prior assertion107 failures stay
as historical evidence in east-b4/haste players1..7 logs. Build8 passes;
H:/PAL4/objs/haste-native-20261028-players8-lasttest.log is final evidence.

2026-10-27. Live187 restored the M06 Z03 action but still started it at0.402s
versus original0.804s; control returned16.069s versus17.326s. Camera samples
showed an early traversal of the same initial trajectory. Endpoint agreement
was not accepted as intermediate timing agreement.

## Static evidence and implementation

Ownership-classified and inspected current IDA7DF4B0,7DF540 and51A820.
7DF4B0 updates/removes waits and invokes the selected module executor once.
7DF540 calls SDK context vtable+64 ExecuteStep(0) once outside IMM; only mode+268
==1 loops through source steps. Existing NativeScriptFrameRunner::TickOriginal
already implements this rule, including loading/state boundaries and IMM batching.

The application Scene root/event/init frame entries now call the existing
TickOriginal/TickEventOriginal/TickConcurrentSceneOriginal APIs. Previously they
used the accelerated TickUntilBoundary helper and collapsed nonblocking source
statements into one frame. No replacement VM, artificial delay, changed frame dt,
script mutation, story flag or externally forced completion is introduced.

51A820 queues a scene initializer and never executes it. Removed eager initializer
execution from scene-service binding and scene reentry, so camera/controller
restoration completes before normal Scene frames execute the initializer.
Other state and timed-context scheduling callers remain a separate audit; this
change does not establish timing fidelity for all game states.

## Regression coverage

The SDK regression checks that distinct source callbacks execute on separate
normal frames, while the same script completes in one IMM update. Existing state
boundary and wait-resume checks remain. SDK/app entry2/2 passed in2.02s.

Real-resource fixtures now observe queued initialization instead of assuming it
completed inside Loading. CompleteNonblockingSceneInitProbe advances at most64
actual SDK steps, rejects blocking waits and verifies selected context/depth stay
stable before the parent is popped. Both archive loads must finish their real
initializers before later cancellation probes; otherwise the original ArenaReady
guard correctly rejects a later NewGame load.

Per-frame Role checks cover ordinary interpolation, active blend countdown/target
completion, and ActorControl's subsequent real C01/C12 idle selection. Scene ready
must be Boolean while scripts progress and must be1 at completed Q01 story.
The Q01 frame allowance is480 frames; first-dialogue allowance is200 frames. These
are bounded test budgets, not runtime time or completion changes.
Second-NewGame tests update actual Logo/Bink owners and separately verify queued
nested initialization, exact N01 resumption, real HOLD/dialogue, boar approach,
tutorial Escape, return-for-bow items/hints and cave script lock/dialogue.

Final build14 passed. Complete cave-memory resource14 passed with normal shutdown
GUI13578/13578, including repeated NewGame, archive restoration, real animation,
cave story and combat/resource lifecycle coverage. Logs are under
H:/PAL4/objs/m06-script-frame-20261027-*. Earlier failed fixture assertions remain
in resource1..13 logs; migration investigation is archived in
H:/PAL4/objs/m06-script-frame-migration-history.md.

The original read-only actor diagnostic skips zero-length tree-key text reads;
actual original GOB lookup initially still failed. Read-only inspection identified the shared VC6 nil node: three null links with uninitialized key data (text0x1c/length256). The reader now skips that sentinel before touching key storage; all five actual original GOB lookups and endpoint comparisons pass. Other read errors remain explicit.

## Paired runtime acceptance

Live187 closed normally before building. Live188 starts from the genuine pre-fire
M05/global30500 original checkpoint, with original injection only, explicit original
DisableOriginalHeapTrim, native direct EXE, matched startup seed, muted800x600 pair
on the dynamically detected nonprimary1600x900 display. Same authorized relocation
and one paired Space replay the M06 mechanism. Live188 completed418 samples/42s after one paired Space (keydown gap0.2381ms).
Control unlock was observed at17.275314s on both; live187 was original17.325890s,
native16.069472s. Both execute Z03; onset original0.702866s/native0.803285s,
return to C01 both1.909213s. Endpoint Scene/control1, pose/direction/camera float
bits, MT624 and five GOB membership/position/root-LTM checks match.

The remaining camera cut is not accepted: one sample17.175s caught original still
at the hole view and native at player view (both control0). Before17s, maximum eye
component difference23.663, median0.021973 across170 samples; separate frame reads
and100ms intervals do not resolve the remaining phase error. Idle C12 began20.191s
versus22.662s; baseline idle counters were not recorded, so its cause is unproven.
Do not force clocks or completion flags. Next replay should bracket camera reads
more closely and observe idle baseline, or use identical actual movement input
before the authorized event relocation. Full path/pixel timing remains open.

Evidence H:/PAL4/objs/live188-mechanism-verdict.json and
live188-m06-endpoint-verdict.json; current state live188-work-state.json.
Live188 is historical and closed normally. No new save was
written: completed live182 checkpoint and pre-mechanism live176 checkpoint remain.
Full game, twelve modules and full save/RNG persistence are incomplete.

## Refined live190 timing evidence

A common actual W pulse establishes a comparable idle baseline without writing
clocks. Pure read-only process-memory sampling (1531 samples/42s; 5ms around
the cut; median bracket0.4094ms, maximum2.3564ms) bounds the original cut at
17.1945377–17.2000603s and native at17.1557079–17.1622872s. Control unlock
bounds are17.2282196–17.2335487s versus17.1894430–17.1945377s.
First idle onset is19.6267637s versus19.6043596s. The previous2.47s idle
difference largely disappears with a comparable natural baseline, but a residual
camera/unlock phase difference remains open. Endpoint camera/pose bits, five GOBs
and all MT words match. Evidence: H:/PAL4/objs/live190-mechanism-verdict.json,
live190-m06-endpoint-verdict.json and live190-mechanism-camera-trace.jsonl.

Camera task update order versus wait checks is an unverified hypothesis; inspect
IDA before changing ownership or scheduling. Live189 snapshot exports impose
more observer work and are not interchangeable with the pure-memory trace.
Live189/190 closed normally. Current continuation is live191, a natural paired
battle near the M06 exit. No new save or production code change in these runs.

## IDA follow-up during live191

Reinspection confirms58EFD0 processes script wait handlers, then timed scripts,
then the active camera update. Camera_GameLoopUpdate5EAC80 calls
GOMTaskContainer_Update5ECEE0 before Camera_Update. Native
NativeScriptCameraSequenceRuntime::Tick currently advances tasks before checking
its waits. This ordering differs from the original and should be split into
state-owned playback advancement versus script-owned wait polling. No camera
scheduling change has been made yet. The precise share of the observed39ms
residual attributable to this order still requires paired replay after a fix.
