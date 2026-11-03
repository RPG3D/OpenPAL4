# Direct GOB position callback

2026-10-31 M10 reaches callback `-176`, `giGOBSetPosition(name,x,y,z)`.
This is distinct from `-308 giGOBMovment`, which creates an existing timed
movement handler. Original truth is recorded in
`H:/PAL4/objs/m10-gob-set-position90-investigation.md` and its live IDA artifact.
Ownership-first review of `5D74C0` and the Obstacle setter `666550` confirms
lookup followed by one absolute position setter, with no activity test or
wait/timed-movement cancellation. Hidden objects with a clump still transform.

The native callback delegates to shared `ScriptGobPosition::Set`. A call-local
`GobPositionArguments` adapter projects payload0/text1 tokens onto the shared
legacy string-reading boundary, using the existing `services.object_move`
manager, name lookup and position setter. Empty text uses the shared empty-name
path and reaches existing lookup; missing objects return before setting. No
legacy VC6 string image, new service pointer, active query, wait registration
or cancellation is introduced.

The setter delegates once to existing `IScriptObjectTimedMoveRuntime::SetPosition`
with flags0. That native facet and the script signature are void, so the
adapter returns normalized0 internally and never exposes an invented original
register result. The real object transform continues to use
`NativeGobObjectImage::SetPosition` and shared SRT/secondary-frame/bounds update.
An already-running timed move retains its captured trajectory and can overwrite
the direct position on its next normal update, as the original does.

`VerifyGobDirectPosition` in `angelscript_game_callbacks_tests.cpp` invokes the
actual compiled SDK callback, checks XYZ/flags0 and immediate script completion,
missing and empty-name lookup, and explicit missing-service failure. Every
activity query or timed-move/wait operation in its fixture fails immediately,
so the direct callback cannot accidentally take the timed movement path.

Validation: native Release build1 and owner-observation rebuild1 passed;
actual SDK callback test1/1 passed. Full native resource1 exited0/noSKIP,
GUI13260/13260, mute462, dynamic nonprimary1600x900 display. The actual-resource
probe also compiles/calls giGOBSetPosition on hidden MO001, compares its owned
position and actual RW LTM, verifies absent-name no-op, and proves an existing
asynchronous move remains registered and subsequently overwrites the direct
placement using its captured trajectory.

Pair92 cold-loaded original61003, replayed24 wealth captions and tip EVF with
real movement, then passed all four CommonDialogs and the tutorial endpoint
229094171. During the tutorial's JA03 fall,24 paired snapshots include12
heights from232.2561 to8.2562, with owned GOB position/LTM/visibility equal.
This accepts the formerly missing direct setter on that route and the sampled
foreground fall. It does not accept every asynchronous machine trajectory.

Next distinct fork: at same outer10780 the first periodic JA01 fall differs,
originalY230.498001/native219.125, approximately15 movement ticks apart. The
leader's seven float32 groups, party/globals/money/fullMT/inventory still match
and stock3126 remains0. Stop-and-preserve evidence is under H:/PAL4/objs/
m10-stones92-first-024{,-gobs}.json. Root cause remains under investigation;
no13-stone collection or61100 completion is claimed.

The earliest failure was initially called movement-176 in commentary. The
actual signature catalog confirms -176 is direct position, whereas -308 is
existing timed movement; implementation and acceptance follow that corrected
mapping. Cached original CommonDialog may be detached but locally visible;
test inputs require current GUI attachment/modal ownership and expected text,
not stale visibility/caption. Latest normal restart checkpoint remains61003.
