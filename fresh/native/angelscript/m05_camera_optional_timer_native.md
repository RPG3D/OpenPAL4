# M05 camera precision and optional timed script entries

2026-10-27. Source scripts are read directly as GBK from
`I:/Games/original/gamepatch/gamedata/EditData/script/M05.cs`.
The runtime continues to restore the original CSB through the existing package loader.

## Observed failures

live175 completed the 15 unique M05 opening captions 41208–41222 in order on
both games, with matching Scene/control, actor pose and full MT state. Camera
target/eye Y differed by one float step. Native later exited when the repeating
180-second timer requested absent `func9001`; original remained alive.
These failures are retained in `H:/PAL4/objs/live175-work-state.json` and its
referenced captures. The genuine global30500 save was written by original only;
it is not yet a paired accepted checkpoint.

## Camera correction

IDA 443F90 computes `(height + height) * float(0.9) + positionY` under x87 PC24.
Both live main threads had FPCW007F. The native helper used double intermediates;
it now uses float intermediates for each original operation. The regression
uses height bits421CFE9D and positionY -315.6690979003906, expecting targetY
bitsC3750584. Live verification remains required.

## Optional entry correction

IDA 7DEE60 returns asNO_FUNCTION(-6) before parent suspension, wait cleanup or
stack insertion. The repeating timer600B90 and countdown47CAA0 ignore that
return, release the interpreter and clean modules. Other lookup failures are
distinct errors.

NativeAngelScriptEngine::HasFunction now shares the existing scene optional
lookup rule with NativeTimedScriptStack. The timed stack checks before context
creation or parent mutation and discards an absent synthetic module. Repeating
timers remain enabled. Empty names and ambiguous overloads remain errors.
The strict frame runner is unchanged. The production loader reloads CSB on
each request and has no synthetic-module cache.

## Validation

Build2 succeeded. Four targeted tests passed in 2.18 seconds:
base object scene parameter, native script timer, AngelScript frame runner,
and native application entry. Tests cover missing repeated callbacks, parent
wait preservation, countdown absence, later available callbacks, invalid names
and ambiguous overloads. The first run failed an obsolete assertion expecting
missing countdown callbacks to throw; it was corrected after checking IDA47CAA0.

Build/test logs: `H:/PAL4/objs/m05-camera-timer-20261027-{build2,test2}.log`.
Resource gate passed with GUI10866/10866 and normal shutdown. Live176 cold-loaded
the genuine global30500 original save on both games with matching pose/camera
bits/full MT624. Shared relocation to the old failure position reproduced the
regression setup: height and root LTM position matched, camera targetY now
matches original C3750584. This setup is not movement fidelity evidence.
Live176 also survived the actual180s timer trigger, emitted the absent-entry
diagnostic, and retained Scene/control1, matching pose/camera bits and full
MT624. Read-only original timer observation confirms it remained enabled with
delay180 and callbackfunc9001 after wrapping. Evidence:
`H:/PAL4/objs/live176-m05-timer-verdict.json`. Paired save writer acceptance
and onward M06 remain pending. Original retains
the explicitly selected process-only DisableOriginalHeapTrim test condition;
stock allocator failures remain open. No full-game completion is claimed.
