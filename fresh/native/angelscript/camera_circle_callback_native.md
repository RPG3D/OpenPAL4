# Prepared camera sequence callback289 (2026-10-30)

The post-Boss Dan Room checkpoint (global41602) reaches the fourth sliding
puzzle without replaying either fight. The first slow19-input attempt exhausted
the puzzle timer. The faster driver reads both actual piece arrays after every
accepted move; all19 positions match. Native then failed on missing callback289,
while original remained alive. This is the normal success continuation in M07
func7004, after preparing viewNO006_r and calling giCameraRunCircle(false).

Ownership classification5E4900 returns OWNERSHIP_REVIEW_REQUIRED. Active IDA
Camera_RunSequence at5E4900 confirms the existing ScriptCameraSequences::RunCircle:
empty prepared records return0; otherwise allocate the wait handler8495C4,
clear camera tasks, enumerate prepared records, append task options
{1,0,field14?0:field13}, apply each record, begin waiting, flush and enqueue the
handler, then return1. This is game glue; existing SDK/CPK and camera task owners
remain responsible for resources and playback. No library algorithm is added.

The generic callback dispatcher now forwards the bool argument to that existing
method and returns its integer result. The added original-resource AngelScript
package test compiled actual giCameraRunCircle calls: old dispatcher fails the
ABI assertion (camera-circle-generic-red1.log); repaired dispatcher passes for
false/true wait arguments and the empty record case, including task options and
wait-vtable checks. Build/test logs: H:/PAL4/objs/camera-circle-generic-*.

Fresh pair90400/30756 loaded the shared native post-Boss archive, applied the
same relocation and normal puzzle inputs, and again matched all19 piece arrays.
The repaired callback survives the normal success animation and returns both
games to Scene. Snapshot1021805366 has equal complete MT and all48 globals,
including story41602 and the actual maze slot90DED8=1. Decompiled source labels
are not authoritative global indices. No puzzle, story, result or RNG writes
were used. Evidence: pair-camera-circle-fix-puzzle-journal.json,
pair-camera-circle-fix-restored-scene.json and launch1.log under objs.

The experiment uses controlled17ms frame input and inherits the authorized
Boss HP setup from the checkpoint. Camera interpolation at every frame, default
real-time scheduling and the complete game remain separate acceptance work.

## Next actual frontier

Update: callback313 is now bound and actual ladder/26 subsequent story captions
pass through global50100. The subsequent savepoint encounter is a separate open
comparison. See [current-player timed movement](current_player_timed_move_native.md).
The paragraphs below preserve the original failure evidence.

Paired relocation1358913268 to (70,168.4,-7.6) and W1705736782 cross the actual
func7005 ladder event. Native then exits on missing callback313,
giPlayerCurrentMovment, after starting the two normal GOB movements. Original
continues through func1023 into M07/7, restores control at
(-1265.3817,-0.426886,1548.4672), and writes actual maze slot90DED8=2. Read-only
original evidence: pair-camera-circle-ladder-original.json under objs. Native
error: compare-rebuilt-20261030-132557.err.log. No following input was sent.

Ownership5E8600/5E8760 requires game review. Active IDA5E8600 confirms current
player lookup, movement-component gate at+496, conditional C01/restart1, a68-byte
wait849684, and positive-duration start/target interpolation. Existing
ScriptCurrentPlayerTimedMove already models these calls; native dispatch and
context/lifecycle services still need binding. Reuse existing player animation,
position and interpolation owners. Do not teleport through this scripted move
or mark the ladder transition passed. The verified41602 checkpoint is retained
for the next repair and19-step normal puzzle replay.
