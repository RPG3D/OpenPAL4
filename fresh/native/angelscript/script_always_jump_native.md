# Native giAlwaysJump binding

M10_1_init calls giAlwaysJump(true) after the opening func2001 returns and
on its restored branches. Callback-132 had a registered signature but no native runtime dispatch.
Ownership review/live IDA5D58A0 identifies game glue: obtains real ActorControl
through4235A0 and writes the unsigned input byte as a dword at+110. Reuse the
existing shared ScriptAlwaysJump::Set implementation; no jump algorithm is
invented by this binding.

NativeScriptPlayerLockRuntime already borrows the actual ActorControl owner.
It now also implements the narrow IScriptAlwaysJumpRuntime interface; its new
write endpoint accepts only that constructed owner's+110 field. Main/event/
concurrent/timed script service copies bind the same creating owner through
existing service setup. Scene replacement refreshes these services through
the existing player-lock lifecycle.

The actual SDK resource fixture compiles giAlwaysJump(true/false), compares
the complete ActorControl word image after each of four toggles, rejects an
adjacent field write and restores the original field through shared Set. This
is a callback/ownership probe, not full jump motion/collision verification.
Final build/resource results and live M10 endpoint flag remain pending.


Combined application build3 and full original-resource resource2 passed:
exit0, no SKIP, GUI12815/12815,472 mute observations on dynamically selected
nonprimary1600x900. The actual calCtrl strict timer/script observer/reverse/
helper/reset and compiled AlwaysJump probes printed their passes. Actual
Q06/M10 retrial remains pending; no M10 mainline acceptance is claimed yet.


Pair62 Q06->M10 actual init reached61001 after19 caption checkpoints;
625369691 additionally reads real ActorControl+110 through both validated
process identities and finds1 on both. Full flag field and the settled
numerical gates passed. This is not all jump trajectories/collision cases.
