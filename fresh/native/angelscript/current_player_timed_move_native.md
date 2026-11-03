# Current-player timed movement through the scene wait queue (2026-10-30)

The actual M07 func7005 ladder event stopped at callback313,
giPlayerCurrentMovment. Original completed func1023 into M07/7. The previous
global41602 save and19 normal puzzle inputs reproduce the native failure without
replaying either Boss.

## Original boundary and reuse

Ownership5E8600/5E8760 requires game review. Active IDA confirms current player
selection, a nonzero movement component at+496, conditional case-insensitive
C01 selection/restart1, a68-byte handler849684, and captured start/target/duration.
Update accumulates delta, caps elapsed at duration, closes the handler, calls
the existing linear interpolation helper for XYZ, then invokes player virtual+60.
That setter includes the existing camera/listener synchronization. A zero or
negative duration does not flush or interpolate; the existing shared code retains
that behavior.

NativeGameScriptCallbacks now forwards callback313 to the existing
ScriptCurrentPlayerTimedMove, linked into the native callback target. The new
adapter in NativeScriptObjectEffectsRuntime borrows the existing per-context
NativeScriptPlayerActionRuntime and player-position service. It validates player
slots, selects actual clips/restart flags, and uses the real position override.
GOB effects and player moves share the existing ordered ScriptExecutionContext
queue; the849684 tag selects the existing player update rather than the GOB
update. Cancellation closes without snapping. Binding rejects changed services
while handlers exist; handlers clear before scene services retire.

No separate movement/interpolation, private string, allocator or library
algorithm was introduced. Original resources, RenderWare and the existing
FloatInterpolator/ScriptWaitHandlerBase owners remain active.

## Regression and live evidence

The original-resource fixture compiles actual SDK giPlayerCurrentMovment calls.
It verifies blocking continuation, C01 selection, simultaneous nonblocking GOB
and blocking player movement, halfway XYZ, endpoint XYZ and real RpClump root
LTM, asynchronous continuation, cancellation without snapping, and foreign-player
rejection. The fixture runs before ordinary scene-service binding, so it wires
the same actual camera/position services locally with the silent listener.

Full --cave-memory-smoke with the canonical original resource/save corpus passes,
exit0 and GUI13244/13244. It stays muted/nonactivating on detected DISPLAY1.
Logs: H:/PAL4/objs/current-player-timed-move-smoke4* and native-build3.log.
smoke1 was skipped because an original window remained; smoke2 exposed the
fixture's missing camera binding. smoke3 passed the new movement checks but its
later legacy-save assertion used the modified comparison slot0, which is not
the canonical early-game fixture. These runs are not counted as full passes.

Fresh pair84972/73540 loads the verified41602 checkpoint and matches all19
puzzle moves. Normal relocation1056693197 and W606354896 trigger func7005.
A read during arena replacement briefly found no original ActorControl actor;
the driver stopped without repeating input. Reading the same live pair again
at1967840550 verifies both Scene/control1, M07/7, equal positions
(-1265.3817,-0.426886,1548.4672), all48 globals equal (maze slot90DED8=2), and
complete MT/index624 equal. See pair-current-player-move-ladder-restored.json.

The same pair enters M07/8 through the actual func1025 volume. All26 source
captions41930..41955 match before each paired Space, including playback/page
gates. Snapshot480821416 verifies story50100, all48 globals and full MT equal.
Artifacts: pair-current-player-move-section8-story* and section8-restored.json.

## Subsequent encounter remains open

During savepoint setup, original reached overwrite confirmation while native
entered an ordinary encounter (2518,2518,2521). Snapshot235761741 retains
original MT624/native7 and native combat4. Do not count that save attempt as
paired equality or attribute the difference to the new timed-movement callback.
Independent free-scene elapsed histories, pursuit and collision remain to be
investigated. ATB/MT export202610307 and both input journals are retained.

Original's normal save writer archived global50100/arena255, SHA256
70a9edd261666fb6f61be5b5bbe47f1a3ddd8568bfaf4007107575a6a3c08d66, under
H:/PAL4/objs/checkpoints/m07-leave-tomb-global50100-20261030-current-player-original.
This supplies the closest checkpoint for follow-up; native writer and encounter
equivalence are separate acceptance. The run still uses17ms
controlled time and inherits authorized Boss HP setup from its checkpoint.
Default real-time scheduling, full camera-frame equivalence and the whole game
remain unverified.

Reloading this original-written50100 archive as shared slot0 starts fresh
pair91960/79128. After common startup SPACE/ENTER, both are in the same ordinary
encounter: request1556960937 has equal positions, all48 globals, all six sampled
combat actor fields and complete MT/index27. This is a verified paired reentry
point, not proof of untouched Scene restoration or a native save writer.

Normal paired Flee key6/request998115156 then exposes another missing native
consumer: Combat actor skill FUNCTION9115. Original remains in the fight
(subsequent sample phase4/MT49); do not claim successful Flee. The native process
exits and the original remains responsive. Error log uses timestamp135855;
read-only original sample is pair-50100-cold-original-after-flee.json under objs.
Ownership580590 requires game review; active IDA confirms target collection
enumeration via virtual352, count of actors not in state3, and four integer-pair
dispatches via virtual372 when count<=action+20. Existing
CombatEventOutcome::ProcessMultiEffect already models the entry. Native skill
dispatch/collection/event services remain to be wired and tested from this
50100 checkpoint. No HP, story, result or RNG writes were used for the encounter.

2026-10-30 follow-up: FUNCTION9115 is now bound and the first ordinary
encounter returns to Scene through normal paired Flee (full MT70). A second
encounter retains MT77 equality but exposes a stable positive-step ATB
difference; no further gameplay input was sent. Production summon spawning
remains unbound. See [9115 evidence and open boundaries](../package/combat_multi_effect_native.md).
