# M06 healing attachment callbacks

Current acceptance (2026-10-27): live195 completed all25 captions41253–41277,
returned Scene/control1, Lingsha rejoined and global31100. Both games match
pose/direction/all camera float32 bits and full MT. Normal YuanZu item saved
both309457-byte writers/global31100/arena240; live196/native source and
live197/original source cold-restored on both games with matching three-party UI.
Checkpoint H:/PAL4/objs/checkpoints/m06-rescue-global31100-20261027-live195.
Current live197-work-state.json is the continuation entry; earlier pending notes
below record development history. Full timing/save/RNG persistence remains open.

Live194 continued from the genuine30900 trap checkpoint. Both games crossed
func1003 into M06 geometry2/resource2A, then actual shared movement crossed
ev_M06_2_7 to func2002. Captions41253–41262 matched (10 captions), NPC waking
was reached; native then exited with callback-168 unimplemented at
giEffectAttachToPlayer(2,"H_TL03.2",false). The original was closed normally
after the input driver stopped. No full rescue-scene acceptance yet.

Ownership review5D71B0/5D7200/5D7260/5D7280 returned review-required. Direct
IDA confirms PAL4 glue using PlayerManager and actor effect methods. Indexed
attach resolves the player; true selects4437A0 left hand, false4436E0 right hand.
Current attach reads manager+2C50 and uses slot strideB14. Indexed/current detach
invoke443900 UpdateWeaponEffects, not weapon model cleanup.

Reused ScriptPlayerGobAttachment, call-local PlayerHandArguments string projection,
NativePlayerGobAttachmentRuntime and existing equipment WeaponRuntime. Dispatch
now binds-168..-171 without a new effect owner, filename rewriting, raw SDK
string emulation or original CSB changes. Existing scene/context bindings already
carry this same player attachment owner.

Compiled callback probes cover indexed2/current, both bool values, exact original
H_TL03.2 name, empty strings, absent indexed player, effect-only detach and missing
services. Real-resource regression uses original106 model/right-hand hook,
H_TL03.2 creation, update/draw, repeated detach/drain and package balance.
Build1 passed; standalone SDK test1 passed1/1 (0.11s), and full application
resource1 passed with normal shutdown, GUI13082/13082, including the compiled
-168..-171 probes. The added original106/H_TL03.2 resource probe belongs to
the actor animation executable; this separate gate remains pending.
Actual func2002 replay must pass after the gate; no forced healing/HP/RNG flags.

Evidence H:/PAL4/objs/live194-rescue-dialogue-journal.json,
live194-rescue-advance1.log and compare-rebuilt-20261027-053754.err.log.
Reliable checkpoint remains
H:/PAL4/objs/checkpoints/m06-trap-global30900-20261027-live192.
Both writer cold-restores were accepted only for the documented visible/pose
checks, not complete save fields or RNG persistence.

## Resource fixture correction

A proposed early focused actor-test entry exitedC0000005 before acceptance; it
was removed. The established complete actor-resource path then reported
BillBordObject requires a bound scene camera. H_TL03.2 introduces a billboard
into the weapon effect fixture, whose previous effects did not require a scene
camera binding. The fixture now binds its actual SDK probe camera before effect
creation and keeps it alive through effect session teardown. Logs retain both
failures; actor-build3/resource3 are the next verification. This does not change
the application's scene camera or claim the live rescue already passes.

The first camera-bound full run still passed0 as draw context. CDB records the
read at4 through ValidateObjectTransform in the distance-gated nested strip;
the fixture now draws with its real RwCamera pointer. Resource4 then reached
the new detach assertion: shared EffectHandle::Reset uses idFFFFFFFF/system0,
where the new assertion had incorrectly required id0. The test now requires
the exact reset sentinel, no active handle, and natural pending drain. No product
algorithm was changed to satisfy these fixture requirements. Final actor gate
and M06 actual replay remain required.

## Gates completed

Actor-build5 and complete actor-resource5 exited0, including original106/H_TL03.2
right hook, actual update/draw, repeated detach with FFFFFFFF/0 handle reset,
natural pending drain/package balance, and desktop focus/cursor preservation.
The existing actor animation/motion/reset suite also passed. No quick-entry
argument remains. Application build1/SDK test1/full resource1 already passed;
subsequent changes affected only the independent actor fixture. Live195 actual
rescue replay now starts from30900, not NewGame or a maze replay.

Final validation: application build1, SDKtest1 (1/1,0.11s), full application
resource1 GUI13082/13082 and complete actor build5/resource5 all passed.
The actor fixture now binds and draws with a real scene camera, preserves it
through teardown, and checks the existing FFFFFFFF/0 effect-reset sentinel.
No focused test CLI or CMake shortcut remains. Actual paired treatment capture
live195-rescue-step10-boundary2-1781022.png and complete verdict
live195-rescue-verdict.json are under H:/PAL4/objs. Current next step is normal
fire mechanisms, not re-running completed rescue or maze traversal.
