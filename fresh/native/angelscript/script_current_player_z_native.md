Latest continuation: pair318 is now closed. See [XML temporary lifetime](../package/xml_attribute_copy_temporary_native.md) for pair319 held109 and the still-open transition CRT gate.

# M15 area2 current-player Z query and transition gate

2026-11-03. Accepted persistent mainline frontier remains global130201.

## Missing callback and ownership

Pair317 restored the ordinary native130201 checkpoint, passed cold0/63, then
used actual ev_M15_1_3/func1001 via paired teleport and movement. Native failed
at completed86/granted88 with `PAL4 game script callback -193 is not implemented`.
The original remained alive until normal close. This is a game callback, not a
library function: IDA46A760 registers Script_GetCurrentPlayerPositionZ/5D7920.
The handler selects manager+2C50 current slot, strideB14, and returns the float
at actor+B4 (or zero for a null selected address). No movement, RNG draw or wait
handler is involved. Static export: H:/PAL4/objs/m15-current-z-ida.json.

The existing IScriptPlayerPositionRuntime boundary now exposes CurrentZ, its
native provider borrows the current player's existing state, and callback -193
returns it through AngelScript SetReturnFloat. The existing Y service is retained.
An unsupported provider fails explicitly rather than returning an invented zero.

## Regression and live verification

The actual AngelScript compiler/SDK dispatch test fails before the callback is
implemented, then passes after the fix. It verifies exact signed float bits,
fresh reads, the M15 strict Z<-1400 predicate on both sides of its boundary, and
no movement or queued work. Native application build and callback CTest1/1 pass.
Logs: H:/PAL4/objs/m15-current-z-{red,green}-{build,tests}.log.

Pair318 uses an immutable rebuilt executable/map/DLL in
H:/PAL4/objs/m15-pair318-bin/ and the same native ordinary save SHA256
6b029115be7d781fd0b55b754551578df2f0039b8d2ccbd9da21bf5620756a25.
Cold0/63 world/camera/character/model/party/inventory/fullMT/monster/helper,
held63 learned/forbidden skills and73669 current-seed CRT calls pass.
Actual func1001 now finishes into M15/2/control1 at held109. Position/direction,
camera float32 fields, party/globals/money/inventory/fullMT endpoint still pass.
Both windows were launched muted/nonactivating800x600 on the dynamically
selected nonprimary1600x900 monitor, without screenshots.

## Open CRT transition divergence

Do not accept the complete transition yet. The continuous latest-seed stream
has130728 original and130735 native calls at held109. Its first kind/value/
classified-site mismatch is ordinal110354: ChangeColorTask versus ScaleTask,
both value7315. Rooted read-only inspection finds equal root ID/name lists and
15 particles, but a different natural inline-object address tree order.
Current reachable task/vtable/target membership already differs at110342:
original15:H_169/rq/ggb versus native16:H_169/rq/ggb. These are distinct root
identities despite duplicate names. Historical pointers inspected at109 do not
independently prove past lifetimes or the earliest historical target mismatch.
No sorting, address remapping, allocator control, reseeding or RNG padding was
introduced. The next investigation must narrow allocation/lifetime ordering
before replaying from the same130201 save.

Evidence under H:/PAL4/objs/:
- m15-second-dragon318-area2-ready.json (world endpoint)
- m15-crt318-current.json, m15-crt318-first-fork.json, m15-crt318-window-*.json.gz
- m15-particle318-area2-held109-owners.json (rooted current natural inline order)
- m15-area2-318-fork-task-current-membership.json
- m15-area2-318-current-task-targets.json
- m15-area2-318-final-held-gate.json, m15-area2-318-final-framegate.json

Live original17032/HWND1050654 and native26776/HWND1051136 are held109 with
remaining0; the transition driver is terminal. Revalidate identities before
reading. Next story func2003 and BOSS are not accepted. No public push of this
runtime fix has been performed.
