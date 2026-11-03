# Player lock while monsters are pursuing

Live197 from verified31100 checkpoint reached light1_f/func7002 using common
relocation and one actual shared Space. Native exited before the first camera
with "stopping active monster pursuit requires native return path". The original
was normally closed; the first fire has not been accepted by this run.

The fault is in giPlayerLock, before the explicit giMonsterStopPursuit that
follows in the script. NativeScriptPlayerLockRuntime's ResetRuntime called
NativeMonsterManagerOwner::StopPursuitAll(0) without paths. Idle monsters hid
this omission; actual pursuing monsters require EnterReturn.

Ownership classification423AC0/433830/433800 requires review. Direct IDA
423AC0 calls MonsterManager_SetStopPursuitFlagAll on lock;433830 writes stop
flag+704 then, when+730!=1 and phase+6E0==1, calls the real return-state path.
These are PAL4 game glue, not a missing third-party library implementation.

INativeScriptPlayerLockHost now supplies StopMonsterPursuit for the manager
passed by the lock runtime. The application composes the existing
NativeMonsterPursuitPathRuntime with the current collision tree and monster
allocation memory, exactly as existing World Scene entry already does. The
shared manager walk, stop flag, return state and CollisionActor path algorithms
are unchanged. A missing arena is accepted only with an empty manager.
The stack path adapter stores no long-lived borrowers; path buffers remain
owned by existing scene memory. No forced idle, snap-back or chase suppression.

The monster-resource probe uses EnterOwnedPursuit to enter the real owned
chase state, then compiled giPlayerLock twice and giPlayerUnLock. It requires
stop flag, exit from chase, cleared target, no repeated allocation growth, and
restores the fixture with the existing pursuit reset. The live pair gets no
external phase/HP/RNG writes. Build1 passed; monster1/full resource gates and
actual M06 replay are pending.

Evidence H:/PAL4/objs/compare-rebuilt-20261027-062324.err.log and
live197-light1-start-capture.log. Checkpoint remains
H:/PAL4/objs/checkpoints/m06-rescue-global31100-20261027-live195.

Validation: build1 passed, actual monster1 regression passed (GUI2189/2189),
full cave-memory resource1 passed (GUI12890/12890), both normal shutdown.
Live198 actual first fire no longer exits at player lock and returns control.
The broader endpoint check then found an independent mirrored yaw, documented
in m06_facing_hemisphere_native.md; the whole fire remains unaccepted until
that correction is replayed.
