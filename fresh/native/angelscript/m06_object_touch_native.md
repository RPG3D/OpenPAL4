# M06 scripted GOB touch

Live191 reached M06/1B func2001 through the real floor boundary. The common
movement crossed ev_M06_1_3 and both games displayed captions41223–41238 in
order (16 captions). Native then exited at callback-102 after showing Jeffect001;
the original continued. This is not a successful whole scene acceptance.

IDA5D4D50 looks up the named GOB and calls virtual+0x50 with the current player
slot+0xAC, re-reading the current index after lookup. Existing ScriptObjectTouch
already reconstructs this. Effect vtable848C18+0x50 points to59C770: read saver
flag+0x1F4 and request state9 only when nonzero. It does not restart the effect
or invoke a scene script merely because its name is Jeffect001. Library ownership
classification returned OWNERSHIP_REVIEW_REQUIRED; the inspected functions use
PAL4 managers, GOB dispatch and game state transitions, so this is game glue.

NativeScriptObjectTouchRuntime borrows the persistent player/GOB/memory owners.
The script callback delegates to shared ScriptObjectTouch and the existing native
object interaction dispatch. ActorControl selection retains its enable check;
scripted virtual invocation bypasses selection as in IDA. The shared TouchObject
path retains Action, Effect saver and Normal object behavior. Primary, scene,
event and timed contexts inherit the same persistent binding. Scene teardown and
player shutdown retire the adapter. No original CSB edits or forced story/HP/RNG.

Compiled SDK regression covers-102, the original name, selected player offset,
re-reading the slot, unavailable services and missing objects. Real resource
probes reuse Q01 effect001 with saver0/saver1, disabled selection and missing
name, then restore all fixture flags; these are resource tests, not live writes.
Build2 and the compiled SDK regression passed (ctest1/1,0.09s). Full installed-resource
cave-memory-smoke resource1 exited0 with GUI13330/13330 and PASS, including the
new real GOB probes and normal cleanup. Live192 actual M06 replay completed all30 captions41223–41252 and returned
Scene/control1. Position/direction/all camera float32 bits and full MT match.
The actual System party UI confirms Lingsha left; the original-writer cold restore
also directly reads global_mvar90DEC0=30900.
Build1 failed
linking because the selective SDK library did not yet include shared
script_object_touch.cpp; its source list is now corrected.

Evidence: H:/PAL4/objs/live191-trap-dialogue-journal.json,
live191-trap-advance1.log and compare-rebuilt-20261027-044208.err.log.
The original was closed with WM_CLOSE after the native exit. No new checkpoint
was written; completed fire checkpoint live182 is the shortest existing replay.

## Checkpoint continuation

Both games normally used one YuanZu item through the same UI; after leaving Save
both counts changed5→4. Serial original/native writes each produced304421 bytes,
global30900/arena236, archived under
H:/PAL4/objs/checkpoints/m06-trap-global30900-20261027-live192. Native writer
was cold-loaded by both games in live193; original writer by both in live194.
Both restore Scene/control1, matching pose/camera bits and MT, with Tianhe and
Mengli party UI and matching displayed stats. Full save fields and RNG persistence
are not established by matching two fresh processes. Live192/193 closed normally.
Current live194 stays muted in System UI with no driver; details
H:/PAL4/objs/live194-work-state.json.
