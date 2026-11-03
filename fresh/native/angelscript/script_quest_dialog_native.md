Current update (2026-10-20): original Q02 slot2 loading, actual scene/source
and NPC4021 presence, original DLL selector Task -> production timed SDK ->
original Buiness_Q02_5001 -> Quest/state10 -> No/Yes -> Scene/global write are
now verified. Marker9 and Action snapshot native gaps and Machine unknown-action
restore were resolved. This supersedes the older Q02 GOB blocker/selector-pending
claims below. Test opens the selector explicitly; original NPC interaction and
whole func5001 remain pending. NPC concrete atomic tasks remain pending.
See ../package/gob_marker_action_native.md for scope/evidence;
CTest pal4_renderware_application_q02_shop_original passes13.14s. Shared4 pass0.18s.

# Native Quest SDK callback and original task acceptance

Verified increment, 2026-10-20. Callback -207 giShowQuestDialog and -208
 giGetQuestDialogResult now use existing ScriptQuestDialog. Its CreateDialog
interface method was renamed CreateQuestDialog to avoid the Windows API macro;
all consumers and shared tests were updated.

NativeScriptQuestRuntime borrows one actual SDK context, the native arena XML
interface and required actual dialog/current-state services. It validates its
executing context before creating/showing UI. ScriptFrameStateDocument still
owns frame-key lookup, attribute add/update, temporary destruction and state10
entry. Native XML handles and bit-preserving32-bit frame attributes replace
private VC6 containers. The SDK context suspends after real state10 entry and
resumes only after Scene1 returns. UI close alone does not release continuation.
Cancellation clears only the context's wait and never closes a foreign/global
window; retiring an executing context is rejected. Existing opening waits
aggregate Quest polling, blocking and unbinding. Primary, event, concurrent
scene and timed contexts receive separate adapters; loaded scene services rebind
the primary owner. Singleton CommonDialog2 and state10 services retire before UI.

## Unmodified original bytecode acceptance

The complete application queues Q02.csb/Buiness_Q02_5001 through its actual event
runner, with actual context services and original CSB loader. The task function
has no NPC movement dependency. No decompiled source is compiled as replacement.
Both choices execute actual Quest callback -> CommonDialog2 -> ModalFrame10.
The context stays suspended through visible frames and after the DLL choice
closes the window while state10 is still active. The next modal update observes
close, clears frame ownership and returns Scene. The same event SDK then executes
-208 and the original conditional/global store. No preserves0, Yes writes1 to
var_shop010 at native PALGameIV's mapped original90DF38. Four modal render frames,
world/control freeze, config destruction, state/window retirement and final render
accounting are checked. The fixture restores the prior global value, restores
current-world script selection using a queued/cancelled original event, and
retires its UI services before subsequent cave progression tests.

This is original task-function acceptance from the current loaded Scene. It is
not Q02 NPC4021 interaction, its whole func5001 branch, or the selector's task
button/native stack route. Those must run with the corresponding scene/source
and actual event trigger. They remain pending.

## Verification and limits

Build6/test4 passes complete original-resource application15.46s (CTest15.50s).
Build3/test2 passed application15.89s plus native arena/state boundary and SDK game
checks (three total15.97s). Build5/test3 relinks five affected script checks, all
pass0.16s: global bindings, trade wait, callback dispatch, frame runner, flash wait.
Build7/test5 reuses the shared Quest boundary test and passes0.02s (CTest0.06s),
including XML branches, fallback strings/order and exception cleanup.
The first application check incorrectly looked for an external registered global
inside module locals; this exposed both missing global registration and an old
parser semantic-name error. See global_variables_native.md for the correction.

Original task variable reset/full persistence, NPC/shop-button route, all task
variants, full visuals/combat/save/load/game progression remain unproven. State10
for other frame classes such as Inn also needs actual production acceptance.
Tests stay hidden, muted, nonactivating on the dynamic nonprimary1600x900 monitor.
TEMP/TMP and all artifacts are under H:/PAL4/objs. No play version launched.
Evidence prefix M09-quest-state-20261020- includes final-application-detail.log.
