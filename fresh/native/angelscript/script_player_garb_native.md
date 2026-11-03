# Story player costume lifecycle

The real Q06/QN05 func2004 path stopped after caption42730 on missing callback
-150 (pair113). Original CSB remains the executable script; supplied GBK Q06.cs
is used only to identify the next operation.

Ownership5D66D0 required game review. Live IDA confirms role0/1/2 requested
models102/104/107; compare current/requested names; for each changed role retain
state+1F0, scriptOnRemove, Script_ProcessAndCleanup, restore nonzero state and
scriptOnAdd, then SetActor only for the current role. IDA's misleading
Quest_InitializeScenePlayers name was corrected to giAllPlayerGarb2_ApplySecondModels.
The shared ScriptPlayerGarb already implements this order and is reused.

NativeScriptPlayerGarbRuntime borrows the persistent player scene, slot text
owner and existing NativePlayerModelReloadRuntime. Text assignment/allocation
retains owned x86 views, rejects foreign buffers and checks capacity before
copying the third requested model. Case comparison, actor package/lifecycle,
animation release/rebind, bounds, FACE/LEG metadata and equipment cleanup all
remain in their existing owners. No model loader, CRT string algorithm or
scene-attachment implementation is copied.

The host shares current-player controller/animation rebind logic with leader
changes. Garb retains the same live scene binding after its real scriptOnAdd;
leader changes still replace the retired previous-role binding. The root and
scene/event callback compositions borrow the same persistent Garb adapter;
per-context animation borrowers and JO display are rebound after current-role
replacement. Shutdown retires the adapter before model reload owners.

The initial change enabled only actualcallback-150. Later-153 composition is
documented below; other Garb callbacks are not implied to be implemented.
Missing services report the exact SDK ID rather than silently succeeding.

Pair114's initial bootstrap was explicitly rejected because both games were
still at their visible normal load-confirmation dialogs, not Scene readiness.
Read-only UI and waiting gates established that state. One additional common
Enter completed the same pending load; the next independent bootstrap was
accepted. No pending request was replayed or force-completed. Cold camera
baseline still failed and remains separate from the following isolated stage.

The normal func1009 entrance and common Space in QN05 invoked actual func2004.
All14 captions42717..42730 passed strict endpoint comparisons. Real Garb2 then
completed and returned control1 at global70200/QN05. Independent
q06-room114-global70200-verdict.json checks all seven float32 groups,
party/48globals/money/full MT/inventory, plus each role's model/requested name,
state flag, position and rotation bits. Both sides have102/104/107; active role0
has state1 and roles1/2 state0. This proves the sampled endpoint, not all frames
or rendered pixel identity.

Both normal SaveWindow writers were archived after actual F7/BtnSaveData/slot0
overwrite confirmation. H:/PAL4/objs/checkpoints/q06-room-global70200-20261031
contains native/original625678-byte archives and provenance. Writer hashes differ;
no complete binary equivalence or pre-save RNG persistence is claimed.
Native source115 and original source116 were each loaded by both games.
Both specified cold field gates passed at0 and63 counted frames. Original-source
116 also passed all three actual/requested models, role flags and pose bits at
both points. No RNG compensation: cold MT is the actual fresh matched process
stream, not required to retain the pre-save stream.

After115's successful cold/63 gate, common UP(.08s) exposed a manual-movement
endpoint fork: original position[-48.7750244141,.680814743,63.1654968262], native
[-48.774765,.680814743,63.1659584], yaw120.003173828 /120.000511. All model names,
party/globals/money/full MT still correspond. Evidence
q06-room-native115-UP-fork.json is rejected and input stopped. This does not
undo the earlier cold-restored field checks or prove the whole movement module.
Next reproduce from70200, not the14 already-passed captions.

Final build3 and full resource q06-garb-20261031-resource1 passed exit0/noSKIP,
GUI12820/12820,478 mute observations on dynamically selected nonprimary1600x900.
Pair114/115 closed normally. Pair116 remains held63 with no input driver and
a PID/creation-guarded audio watcher; identities under H:/PAL4/objs.


