# Script RunTo speed and M11 edge classification

2026-11-01. Ownership gates precede live IDA. These are game callback/math
boundaries; existing collision, animation, SDK, string and native library
services remain in use. No script replacement, endpoint overwrite, RNG or
clock compensation is introduced.

## Original callback constants

| Callback | Entry | Speed assignment | Original float bits/value |
| --- | --- | --- | --- |
| PlayerWalkTo | 5E1CC0 | 5E1D79 | 42340000 /45 |
| PlayerRunTo | 5E1FE0 | 5E2099 | 430C0000 /140 |
| CurrentWalkTo | 5E2280 | 5E2343 | 42340000 /45 |
| PlayerBackTo | 5E2520 | 5E25D9 | 42340000 /45 |
| NpcWalkTo | 5E27E0 | 5E28A5 | 42340000 /45 |
| NpcRunTo | 5E2A90 | 5E2B55 | 430C0000 /140 |
| NpcBackTo | 5E2D40 | 5E2E05 | 42340000 /45 |

Both run callbacks were incorrectly135 in ScriptPlayerWalk. Their independent
immediates are1124859904=140, not the earlier misleading IDA prose135. Those
IDA comments are corrected. Shared indexed/named run methods now use140;
walk/back remain45. The boundary tests assert the independent original raw
immediate bits for run and walk/back, and the Q01 production resource fixture
now expects its actual script RunTo140. An unrelated synthetic collision test's
chosen135 speed remains an arbitrary fixture input, not a script contract.

## Actual failed paired evidence

Pair131 original70500 -> real flight -> M11/1 opening accepts captions42847..42861
with exact text/page, seven pose/camera float32 groups, party/48globals/money/
fullMT and observed inventory. At42862, leader position matches but retained
yaw is original188.7532958984375/native188.74197387695312. Actual speed words
are140/135. Read-only endpoint/path sampling shows the last velocity is already
zero and motion is held, so it cannot prove speed alone caused the yaw error.

Pair132 corrects both speed words to140, repeats the accepted prefix, then reads
every held frame after42861. At observed frame6, before the first nonzero move,
pose/collision position/zero velocity/speed140/hold0 match; one of nine path
points and selected cursor differ (original2/native8). This independently
refutes a speed-only diagnosis. No42862 input is sent after either failure.

## Captured PC24 dot cancellation

Read-only132 comparison confirms all1226 floor face vertices and neighbor IDs,
path face sequence [887,887,901,908,918,923,929,936,910] and candidate heap IDs/
priorities agree. On face908 edge1, identical midpoint/normal operands produce:

- original PC24 rounded products35.6279411315918 and-35.6279411315918, sum+0;
- widened double products35.627941454949905 and-35.627942006714875,
  sum-5.517649697139859e-7.

The negative native result incorrectly classifies the exact edge midpoint
outside, retreats it10 percent toward the triangle centroid, and changes
SkipReachable's chosen cursor. Offline replacing only this node restores
original cursor2. No epsilon is required or introduced.

Live603150 assembly FMUL60315B/FMUL603160/FADDP603162 establishes each PC24
rounding boundary. D3dVector2Operations::Dot now explicitly stores each product
and sum as volatile float before widening the public return. Ordinary float
locals failed the captured tests under MSVC x86 extended intermediates; that
failed build/test is retained. Existing vector2 and line2 regressions now assert
exact+0 and Classify(midpoint,0)==0 using the captured operand bits. Length/
Normalize are reviewed but unchanged; other normal-cache ULP differences do
not establish a need to change them for this demonstrated failure.

## Captured PC24 negative-X yaw

Pair133 corrects the dot/path divergence and matches all motion fields through
frame54 after42861. At frame55, identical velocity bits3199820186/0/3222703116
produce original yaw433CBD3E versus native433CBD3F. Live442970, specifically
FCHS442B14/FADD(double2pi)442B16 before FMULdegrees442B28, proves the missing
PC24 complement rounding. BaseObjectMotion stores that complement as volatile
float before multiplying degrees. The regression uses the production shared
GameVectorMath normalize and GameMathTables approximate arccos; neither is
rewritten. Failed old-source and corrected regression logs are retained.

## Production acceptance and checkpoint limits

Pair134 repeats original70500 -> actual flight/WorldMap/M11 and accepts all18
original-CSB captions42847..42864/pages: text/readiness, seven float32 groups,
party/48globals/money/fullMT/observed inventory. All136 observed held movement
frames0..135 match position/collision position/velocity/speed140/hold/cursor
and all9 path nodes. Natural global70700/control1/fade completion and63 further
frames settle three model/current/requested/state/pose records102/104/107.
This is sampled numerical acceptance, not all frames or pixel equivalence.

Both normal SaveWindow writers were reached via actual save1/commonSpace;
System F7 Save is disabled in this dungeon and is not the accepted writer path.
Normal occupied-slot overwrite writers are serialized only to preserve each
shared-path archive. Each0.dat is690789 bytes, with different hashes, under
H:/PAL4/objs/checkpoints/m11-opening-global70700-20261101. Shared slot0 is now
the original source. Native-source pair136 and original-source pair137 pass
saved position/direction, bilateral seven float32 groups, ready/control,
party/48globals/money/fullMT/inventory and three models after63 common frames.
The initial136/137 cold0 endpoints failed only camera_eye. This was subsequently
fixed and accepted in141/142; see ../renderware/camera_before_arena_ready_native.md. No full cold-start, entire-save or pre-save MT continuation claim.
Cold reads use fresh matched process seeds, without RNG compensation.

Pair135's launch driver ended before menu input. Resumed normal load later
naturally reached weather func7001 and missing callback-148, so it is retained
as failure, not checkpoint acceptance. Pair136/137 explicitly arm the startup
checkpoint to hold at the actual first ready Scene/control1 boundary.
The weather callback and cold0 eye discrepancy remain the next blockers.

## Validation and artifacts

m11-run-speed-core-tests1.log: script walk/run/back1/1 passes.
m11-opening132-dot-tests2.log: vector2/line2 capture2/2 passes; tests1 failed.
m11-opening133-yaw-tests2.log: base motion1/1 passes; tests1 failed.
Final application m11-run-path-app-build5.log passes. Final original-resource
m11-run-path-resource1 exits0/noSKIP, GUI13503/13503,477 mute observations.

All artifacts below are in H:/PAL4/objs:
- m11-opening131-movement-readonly.json/.md and42862-all-float32.json;
- m11-opening132-path-readonly.json/.md, path-diagnosis, trace-offline, path-ida;
- m11-opening133-leader-run-frames.json and yaw-diagnosis.json/.md;
- m11-opening134-story-dialogue-journal.json, leader-run-frames.json,
  final-verdict.json and settle-journal.json;
- m11-native70700-cold136-* and m11-original70700-cold137-*;
- checkpoint provenance.json records exact accepted scopes and pending cold0.

Tests use numerical snapshots,800x600 on the dynamically selected nonprimary
1600x900 display, muted and nonactivating. No diagnostic screenshots, forced
story/inventory/RNG/victory flags or replacement CS execution are introduced.
