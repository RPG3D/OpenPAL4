# M06 facing hemisphere correction

Live198 fixed the prior lock/pursuit crash and naturally finished the first
M06/2A fire. All10 GOB active/position/root comparisons passed, then the endpoint
verifier rejected yaw/camera: original293.9325256347656, native66.0674667.
Player position and camera target match; eye X is mirrored. This run is not an
accepted whole mechanism result. Pair198 closed normally before rebuilding.

Ownership review6629A0/5E8310 requires manual review. CurrentPlayer_FaceToGOB
subtracts GOB-player positions, normalizes and calls6629A0. Reinspection of the
actual assembly (not only decompiler status temporaries) confirms first dot
against basis84B820=(0,0,1) feeds the existing approximate arccos. The second
cyclic dot at6629D2..6629F8 is0*y+0*z+1*x. fcomp0 / test ah,5 / jp keeps the
angle for nonnegative or unordered values and reflects only ordered negative X.
The existing reconstruction incorrectly used Z for the hemisphere; old tests
conflated negative X and negative Z.

AngleConversion now retains both original dot products, the existing conversion
constant/approximation boundary, and chooses the X hemisphere. Zero products
are retained for original NaN/Inf behavior, including positive-zero accumulation
before negative-zero Z. No atan2 replacement or duplicate native math helper.
Focused regressions add opposite X/Z quadrant signs and unordered-X handling.
Current classes/functions docs remove the old negative-Z claim; IDA comment
records the actual stack/register evidence. Unit/native builds completed;
focused angle test passed 1/1 in .19 sec; full resource1 passed GUI12890/12890. Live201 naturally completed both fires, nested7004 and actual exit1009 into M06/2B. All10 GOB active/position/root translation fields matched at each fire; actor pose/all camera float32 bits/full MT624 matched. Root translation covers only3f, not full rotation/scale.

Evidence H:/PAL4/objs/live198-light1-after-original.json,
compare-native-312128-1810102.json, live198-light1-verdict.log,
live198-light1-native-gobs.json and live198-close-pair.json.
Continuation checkpoint is m06-double-fire-global31100-20261027-live201, saved through real save1 in2B. Native writer live202 and original writer live203 both cold-loaded on both games with pose/camera bits/fullMT equality and party UI checked. Original313775/native313771 bytes:4-byte difference and full save fields/RNG persistence remain unproven. No forced yaw/camera/flags.
