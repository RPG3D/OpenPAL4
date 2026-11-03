# giScriptClearCTXButCurrent native boundary

2026-10-30: actual M08/2 floor22 helper contact invoked the real func7004 via
the original CSB/Machine task. Native exited with missing callback -177, while
original completed the trap teleport and restored input. This is a runtime
integration gap, not a reason to invoke the story function directly.

Ownership gate:5D7510 and7DF340 are not individually excluded library entries.
Live IDA5D7510 checks the selected execution cache count at executor+54, and
only when greater than one calls7DF340. That function preserves entries whose
full cache name compares equal to the selected name via CRT stricmp, stops
and releases other SDK contexts, compacts the records, then cleans handlers.
Loaded SDK bytecode modules are distinct from these pending execution entries.

The native callback reuses ScriptCacheContext::ClearExceptCurrent and
ScriptExecutionContext::RemoveOtherStackModules/CleanupHandlers. The adapter
collects queued/suspended root, event, scene and timed executions. FrameRunner
retains the original full cache identity; external keys already ending in
[function] are not suffixed twice. The selected record is copied to stable
storage while the shared policy compacts its temporary record span. No CRT,
private SDK context internals or private container algorithm is reconstructed.

Other executions stop through public SDK Abort. Native reusable root/scene
contexts remain alive while wait adapters borrow their addresses. Timed owners
clear their actual waits and retire cancelled entries/modules after policy
cleanup, retaining a still-live shared module. The host clears actual root and
scene handlers. The current executing context is never aborted or released.
Its callback requests SDK Suspend; the runner flushes retained/current wait
cleanup immediately after SDK return and before another script instruction.
This preserves native wait-owner guards and prevents later instructions' new
waits from being accidentally cleared. Full execution resumes after cleanup;
stepped execution retains its normal instruction boundary.
Zero/one entry follows the original count gate without handler cleanup.

Scene busy state now depends on pending executions, so retained idle contexts
after cancellation cannot suppress future timer dispatch. Existing nested-scene
regression exposed an earlier ordering defect: case-insensitive duplicate cache
checks must precede a case-sensitive new SDK lookup. The duplicate check moved
before lookup; absent initializers still return before affecting prior waits.

SDK tests exercise real compiled -177 callbacks, suspended-parent cancellation,
live timer cleanup, current continuation, stable borrowed context lifetime,
single-entry gate, case-insensitive equal-name retention, rejected calls with no
selected active context, and nested scene pop without resuming cancelled code.
Nested timed regression additionally uses a factory that deliberately drops
the inherited cache service. Entry binding now restores the session-owned
cache after that factory, just as it restores the selected timer. The regression
cancels a real waiting timed parent, executes its nested child, releases the
cancelled module and proves the parent cannot later resume. Initial static SDK
tests lacked that application-factory path; the first live retry therefore
still failed with unavailable service -177. Its evidence is retained, not a pass.
Final build/test/live evidence is recorded below when available.

Evidence preparation is under H:/PAL4/objs/:

- m08-2-monsters-prefixes.json: unique resource prefixes, not complete MST parse.
- m08-platform-isolation-{original,native}-monsters1.json: read-only actual scene
  monsters; one patrolling monster differs, CRT/patrol alignment remains open.
- m08-2-all-platform-config.json:50 unique type2 config records; floor21 has no
  trap task, while floor22 has script2/func7004/wait_mode1. Earlier choices
  floor01/floor46 were within normal monster proximity and did not test traps.
- m08-floor22-contact1.json: both Scene after real relocation; common UP invokes
  platform helper. Native then exits during callback; key release journal
  console-input-1477901839.json preserves the native release failure. No extra
  input was sent afterward.
- m08-floor22-original-completed1.json: original ends at
  (510.3500061035156,8.039999961853027,798.1599731445312), yaw270, input enabled.

Original99560 was closed normally after capturing that terminal state. Native
105480 had already exited; these old identities must not be reused.
Nearest accepted disk checkpoint remains M08/1/global50600. Full game, CRT
initial drift and the actual post-fix trap comparison remain open.

Post-fix retry2 reached past -177 and then failed at -107/giPlayerGetLeader,
before the player blend calls. Native leader query now forwards the existing
ScriptPlayerStatus::GetLeaderState; player blend entry bindings are integrated
with the existing scene-membership/material adapter. The user's Loading/fade
request took priority before another actual trap retry, so func7004 completion
is still unaccepted. SDK cache tests now include nested timed factory replacement
and a following giWait that must survive callback cleanup. Final relevant logs:
loading-fade-final-target-tests1 and loading-fade-resource-smoke1 (exit0,
GUI13252/13252). The callback/cache changes were built, not injected into native.