Shared Garb regression1/1 and actual compiled AngelScript callback regression
1/1 passed. The latter checks missing-service failure at-150 and the shared
three-role selection/unchanged-model early returns, including original third
text-allocation contract. Initial test selection used a nonexistent suffixed
ctest name and ran no tests; q06-garb-sdk-tests2.log is the actual passing test.
Runtime failure cleanup and all Garb variants still need broader coverage.


## Indexed first costumes, actual120400 room event (2026-11-02)

Pair276 begins from the verified ordinary original120400 writer atcold63,
returns through actualM20 func1001/Q06Q func1009 and entersQN05. Its live EVF
ev_Q06_QN05_1 maps tofunc2004, whose120400 branch callsfunc2023; older next-step
notes sayingfunc2005 were incorrect and are corrected. Walking approaches
leave both games idle; this is an interaction-onlybed event. Earlier QN05
evidence already identifies teleport[90,.7,-60]+Space. These failed approach/
bounded-helper logs are preserved without restarting or changing story state.

ActualSpace starts the event. The firstcaption44396 passes, thennative faults
at8211(granted8212) on missingcallback-153 giPlayerGarb1. Original remainshealthy
and is identity-checked/closed before relinking; no input request is replayed.
Ownership5D6AA0 requiresgame review. LiveIDA confirms indexedfirst costumes
101/103/106 and the same state/remove/reload/add/current-role rebind lifecycle
as the existing sharedApplyFirst/nativeGarb adapter. IDA's generic oldname is
updated toScript_PlayerGarb1_ApplyFirst. The callback now forwards signedindex
toScriptPlayerGarb::ApplyFirst through the existing required player_garb service.
No new loader, actor/model owner, privateCRT string algorithm or replacement
scene path is added. Indexedsecond-costume and other variants remain outside
this change.

Native rebuild succeeds. ActualAngelScript callback1/1 covers allthree input
indices selecting101/103/106 and the existing third-role text allocation seam;
sharedGarb1/1 covers lookup/invalid-index and lifecycle branches. Logs underobjs:
q06-garb1-20261102-{build,sdk-tests,shared-build,shared-tests}.log; liveIDA body
isq06-garb1-20261102-ida.json. This small dispatch follow-up does not add a new
full-resource exit claim; its real scene reloads are verified in the paired run.

Pair277 starts from the same ordinary120400 source, cold0/63 sourcehash/saved
baseline and strict field gates pass. Actualexit/door/Space inputs directly
triggerfunc2004/func2023, without the two failed walking preparations. All77
captions44396..44472 pass text/page readiness and seven worldfloat32 groups,
party/48globals/money/fullMT/inventory/favor endpoints. Actualfirst-model reloads,
facial changes/walking/fades/HP-MP recovery finish naturally at19404:
global120500, Q06/QN05 ready, control1/fade0. Four model/request/state/pose
images match; role0 is101, role1 remains104, role2 is106, role3 remains105.
Onlyrole0 isactive. ItsHP1806/MP109 reach maxima through originalscripted
giSetFullHP/giSetFullMP; no observer HP or result setup occurs. Inventory/favor
are unchanged, allfour learned arrays and role0'sfive forbidden skills persist.

Evidence:H:/PAL4/objs/q06-room120400-277-mainline-verdict.json,
q06-room120400-277-post120500-verdict.json, run-q06-room120400-pair277.log and
q06-room120400-277-finish.log. Initial276 attempts/failure stay in
run-q06-room120400-pair276{,-boundary,-bed}.log andbed/boundary input JSON.
RoomhasnosavepointGOB, so no new ordinary120500 save or cold replay is claimed;
nearest normal checkpoint remains120400. Pair277 isheld19404/no driver. Next
actualQN05func1010 exit loadsQ06Q and its120500 branch callsfunc2024. Older265
allocator/update-order, all-frame/render/wholearchive/pre-saveMT/full-game
limitations remain open.
