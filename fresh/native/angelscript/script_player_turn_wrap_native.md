# Script turn across zero (2026-11-01)

## Live IDA evidence

Ownership gates retain game callbacks5E13C0/5E1620, not a private library
implementation. Live5E13C0 assembly at1509/151C enters1522 only when
current<180 and target>180. The target comparison masks x87 C0/C3 with4100;
less/equal goes to1547, so the decompiler's uninitialized status temporaries
must not be translated as ordinary C++ conditions.

For a delta above180,1536..1543 evaluates -(360-target+current), retaining
both float-operation boundaries. The reverse crossing at1547..1594 requires
current>180 and target<180, wrapping only if current-target>180. Equality at
180 retains the direct branch. This is the original branch tree, not a generic
normalization of arbitrary input angles.

The previous shared TurnDelta incorrectly used current<180 AND target<180.
It also treated current==180 as a reverse-crossing case. Existing test10->350
encoded the same incorrect +340 result. The shared fix now produces -20;
the actual Q06 case132->324 produces -168 rather than +192. Tests include
both crossing directions, ties and equality at180, plus180->-10 retaining
the original direct -190 result. All indexed/current/NPC facing wrappers reuse
this helper, with their existing lookup/math and wait ownership unchanged.
5E1620's speed/duration/interpolation and SDK animation APIs are not replaced.

## Numerical localization

Pairs173..176 start from original80100, execute the real route and CSB, and
progressively observe an earlier part of the43260 divergence. Their earlier
camera/action hypotheses are superseded by the turn evidence below.

- Pair173: camera003 begins10067/10072 and004 begins10177/10182.
- Pair174: C10 completion is9615/9620; J19 completion9945/9950. Bytecode offsets
  agree until C10's end wait, so later SDK/camera progress is not the origin.
- Pair175: C10 already starts8725/8730. Earlier role3 end-action completion shifts
  the actual Q06SY load and ArenaReady by five frames. The world readiness
  guard is not changed on this evidence.
- Pair176: both reach the role3 turn on line4988/frame8385. Original switches C02
  to C01 at8407, native at8412; the source steps beyond the turn at8408/8413.
  C09 then starts8440/8445, both with duration2.466666698. This moves the first
  demonstrated scheduling divergence before C09/EndAction, confirming those
  later waits merely retain it. Source/animation/camera endpoint observations
  remain separate from proof of every animation clock or every world frame.

Artifacts are H:/PAL4/objs/q06-rest80100-pair{173,174,175,176}-phase*.json/log.
All four pairs are closed normally after their required observations. Bounded
observer exhaustion did not imply game exit or trigger a timeout-only restart;
later runs deliberately moved observation earlier using the same near-save.

## Diagnostic boundary and validation

NativeComparisonScriptContext now publishes the existing SDK context's local
address and public current-function ID, plus actual shared queue size/block
flag and action count. Read-only external observers can compare original and
native bytecode positions without inventing context state. The old waits mask
can reflect the same shared queue in several families; it is not a handler
count. No diagnostic callback advances simulation or edits actor/script state.

q06-turn-wrap-build1 succeeds. The focused existing turn suite (including
corrected and added cases) passes1/1; original CSB session/EVF and compiled
callbacks pass3/3. Real-resource q06-turn-wrap-resource1 exits0/noSKIP,
GUI13255/13255, muted observations489. This includes production initialization
and teardown of the unchanged animation/library owners.

Pair177 reloads original80100 and executes the actual M20 exit, Q06 func1077
room entry and bed func2011. All49 captions43224..43272/49 page endpoints pass
text/readiness, seven float32 position/camera groups, party/48globals/money,
full MT/inventory and the four-role favor matrix. This includes the previous
43260 failure and the later NPC actions/walks, camera waits and load transitions.
Natural Q06/Q06SY80400/control1/fade completion passes1922403740; four-role
model/request/state/position/rotation words and favor pass797616551.

Normal F7/System Save exposes attached, visible, enabled SaveData on both sides.
The first read-only preflight used the wrong offset for a verified DLL getter
signature; it stops after the one F7 input, then the same open menus are
inspected correctly without another input. Both actual slot0 overwrite prompts
are verified and confirmed serially. Archive
H:/PAL4/objs/checkpoints/q06-rest-global80400-20261101 contains two705279-byte
writers: original fd908f0c1275572698b1c2de7f15b65a2670ebeb19a2cf1a7b9c8d9a92eefbfb,
native9f20275fe37629053741f5eeb7be3bf16ccaf29d4563d0ff9bb5c2151e03f25a.
Save exit632517721 again passes the listed fields and all four role models/
states/pose/favor. Pair177 is normally closed. Native-source178 cold0/63
passes107432631/1074153454, with four-role model/state/pose/favor checks
661088047/30932230. Original-source179 cold0/63 passes1225729307/493069116,
with model/state/pose/favor1282528145/156666639. Each source SHA256 is checked
against the common slot before both games load it. Saved leader position/yaw,
party/globals/money/inventory/favor and bilateral seven float32/full fresh-seeded
MT fields pass; independent original Q06/Q06SY ready1 and both control1 pass.
Inactive role3 restores zero pose on both sides rather than retaining its
pre-save cinematic pose; no forced pose restoration is used.
Both writer sources pass the listed cold fields. The shared slot holds
original80400. Pair178 is normally closed;179 remains held at cold63,
original147760/native125640, identities q06-descent-original179-identities.json.
Use this near-save for later mainline work without replaying the49 captions.
Whole archive byte identity and pre-save MT continuation are not claimed.

No clock, yaw, camera, RNG,
story or inventory compensation is used. All generated artifacts stay in objs;
runs are muted/nonactivating800x600 on the nonprimary1600x900 monitor, without
screenshots. Full-game and previously documented patrol/CRT/M11/render/save
limitations remain open.

## World animation clock follow-up

Turn-delta reconstruction above is unchanged. M14 pair259 separately exposed
duplicate native blend advancement after a turn. The world-clock ownership
fix and actual pair260 retest are recorded in
[script_turn_animation_clock_native.md](../renderware/script_turn_animation_clock_native.md).
The earlier script-only blend driver now yields to the existing per-actor
world animation clock; this does not accept the remaining FL/FR or43980 gates.
