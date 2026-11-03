# Native script random player position and integer RNG

2026-11-01. Pair135 naturally reached M11 weather func7001 after normal70700
save load and exited at missing callback-148. This batch connects existing
ScriptRandomPlayerPosition through actual SDK callbacks-147/-148. Pair138
subsequently passes that callback and fails at missing-89; this batch also
connects the existing ScriptWorldMisc integer range policy for-89.

## Ownership and live IDA

All selected game entries were classified before live decompilation/disassembly:
5D61A0/5D6400,5D6150,521510,666550,661A50,668370/6683C0 and5D4000 all require
game boundary review. Public CRT rand/srand and existing RenderWare APIs remain
library-owned, with no private CRT/collision/random implementation added.
Live player vtable842F10+4 is666550. The verified5D6400 name/comment in IDA now
describes current-player random position rather than general movement.

5D6400 selects the current leader at player manager+2C50, generates three
SignedUnit(0) components and InRange(min,max,0), probes a vertical segment+/-100,
rejects kind1 EVF clumps via5D6150/661A50, and tests the candidate with existing
movement CheckActive. Up to100 candidates are tried. Success uses the base
vtable+4 transform666550 and updates minimap with actor+AC/+A0. It does not use
the vtable+60 camera/audio position override. Indexed-147 shares the existing
candidate algorithm.521510 forwards the manager's record pointer directly;
record+4 is kind and record+44 is clump, not its separate factory resource.

NativeScriptRandomPlayerPositionRuntime borrows the existing player-position
lookup, live EVF manager, NativeScenePlayerMovement, NativeClumpCollisionQuery,
NativePlayerTransformRuntime and NativeMinimapManagerOwner. The application
retires this borrower before replacing its position or movement owner. Shared
context services expose it to root/event/concurrent/timed original-CSB calls.
No script text replacement, story/RNG writes or deterministic relocation shim.

Live668370/6683C0 FCOMP0/FNSTSW/AND4100/JNZ skip srand for zero, negative and
unordered seeds; only positive seeds reseed. The old shared GameRandomMath
incorrectly used>=0. It now preserves zero-seed state and explicit PC24 rounding
at the original arithmetic operations. The integer-89 policy5D4000 is exactly
min + rand() % (max-min), upper bound exclusive. A narrow random-integer
interface lets this existing policy use the public host CRT (normal tracing)
and permits deterministic SDK boundary tests without a private RNG.

## Validation

- m11-random-position-core-tests2.log: random math, random player position and
  world-misc3/3 pass, including zero-seed no-reseed, candidate retry/exhaustion,
  event rejection, indexed/current selection and integer interval semantics.
- Real AngelScript SDK compilation/execution in
  angelscript_game_callbacks_tests verifies current/indexed callbacks, rejected
  first ground candidate, floor-adjusted transform/grid, missing player without
  RNG draw or write, and integer RNG's single draw/exclusive modulo359.
  sdk-tests1 had a wrong ctest name and ran no tests; sdk-tests2/3 execute the
  actual rebuilt executable directly and exit0. No empty ctest acceptance.
- Final m11-random-position-app-build2.log passes. Original-resource resource2
  exits0/noSKIP, GUI13255/13255,481 mute observations.
- Pair138 original70700 cold+63 strict fields pass request1452312635. Natural
  weather differs at sampled frame1563: original already moved, native remains
  in fade-out. A bounded diagnostic continuation is explicitly not paired
  acceptance; native passes-148 then reports missing-89 at frame1611.
- Pair139 includes-89. Original70700 cold+63 passes request802902931. Natural
  weather locks both and completes both by sampled frame2223/request1125183659;
  native random relocation/orientation/fades return control without missing
  callbacks. Its event onset and final position differ from original, so the
  weather sequence is NOT numerically accepted. Full CRT stream/timing remains
  unresolved; matching combat MT does not prove CRT agreement.

All artifacts are under H:/PAL4/objs, especially
m11-weather138-first-divergence.json, m11-weather139-natural-diagnostic-journal.json
and m11-weather139-completion.json. No diagnostic screenshots. Muted,
nonactivating800x600 windows use the dynamic nonprimary1600x900 display.
Both pairs closed normally. No altered gameplay checkpoint was saved; shared
slot0 remains original70700. Cold0 camera-eye divergence on both saved sources was subsequently fixed
and accepted in141/142; see ../renderware/camera_before_arena_ready_native.md. Next mainline is actual M11 func1001 -> Q07 opening;
post42864 has not been accepted. Do not claim complete game/save/CRT/pixel or
all-frame equivalence from these scoped tests.
