2026-10-22 current configuration correction: see ../renderware/combat_live_configuration_native.md. Script configuration now delegates to the actual native tail; original M01 completion is a Monster round-limit result2. Earlier detached/default-config defeat evidence does not establish the intended M01 ending. The configuration-only/unbound frontiers below are historical.

Current continuation: SDK-96 now builds the original native CombatParam request;
actual Combat state owner remains unbound. See
[native combat handoff](../package/native_script_combat_start.md).
The configuration-only stopping point below is historical.

# Native combat configuration

`NativeScriptCombatConfig` owns the pending five words, three text values and
three flags as a modern `CombatConfigSnapshot`. The application keeps it alive
across script contexts. It currently owns configuration only; PALIV_COMBAT entry,
battle actors, UI, update/render, outcome and return still require native binding.

Live IDA confirms SDK-94/-95/-218 reach assignments to combat+452/+468/+484
via5D4480/5D4490/5D8650 and576810/576950/576A90. These assignments contain
VC6 string ownership code, so the native path uses public std::string assignment.
The source's argument string is borrowed only during the callback; stored values
and copied snapshots have independent lifetimes. Marker bytes are preserved.
579770 initializes five words0/0/0/0/1, empty texts and flags1/0/0; its marker
seed is indeterminate in the original and explicitly zeroed in the native owner.

SDK-93 uses existing ScriptCombatConfig::Configure (5D42C0): copy all words/texts,
overwrite words0/1, replace type only for0..2, and set flags enabled/1/1.
ReadWord/ReadTextProperty use bounded logical tokens, never native dereferences
or VC6 record allocation. Cleanup releases copied host strings.
Legacy ScriptCombatTextThunks moved to script_combat_text_thunks.cpp, preserving
existing non-native callers without linking private COW operations into this path.

Build1 succeeds. Native compiled-SDK test verifies M01's P39/CA7/CA12 and
true/4/2579/2, repeats, replacements, empty text, independent copied snapshots,
false flags, signed parameter bits, valid types and invalid-type preservation,
and malformed logical addresses. The existing callback test also passes.

Original memory2 passes configuration and fails explicitly at SDK-96,
giStartCombat(M01), with NPC/world teardown. Original CSB is unchanged; no battle
result or story completion is synthesized. Full M01/1,11100,unlocked,finished
acceptance remains pending. Memory is a diagnostic expected to stop, not a passing
CTest. memory1 failed earlier on temporary settings commit; subsequent commands
explicitly set TEMP/TMP under H:/PAL4/objs/tmp.

Next entry evidence: shared ScriptCombatStarter builds CombatParam XML from the
same pending monster queue and enters state3. Shared CombatSceneCoordinator
58ADB0/58B270 then starts CombatSystemStart5756D0, including cold combat database,
console, world and actors. These must be real persistent owners with failure
rollback and scene return; a configuration snapshot does not provide this lifecycle.

Logs and live IDA: H:/PAL4/objs/M07-combat-config-20261017-*.

Final affected regression28/28 passes in148.05s. Rebuilt legacy shared
pal4_script_combat_config_tests also passes after the source split. Opening/load
intentional failures return expected exit1 with NPC/world cleanup. No residual
PAL4/launch process remains. No play session was opened.
