# M07 back hall: immediate player position and yaw

2026-10-27. live276/277 load the original live271 chest save. M07/3's exit
func1009 has geometry nearZ-1870. Paired relocation to that line reaches actual
M07/4 initialization; this is navigation setup, not movement/closed-door proof.
Earlier live274/275 remain failed navigation attempts followed by natural defeat.
Both are normally closed. No story/HP/RNG/clock/result writes.

M07/4 func2003 uses an open strip, not a solid AABB. Its10 vertices form an
open front and sides/back across roughlyZ1380..1080. Standing within its bounds
does not prove a crossing. Actual forward movement across the back starts the
original script on both ends. Current story global90DEC0 is read-only40900.
Text reference is the user's GBK M07.cs; original CSB executes.

live277 uses TraceActionProgress/TraceMtRandom and actual RichText playback:
17 common Space inputs cover16 distinct giTalk calls41783..41798, with one
natural extra page at41797. Native then exits on unimplemented callback-201,
giPlayerSetPosRot(1,126.9,235.4,-448.5,274), before41799. Original normally closes.
No claim that all45 dialogue calls or global41000 have completed.

## Implementation

Ownership checks5D7D90/5D7E00 require review as game code. Existing
ScriptEntityTransform::PlayerByIndex/NpcByName are reused. The latter resolves
NPCs despite the giPlayer overload name. New NativeScriptEntityTransformRuntime
borrows the existing position lookup, rotation and resource-transform services;
it owns no actor or scene data. Both callbacks-201/-202 are bound.

IDA Player vtable842F10+4/+8 are666550/6666E0. They differ from its+60(decimal)
43EDD0 camera/audio override used by normal script positions. Thus immediate
base position reuses GameResourceManager::UpdateClumpTransform directly, and
rotation uses the existing native rotation service. Preserve prior X/Z, replace
yaw, mode0; no action reset, camera/audio update or queued wait is invented.
IDA's old preview-reset comment has been corrected to this proven behavior.

The native fixture compiles both actual SDK overloads, transforms the real
player/RwFrame, verifies nonzero X/Z preservation and exact position/yaw, checks
missing NPC no-op and no new wait handlers, then restores the original transform.
Shared focused and standalone native callback tests pass; application build3
and full resource2 pass, GUI13052/13052 with normal shutdown. build4 explicitly
links the existing resource-transform source into the callback library as well,
so standalone callback targets retain their link boundary.

resource1 failed first in the older reward fixture: a newly constructed temporary
inventory's money field is not initialized until Reset. The fixture now follows
the real NewGame Start/Reset lifecycle, retaining the money200 assertion. This
is test setup repair; production inventory constructor semantics are unchanged.

## Actual post-fix route

live278 repeats from the same original-writer near save with both traces enabled.
19 common Space inputs cover18 distinct source giTalk calls41783..41800, including
one natural extra page41797. Both-201 calls before41799/41800 now execute through
the actual CSB event context. Real RichText completion/page boundaries are read
without writes before every input. Screenshot41799 has equal main draw581/581
but469033/480000 equal RGB pixels; not whole-frame/time equivalence. The exact
Han actor transform is covered by the SDK/RwFrame fixture; it was not separately
read as a live raw actor snapshot.

Native then exits with Machine task type not composed:20 at giGOMTouch(MO004),
before41801. MO004/MO005 contain actual task20 camera quake followed by task15
object activity for dust effects;15 remains a resource finding, not yet reached.
Verdict H:/PAL4/objs/live278-setposrot-verdict.json preserves the failed next
boundary. Original normally closes; no live games/input driver or pending GPU
request remains. Clone slot0 is original live271; no new story save is claimed.

Next isolate the actual MO004/MO005 Machine composition with reused shared
CameraQuakeTask/ActiveObjectTask and existing camera/GOB services, before another
long dialogue replay. Full45-call/global41000 story, full Machine coverage and
all-game equivalence remain incomplete.


2026-10-28 continuation: task20 and15 now reuse shared CameraQuakeTask/ActiveObjectTask; live279 completes all45 func2003 calls and global41000, with both writer limited cold checks in280/281. See ../renderware/m07_machine_quake_activity_native.md for actual scope, frame/CRT limits and remaining M07/4 cache differences.
