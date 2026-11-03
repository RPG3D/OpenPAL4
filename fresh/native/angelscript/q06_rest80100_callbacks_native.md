# Q0680100 rest callbacks (2026-11-01)

## Evidence and ownership

The real Q06/QN05Y bed event ev_Q06_QN05Y_2 runs CSB func2011. Reference CS
is read as GBK only; it is not compiled or executed. Starting at80100, this
function has49 captions43224..43272, night/day room loads and a later Q06SY
load. It removes roles1/2 from the party before the first caption.

Ownership checks retain game-side callbacks5D7C70/5D7750/5E2520. Live IDA
confirms their behavior; registration shells are not their implementations.
4699A0's misleading GetPlayerDirection registration name was corrected in IDA.

- 5D7C70/-199 giPlayerTakeARest: reacquire PlayerManager for each index0..3;
  only a present player with+B08 membership restores max0->current7 HP then
  max2->current9 MP through its+654 bank. After the loop, play the direct
  gamedata\PALSound\WJ063.wav sample with mode1, even for an empty party.
  This does not itself change day/time, SP or script sound IDs/current handle.
  HP setters retain their ordinary derived health-category update (full HP
  changes category1 to0); rest does not bypass that setter or separately
  cleanse gameplay effects.
  NativeScriptPlayerRest composes the existing native recovery setters and
  direct Miles sample owner; ScriptPlayerRest retains the shared algorithm.
- 5D7750/-183 giPlayerResetEmotion: indexed player lookup, then apply the
  existing stored/default face material; missing player returns0. The native
  adapter uses BaseObjectMaterialTexture::ApplyFace and the existing CPK/SDK
  texture owner. It does not substitute an empty texture name.
- 5E2520/-260 giPlayerBackTo: existing route planner and wait handler, speed45,
  C02/Z02 action, movement status1. Callback delegates to ScriptPlayerWalk's
  existing WalkBack; it does not directly set the destination position.

## Regression and real route

The compiled -199 regression uses isolated actual native player slots and real
AngelScript. Whole player images after rest match the established native HP/MP
setters, including derived health and unchanged nonparty/SP fields. Repeat and empty-party
calls still issue WJ063 mode1 samples. Existing shared tests cover per-player
call order and signed stat bits. Compiled -183 covers signed index forwarding
and missing-service rejection; -260 also retains explicit service rejection.

Build1 succeeds. Real-resource1 correctly refuses acceptance because the held
original170 window still exists. After verified closure, resource2 exits0,
noSKIP, GUI13255/13255, mute observations530. The added compiled rest regression
passes in that run. Original CSB session and EVF stream regressions pass2/2.

Pair171 reloads original80100, cold0/63 requests1583683054/280433222. Actual
M20 exit uses teleport[867.69,7,-547.82]+UP.5, then Q06QY func1077 door uses
[5598.4,-1383.3,-5482.3]+UP.08. The room settles after12 compared individual
frames1418899855, full numerical/model/favor endpoint1071388938 passes.
Bed[83,.7,-56]+SPACE.08 activates the real event. Captions43224..43226 pass.
The repaired -199 executes and day QN05 loads; native then explicitly faults
on missing -183 before43227. Receipt1268647564 has completed1426/granted1429.
The driver stops; no further input is sent. Original171 is subsequently closed
by validated PID/executable/HWND. Pair170 and171 failures remain archived.

Final build3 and compiled callback/session/EVF regressions pass3/3. Final
real-resource3 exits0/noSKIP, GUI12815/12815, mute observations479. Pair172
uses the same validated80100 near-save after adding -183 and -260.
Pair172 cold0/63 passes103006460/108454988; actual outside endpoint1846870475
and room1888620928 pass. Captions43224..43259 (36) pass the seven float32
groups, party/48globals/money/fullMT/inventory/favor endpoints. First morning
43227 additionally verifies role0 HP616->1630 and MP101, SP100 retained;
nonparty roles1/2/3 retain all observed values, including role1 MP111 of126.
Health category naturally changes1->0 through the existing HP setter on both.
An overstrict observer expecting unchanged health initially stopped before any
43227 input; its retained failure is corrected by resuming the same held pair,
without replay or a game-state change. Actual-rest receipt40110382 passes.

At43260, the driver stops before sending input: position/direction and
persistent state match, but camera eye/target differ by about0.1 units. Read-only
task images have identical initial float words+50..80 and record operands+0C..3C;
camera004 duration8 is identical, while elapsed is original1.529999256 vs
native1.444999456, exactly five17ms frames apart. Full record headers differ
(process string pointers/capacity), so full-record byte identity is not claimed.
Evidence: q06-rest80100-pair172-camera43260-readonly/time-verdict.json under objs.
Pair172 is closed normally. Pair173 reproduces cold0/63, the actual route and
the36 passing captions, then holds43259 at1392921435. Two stale-native-task
scan preflights fail before dialogue input; neither input nor the game is
replayed. The observer is corrected to validate the live container and tolerate
an empty camera list, then samples the actual transition one shared frame at a
time. Native003 first appears10072 vs original10067;004 first appears10182 vs
original10177. Original source advances beyond end-action line5032 at10068;
native's wait mask clears10070, but source advances beyond5032 only10073.
This initially localizes the delay before camera003 creation, around J19/end-action and
SDK continuation, without proving the earliest animation cause. Later pairs174..176
trace it upstream to the role3 turn; the shared crossing-branch repair and
pair177's complete49-caption80400 acceptance are recorded in
script_player_turn_wrap_native.md. The movement
endpoint remains identical. Evidence: q06-rest80100-pair173-phase.json and
phase-verdict.json. The300-frame observer ends without another dialogue input;
read-only follow-up confirms both were ready at43260 and held. Original149880
and native145744 identities are q06-descent-original173-identities.json. Pair173
is subsequently normally closed for the earlier-phase observations; pairs174..177
are also closed. Keep original80100 for bugs before the newer80400 archive.

No camera/RNG/clock compensation or
precision relaxation is applied. Pair177 later accepts80400 as documented above. Patrol/CRT, previous M11,
all-frame/render/full-save and pre-save MT continuation limitations remain open.
All artifacts are under H:/PAL4/objs; numerical runs are screenshot-free,
muted/nonactivating800x600 on the dynamically found nonprimary1600x900 display.
