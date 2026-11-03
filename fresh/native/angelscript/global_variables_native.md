# Native SDK global bindings and corrected original index order

Verified boundary, 2026-10-20. NativeAngelScriptEngine formerly registered only
int global_mvar; other original negative global accesses were not backed by a
complete SDK property table. Engines now register all48 through the public SDK
RegisterGlobalProperty API, borrowing the application's existing GlobalAddress
storage or owning independent zero-initialized storage for standalone engines.
Existing ScriptGlobalVariableRegistry signature metadata is reused through
NamedSignature; no private registry/vector/CRT algorithm was reconstructed.

## Authoritative order and parser correction

Live IDA CRT table89D330..89D3EC contains mvar, maze1..10, then37 named-variable
initializers. 7E0480 appends case-sensitive deduplicated descriptors, and7E0670
iterates that vector in order into engine virtual+24 RegisterGlobalProperty.
The SDK's negative global operand uses globalPropAddresses[-index-1]. Therefore:

| CSB operand | Original property | Original storage |
| --- | --- | --- |
| -1 | global_mvar | 90DEC0 |
| -2..-11 | maze_var1..10 | 90DEC4..90DEE8 |
| -12..-48 |37 named variables, Shu through Shop030 |90DEEC..90DF7C |
| -31 | var_shop010 |90DF38 |
| -41 | var_shop035 |90DF60 |

The reference parser's global_variable_names.json instead put37 named entries
before the ten maze entries. Its near-source label var_shop035 for original
Q02 task STORE_GLOBAL -31 is wrong. The correct target is var_shop010. The raw
bytecode/function table was valid; the semantic-name table was shifted. Older
research notes and generated near-source must not be used as runtime truth.
The reference project was left unchanged; corrected order/old indices and live
CRT slots are saved in objs/M09-quest-globals-20261020-*.json. This workspace's
runtime uses verified order and existing registry metadata, not that parser map.

## Ownership and verification

NativeNewGameScriptSession accepts the resolver and passes it to its engine.
The application binds all original90DEC0..90DF7C addresses through the same native
PALGameIV owner already used by story/game-data consumers. Supplied story storage
must alias the resolver's mvar pointer; missing storage and mismatched owners
reject construction and release the SDK engine. Returned pointers must outlive
the engine/contexts. Standalone engines own independent storage; all their modules
share it without module copies.

The actual original Q02 task's -31 store now changes native90DF38 only on Yes.
NativeScriptGlobalBindings tests additionally compile real SDK modules reading
and writing distinct global, maze1, maze10, Shu, Shop010, Shop035 and Shop030 host
slots, verify cross-module sharing and another engine's isolation, and reject
missing/foreign storage. All focused SDK/callback/frame/trade tests pass after
relink. Complete application including original task/cave progress passes15.46s.

These checks establish registration/order/runtime sharing. The complete original
application now also poisons all48 actual host slots before the second menu
NewGame. After Prepare/Start, with the SDK runner still queued before its first
Tick, all48 slots and the persistent engine's story observation are zero. The
existing original opening, scene loading, movement and cave regression then
passes (15.41s). This reuses the existing GameSystemData reset, including the ten
maze slots outside its pointer collection; no duplicate reset was introduced.
Evidence: H:/PAL4/objs/M09-global-reset-20261020-{build,test}.log and
M09-global-reset-20261020-final-application-detail.log.
The actual System-menu archive application probe additionally roundtrips all48
slots through F7/save/page9/slot99/confirmation and full menu Loading. It assigns
47 distinct alternating signed maze/named values, preserves a valid story value,
writes the complete isolated archive/PNG, overwrites all48 live slots, then checks
each restored value and the SDK story alias after actual world/player/HUD reload.
The following original slot0 load and cold input checks pass (10.28s). Installed
99.dat/png are byte-preserved. Evidence: M11-global-archive-20261020-* under objs.
This validates that scene and successful archive path; arbitrary-world, settings,
malformed-archive and full-game persistence equivalence remain unproven. Evidence H:/PAL4/objs/M09-quest-globals-20261020-*
and M09-quest-state-20261020-*; tests remain hidden/muted/nonactivating with objs/tmp.
