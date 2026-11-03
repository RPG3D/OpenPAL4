# M06 GOB fades and timed movement

2026-10-30: the same ordered context queue now also hosts the current player's
849684 timed move, borrowing the existing player animation/position services.
Actual M07 ladder and post-ladder story validation is recorded in
[current-player timed movement](current_player_timed_move_native.md).

2026-10-27. Live181's actual light01_f/func7001 stopped at callback-302.
Existing GBK M06.cs is read directly from the supplied gamepatch script folder;
the runtime still executes unchanged original CSB.

## IDA evidence

Ownership classification required review for5E72E0/5E73D0/5E74A0/5E7500/5E7620,
5E7CF0/5E7EC0,5E3300/5E8CE0,59CA30 and666550. All were checked in live IDA.

- Fades use44-byte handlers849618/849624, the actual GOB+184 active field,
  GameClump_SetMaterialRGBA and the existing arena remove/add callbacks.
- Fade-out virtual+8 is5E74A0: restore full RGBA, remove if active, close.
  Fade-in virtual+8 is5E3300: restore RGBA and close while retaining membership.
- Timed movement uses68-byte handler849660, marker5 and name-OBJ-Movment.
  Its virtual+8 is5E8CE0: close without snapping to the destination.
- GobEffect position virtual59CA30 is a thunk to666550, the same existing
  GameResourceManager transform algorithm used by normal GOBs.

## Binding and ownership

Callbacks-302/-303/-308 reuse ScriptObjectBlend/ScriptObjectTimedMove. The new
NativeScriptObjectEffectsRuntime owns one context's typed waits and host strings.
Blend and move adapters share an ordered queue and reuse the existing
ScriptExecutionContext::AddWaitHandler, including marker/name replacement.
ScriptWaitHandlerBase handles construction, suspension flags and closure.
No VC6 string allocation/destruction or new interpolation algorithm is added.

Scene lookup/add/remove reuse NativeScriptObjectVisibilityRuntime and its GOB,
movement and minimap owners. Material operations use NativeVisualModelRuntime
and SDK traversal. NativeGobObjectImage exposes its existing shared transform
path; the Machine setter reuses it too. Owned-object and manager validation
precedes access. Primary, event, concurrent and timed context construction binds
the appropriate owner and wait runtime; explicit cleanup precedes scene release.
Ordering across other independent wait services remains a separate open scope.

## Tests and actual completion

Build1 succeeded. SDK/application2/2 passed1.82s. Existing shared blend/move2/2
passed0.16s. Full cave-memory-smoke passed, GUI11058/11058, normal shutdown.
The real MO001/SDK regression covers suspended script continuation, half-alpha,
completed removal/re-addition, actual root LTM movement, hidden-object no-op,
same-name move replacement, partial move cancellation, distinct fade cancellation,
zero-duration fade-in, missing names and invalid manager/time. The fixture restores
its original pose, material colours and scene membership.

Live182 repeated genuine30500 load, vine entry and shared mechanism setup/Space.
Both naturally completed func7001 and returned to Scene/control1. Final actor
pose/direction, camera bits, fullMT624 and five GOB active/position/root-LTM
samples matched: fog/light01/light01_f/cover02 inactive, Light_u2 active.
Evidence: `H:/PAL4/objs/live182-mechanism-verdict.json`.

**Remaining fidelity limit:** paired capture1620201 shows different intermediate
camera phases. Final agreement does not prove intermediate scheduling, waits,
animation or complete pixel equivalence. The discrepancy is retained for replay;
this is a bounded functional completion, not full timing acceptance.

Normal SaveWindow writes archived both301264-byte global30500/arena235 saves in
`H:/PAL4/objs/checkpoints/m06-fire-mechanism-global30500-20261027-live182`.
Native-source live183 cold-loaded on both games with pose/camera/fullMT agreement
and the five mechanism flags retained. Hidden fog position resets to its original
config on both games when loaded; its final animated position is not claimed to
persist. Original-source live184 also cold-loaded on both games with the same bounded
checks. Both live games remain responsive at the saved M06/1A position, no driver
runs; current state is `H:/PAL4/objs/live184-work-state.json`. Full serialized state/RNG
persistence and the full game/modules remain unproven.

Logs: `H:/PAL4/objs/m06-object-effects-20261027-*`.
