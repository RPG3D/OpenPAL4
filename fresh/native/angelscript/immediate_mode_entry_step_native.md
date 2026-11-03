# IMM entry selects the source-step policy

2026-10-31 pair89 locates the M10 one-frame divergence after the final42598
caption. Actual fade creation at held7705 (elapsed17ms), completion at7793
(phase1, elapsed1.5), and flash-wait retirement at7794 match. The later mode5
call and fade-in start are native one frame early. This excludes changing the
fade clock as a justified correction for this fork.

Ownership-first live IDA `7DF540` selects its SDK execution policy at entry:
if current record+268 is1, it enters the IMM loop; otherwise `7DF6EA..7DF700`
executes one SDK step and returns. An entry-IMM0 step that calls `giIMMBegin`
therefore returns with IMM1; it does not fall through into the loop in the
same invocation. A subsequent entry-IMM1 invocation loops until `giIMMEnd`
clears the mode. Entry execution-flag handling precedes this choice.

The old native `ExecuteStep` tested the mode only after the first SDK step.
Entering IMM0 and executing IMMBegin therefore immediately consumed the IMM
block in that same frame. Pair89 held7795 has original IMM1 while native has
already reached line1009; held7796 original has exited IMM and retains a
closed849550 handler. This is consistent with the entry-policy discrepancy,
without attributing the difference to fade arithmetic or camera geometry.

`NativeScriptFrameRunner::ExecuteStep` now snapshots `run_immediate` at entry.
Only entry-IMM1 may repeat SDK steps; each iteration still applies callback
cleanup, preserves the state-boundary check, checks SDK status, and stops when
IMM ends. It introduces no timer, artificial delay, frame identifier or
deduplication. `TickUntilBoundary` retains its explicit outer stepping policy.

`VerifyImmediateEntryPolicy` in `angelscript_frame_runner_tests.cpp` compiles a
real SDK script, delegates IMMBegin/End to actual NativeGameScriptCallbacks,
and records marker callbacks. It verifies the frame entering IMM executes only
IMMBegin, the next entry-IMM1 frame reaches IMMEnd without consuming the next
marker, and later normal frames finish the script. Existing scheduling and
state-boundary regressions remain. The test target links the actual game
callbacks library so the IMM handlers are real while marker callbacks remain
bounded and do not start gameplay.

Parent validation: Release build2, real SDK runner1/1 and full native resource1
passed (exit0, no SKIP, GUI13508/13508, mute450, dynamic1600x900 secondary).
Build1's missing callback-library test linkage was corrected before acceptance.
Pair90 cold-loaded the original61003 checkpoint and replayed all24 captions/
pages42575..42598. Final61005 request1441021389 and four further120-frame
endpoints2055375738/240631646/639797284/839564014 pass all seven float32
pose/camera groups, control1, party/globals/money/full MT/observed inventory.
An additional actual16-call collision capture passes strict same-index and
outer-frame ownership, without suffix alignment (m10-collision90-idle-camera-
strict.json). This closes the observed free-camera phase/distance fork.

No normal61005 save or bilateral cold restore has been accepted: latest
reliable restart checkpoint remains61003. Full mainline, general clock/RNG
and pixel equivalence remain open. All artifacts are under H:/PAL4/objs:
m10-imm-entry-*, run-m10-collision90.log, m10-collision90-final-verify2.log,
and m10-collision90-idle-camera-arm/strict.json. The first final verifier had a
missing ProcessReader import after its read-only float checks; it was corrected
and rerun against the same held pair, with no repeated gameplay input.

Read-only capture: `H:/PAL4/objs/m10-collision89-script-phase.json`.
