# Native script trade wait

Verified increment, 2026-10-20.

`NativeScriptTradeRuntime` adapts the existing `ScriptTradeWait` to one borrowed
AngelScript 2.2 SDK context. `NativeGameScriptCallbacks` routes system ID -299,
`void giStartTradeSystem( string &in, string &in )`, through that shared algorithm.
No private library/script engine implementation was added.

## Original evidence and ownership

Ownership gates were checked for 5E6E40, 5E6EF0 and 5DDEC0 before live IDA review.
The descriptor at 5DDEC0 registers 5E6E40. That function forwards shop/task,
normalizes null script strings, initializes a 32-byte mode1 wait with vtable
849600 and queues it at executor+54. 5E6EF0 polls PALGameIV's current state entry
(object+18C; the decompiler's data[392]) and its virtual+14 state ID. Null state
or Scene1 completes; Trade4 page/selector and Foundry5 do not.
The old shared interface names TradeUi/TradeUiCompletion remain compatible,
with their actual current-state meaning documented. They are not page visibility.

## Native lifetime and application binding

The adapter owns inline wait storage and an active flag. It validates the active
SDK context before changing global UI; queue and wait identities are checked.
AddHandler suspends that context. Tick reuses the original shared completion
algorithm. ClearHandlers clears only its own wait; it does not close a global
shop and rejects retirement while its context is executing.

The application factory supplies the existing NativeTradeUiSession start path,
the actual PALGameIV current-state slot and NativeStateList::StateId. Primary,
event, concurrent scene and timed context service sets receive their own adapter.
NativeScriptOpeningWait aggregates blocking, polling and unbinding; reloaded
scene services rebind the primary adapter. Context cleanup clears owned waits
before context retirement. Existing frame runners remain responsible for script
continuation and service selection.

## Verification and limits

Build3 compiles the complete application. Test3 passes six tests in16.71s,
including the original-resource application in16.06s. The application probe uses
the real idle primary SDK context and a module-local marker: giStartTradeSystem
opens actual Trade4, the real property selector action opens a shop, closing the
page keeps marker7/Trade4 waiting, and exiting the selector to Scene allows the
same context to finish at marker11. Three added Trade frames bring the global
regression total to18. This explicit SDK probe polls the actual wait before
calling Execute; it does not claim original NPC/event scheduling acceptance.

The independent native test exercises real compiled SDK code and frame runners:
exact strings, independent contexts, foreign-context rejection before UI mutation,
Scene-only continuation, null current state, cancellation, failed start and retry.
The shared boundary test retains original allocation/layout/call-order coverage.
Build4 relinks affected callback/frame/flash tests; test4 passes all five focused
script checks. Evidence: H:/PAL4/objs/M09-script-trade-20261020-*.

Tests remain hidden, muted and nonactivating on the dynamically selected nonprimary
1600x900 monitor. No play version was launched. Original NPC/shop-task acceptance
remains pending. The subsequent application Foundry5/Smith route now passes two
cycles with this same primary SDK wait; see ../renderware/foundry_application_native.md.
Full game equivalence is unverified.
