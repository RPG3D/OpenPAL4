# Native script SelectDialog and ModalFrame continuation

## Observed mainline boundary

Pair60 cold-restored Q06/QN03/global60901 (803702923), relocated both
controlled actors to the real MN001 interaction range, then sent the same
Space. Original entered func2102 and its SelectDialog question/Yes/No rows;
native terminated explicitly on missing callback-162. Keep the paired input,
q06-trial60-before-interact.json and original selection snapshot. Original
was subsequently closed through its verified PID/HWND, without choosing a
unilateral story option. The reliable60901 archive is unchanged.

## Composition and evidence

Ownership review and live IDA confirm5D6E90/5D6EB0/5D7FD0/5E7B20 as game
UI/script glue. Reuse ScriptSelectDialog, ScriptDialogs, the existing native
ModalFrame state10 and original XML service. NativeScriptSelectDialogRuntime
owns a single SDK context wait. Four actual callbacks-162/-163/-206/-307
bind to it; main/event/concurrent/timed contexts each retain their own adapter
through the existing context execution owners and wait aggregation. Strings
are synchronously borrowed as string_view tokens, never treated as legacy SDK
string layout. The original SDK and public DLLs remain in use.

ShowSelect creates/reuses the real UI owner, shows the question, queues and
suspends its SDK wait, constructs an XML document with frame pointer, enters
state10 and destroys the temporary document. No special Q06 branch or direct
story-variable/result write is used. Wait polling uses the actual cached
window attachment/visibility; cancellation retires only its context wait.
NativeSelectDialogLayout owns the real SelectDialog.xml dynamic panels and
public CEGUI event subscriptions; see ../ui/select_dialog_layout_native.md.

Live5E7B20 resolves aFrame_0 at8B0528 to lowercase `frame`. The former shared
`Frame` spelling was wrong and could not feed state10's actual lookup.
ScriptDialogs and its existing tests now use the original lowercase key.
Live4B9B83 assigns the current count on each loop boundary, including the
terminating iteration;4B9D68 positions the footer using count, not count-1.
The shared layout and tests preserve this boundary including an empty list.
Esc returns the saved+A0 default; initial navigation remains index0. Dynamic
button factories and subscription ownership corrections are documented in
the UI topic. No private CEGUI container or font algorithm was added.

## Validation in progress

SDK callback library build3 passes. Five shared dialog/selection/layout/
close/button tests pass (q06-select-shared-test3.log), including Esc0/1/-1,
callback/no-callback ordering, shared Down and Left handling, subscription
owner/untouched window sentinel, and footer count. Read-only selection
observer rejects invalid ranges, bad indices and a frame changed during its
sample;4 Python tests pass. Snapshot schema reports select_dialog_address
without constructing a window in either game.

The real resource fixture compiles all four callbacks through the SDK,
checks actual ModalFrame config retirement, frozen world/actors, Esc1,
Enter0, Down twice2, repeat UI reuse, same-context result and cancellation.
resource1 found wrong widget factory types;resource2 passed choices but its
fixture retired the window before ModalFrame polled closure;resource3 passed
all selection checks but hit the old shutdown total12 modal frames. The
fixture now returns through actual Esc before retirement and checks exactly
three Select frames plus the prior four Quest/eight Inn frames. Final
application build5 and full original-resource resource4 passed: exit0, no
SKIP, printed all real selection probes, GUI13255/13255, dynamically selected
nonprimary1600x900 and451 mute observations. Pair61 cold60901 (1227538630) then actual func2102 passed bilateral question,
ordered Yes/No labels/userdata, hovered0, result-1, Esc default1 and actual
attached roots in ModalFrame. Same Enter returned0 and detached both roots;
all observed selection fields matched. Its actual Q06 func2002 captions
42480..42489 matched text/order/readiness/page state and common input.
Entering M10 then failed on calCtrl Machine task19, followed by incomplete
scene cleanup; keep stamp20261031-102738 and the dialogue/input journals.
The driver stopped on process exit, original was normally closed through
verified identity without unilateral dialogue input. This is SelectDialog
acceptance, not M10 entry/global61001 acceptance. Restart from the reliable
60901 archive after task19 integration.

Full mainline, visual correctness, all arbitrary callback/option variants,
complete frame/realtime/CRT equivalence remain unverified.
