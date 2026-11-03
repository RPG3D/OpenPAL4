# PAL4 AngelScript runtime

`pal4_angelscript_220` builds the official AngelScript 2.2.0 source as a
Win32 static library. `pal4_angelscript_game` wraps its public engine and
`asIBinaryStream` APIs, registers the SDK's `scriptstring` add-on, then
registers the nine IDA-confirmed math functions using host math functions.
The 264 PAL4 function declarations are generated from
`game_api_signatures.json` (copied from the script parser project's
`native_api_signatures.json`), with the missing `-202` overload checked in
IDA. Run `python fresh/tools/generate_angelscript_game_catalog.py --check`
to verify the checked-in table. Their callbacks cross an injected
`IGameScriptCallbackRuntime` boundary; a missing
runtime raises a script exception naming the original system ID.
The engine and add-on are built by the same compiler, so their callback
records use one consistent host ABI. No object from this build is passed to
the original `launch.exe` engine.

The package integration test reads `script.csb` directly from the original
`I:/Games/original/gamedata/script.cpk`, strips its four-byte size header,
loads all 636 bytecode bytes, resolves `void NewGame()` and prepares a context.
With a recording callback runtime, the original function executes to
completion and requests both movies, sets `global_mvar` to 10100 and asks for
arena `Q01/N01` in the recovered order.
The independent engine test compiles, saves, reloads and executes a small
script, then checks math and string callback invocation.

IDA `0x7DE7D0` registers string, math, then game descriptors before loading
modules. `0x7E0670` registers game global functions in descriptor order,
then object types and global variables. The current adapter registers only
the `global_mvar` property needed by `NewGame`; other game variables and
object descriptors remain pending. Original `NewGame` bytecode calls
system IDs `-291` (flash), `-237/-238` (music), `-297` (movie), `-110`
(movie flag), `-57` (new game), `-65` (arena load), and `-19` (string factory).
`NativeAngelScriptEngine` and `NativeNewGameScriptSession` can borrow the live
story word at construction; the storage must outlive the engine. The application
supplies `NativePalGameIVTextureStorage::GlobalAddress(0x90dec0)` so GameData input
and script execution share one word. Standalone probes keep owned storage.
Classified/live IDA45D150 confirms the original global_mvar descriptor stores
90DEC0 at+30. No copy or periodic synchronization is involved. The engine test
executes script writes against a preloaded external word, then changes that word
and executes the same script again to verify immediate two-way visibility.
The WinMain integration host binds original Miles music and the native
new-game composition service, then enters native Q01/N01 Loading through
`giArenaLoad`.
The live WinMain host now registers/owns NativeOpeningVideoState and dispatches
opening movies through state12. Real Scene lifecycle round trips also pass with
a stopped decoder. The separate Logo fixture covers855-frame natural end.
Persistent Logo and routed per-context movies are now integrated. Independent
real-video/state/GPU tests pass; the latest final-movie ordering follows the
original handler phase. Full application checks remain pending; see
../package/final_movie_completion_native.md.

`NativeScriptMoviePlayer` binds `giPlayMovie` and `giPlayMovieFinal` to the
original Bink DLL and package assets for one AngelScript context. It suspends
the context during playback and exposes `Tick`/`Stop` completion to the frame
owner. The original `NewGame` test decodes all 855 frames of `1A.bik`, resumes
into `Pal4A.bik`, then reaches the next missing global-data service `-110`.
A separate RenderWare test follows `giPlayMovie` through a decoded frame to
GPU readback. The WinMain integration host now binds the movie callback,
enters registered state12 and presents frames through its state-owned renderer.
`NativeScriptOpeningWait` also maps the original Escape scan code to movie
close and resumes the suspended context. The window test now passes both
movies, saves `giOpenMovieFlag` through an isolated copy of the original
`pal4.set`, loads four original player actors through `giNewGame` (`-57`),
and finishes `giArenaLoad` through the native arena, PALGameIV and Loading
state owners. The separate original-script test also finishes with a recording
arena service. The original-resource application test now traverses N01, Q01
and back to N01, including Scene entry and a completed `func1001` event
context. This is a bounded scripted test; continuous play and free input
remain unverified.
Other game callbacks remain explicit script
exceptions until their IDA-backed services are connected. The host math
functions can differ from the original x87 wrappers at rounding boundaries.

Effect script IDs -138 through -141 now dispatch through the existing
`ScriptEffectCallbacks` with a borrowed `NativeGameScriptServices::effects`.
Live IDA5D5AB0/5D5B10/5D5B70/5D5BD0 confirms coordinate, indexed-player,
current-player and named-NPC creation, position+0xAC, fixed flag0 and temporary
handle reset. The generic adapter marshals host script strings into the shared
handler's two-word payload for the synchronous call; runtimes must consume/copy
the text during that call. Actual compiled AngelScript tests cover signed
repeat/index arguments, coordinates, actor selection, temporary handle reset,
missing actors and the missing-service exception. These calls create no wait.
The application still needs to bind its real effect group and creation/frame
services; passing dispatch tests does not establish visible gameplay effects.
`NativeScriptEffectRuntime` now supplies the native creation boundary; the
original-resource actor probe executes a compiled script into actual foot_2
particle loading, GPU drawing and natural completion. See
`../renderware/script_effect_runtime_native.md` for ownership and remaining
application integration requirements.
The injected callback runtime must outlive any script execution using it.

`NativeScriptFrameRunner` now owns one AngelScript context and queues a module
function for execution on the next frame. Suspended scripts resume only after
the supplied frame-wait service reports completion; the existing
`NativeScriptFlashRuntime` implements that service. The runner reports script
exceptions and does not retry failed frames. This matches the frame-driven
`ScriptExecutionContext_ProcessWaitHandlers` call reached from Logo state
update in IDA (`0x58FC00` to `0x7DF4B0`). The WinMain integration host now
starts NewGame from its main-menu click and ticks flash and movie waits; the
original NewGame script now finishes and enters Q01/N01 Loading. The
original-resource application test also executes Scene scripts and resumes
event bytecode after the return to N01.

`pal4_angelscript_game_callbacks` now connects the `NewGame` callback IDs
(-291, -237, -238, -297, -110, -57, -65) to the existing reconstructed
game handlers. It also connects -318 to the existing final-movie handler.
The original script executor calls AngelScript 2.2.0's deprecated
`ExecuteStep(0)` at context vtable `+64`. The native runner now exposes
`TickOriginal`, which advances one source step and repeats steps while
`giIMMBegin` mode 1 is active. It distinguishes line-step suspension from
blocking movie, flash, action, camera, turn, timed and dialog waits.
The vendor SDK's `BC_CALLSYS` path was fixed to preserve a callback exception
before handling the step-suspend flag. Original `NewGame` bytecode, a compiled
arena-to-object script and a blocking gate test cover this behavior. The live
DISPLAY3 fixture uses `TickOriginal` at the second Q01/Q01 request and stops
before object callbacks. Its main frame now uses `TickUntilBoundary` to
advance source steps until a blocking wait or a new Loading request. The
boundary also stops immediate-mode execution before the following callback.
The frame pauses while the Loading target differs from the installed arena
scene. Advancing Q01/Q01 Loading between script steps remains pending.
The original Q01/N01 scene-init probe connects `giScriptMusicPlay` (-127) to
`BackgroundMusicPlayer::PlayScriptTrack` and `giCameraSetCollide` (-103) to
`ScriptCameraCallbacks::SetCollision`. It verifies the P23 request (mode 2,
zero fade-in, 2.5-second restore fade) and writes to both camera collision
objects. The same probe reuses the player leader, camera auto-seek, player
lock, position, rotation, arena-ready and system-UI handlers. Original Q01
bytecode sets current-player XYZ to `(7, 7, -22)` and yaw to `180`, marks the
arena ready, writes magic-system entries 20/19 and system-UI entry 12, queues
the `J08` player action and requests equipment IDs 3586 and 3501. It then
prepares `MC001`, runs camera record `001`, plays `WJ001`, starts a two-second
black fade-in, then suspends at `giWait(1.5)` through the reused
`ScriptWaitHandler`. The probe checks the 1.5-second countdown and closure.
A compiled script also verifies both camera callbacks'
integer return values through the generic ABI. The original equipment
categories are not inferred from the probe's recording runtime. These are
service boundaries in the isolated script probe. The live scene-loading host
now loads and starts `Q01.csb` after 95% through the same engine that completed
`NewGame`, preserving `global_mvar=10100`. It plays P23 through the original
Miles DLL, updates both camera collision fields, retains leader index zero,
and executes `giPlayerLock` against the owned player, ActorControl, input and
Loading state. The current-player position callback (`-250`) writes `(7, 7,
-22)` to both the player and RenderWare Clump; its C01 action and actor name
reuse the native Role and player owners. The live Q01 path now also uses the
existing movement selector, floor-color and empty minimap-grid implementations.
Position and rotation enqueue closed 32-byte handlers without suspending the
VM; rotation reuses `GameResourceManager::UpdateClumpRotation` and reaches
`giArenaReady` (`-66`). The live arena-ready service now executes the Q01
empty-Pet branch, refreshes input and minimap state, and sets the arena ready
flag. `giGrantMagicSystem` and `giGrantSystemUi` write GameData entries
20/19/12 through `GameSystemData`. The live `J08` action now uses the original
Role HAnim clip and `101/J08.amf` message to suspend until `HOLD` fires; the
 next callback equips the original sword and armor. `giCameraPrepare` then
 loads `MC001.cam` from the Q01/N01 scene package and finds record `001`;
 `giCameraRunSingle("001", true)` now applies the original immediate camera
 record and resumes through its task-container wait. The next unfinished
 behavior is playback of interpolated camera record `002` after original Miles `WJ001.wav`
 playback and `giWait(1.5)`. The timed wait has a native owner that resumes
 the same script context when its countdown closes; the focused VM test
 verifies the 1.0 + 0.5 second transition. Other fade services also need
 live bindings. An absent service
produces a script exception with its system ID; the isolated recording
callbacks do not supply gameplay behavior.
IDA `0x5d30e0` confirms that `giArenaLoad` ignores its fourth bool argument.
The callback integration test compiles AngelScript against the registered
functions and checks the existing flash, music, movie-flag and new-game
handlers through the generic call ABI.
The original `script.csb` test also installs this adapter without native
services and confirms that `NewGame` stops at `-291` with an explicit
screen-effect service error.
`NativeScriptFlashRuntime` now owns the flash wait records for one AngelScript
context. It reuses `PalScreenEffect::StartFade`,
`EffectRenderControl::AdvanceFade` and `ScriptFlashEffect`'s completion check;
blocking calls suspend the public AngelScript context until a frame tick
closes the handler. Zero-duration fades complete on the next tick without
passing a zero denominator to the original interpolation routine. The original
`NewGame` bytecode now suspends for its first black fade and resumes through
music pause into the native movie service.
The original-bytecode native new-game integration test uses immediate movie
callbacks and an inactive-fade fixture to exercise the callback after
movie-flag persistence. It
loads all four original Role records, resets player slots and restarts actor
control, then executes `giArenaLoad` through a recording arena service. It
checks the `Q01` / `N01` scene request and finishes the original script.

Save execution cleanup (2026-10-16): live IDA7DF330 calls execution-stack
cleanup followed by wait-handler cleanup; it does not discard loaded bytecode.
The original shared ScriptExecutionContext code retains that ordering. Native
frame runners use public SDK Abort through ClearExecution, retaining their owned
contexts because wait services borrow stable references to them. A full native
session exposes ClearExecutionStack with a mandatory host cleanup callback. All
four runners become idle before handler cleanup, then callback services detach;
loaded modules, scene entry and external global_mvar storage remain intact.
NativeScriptTimedWaitRuntime::ClearHandlers removes its pending and active
owned timers after execution stops. The application now composes movie/action/dialog/camera/turn/walk/flash cleanup
as described below; the actual LoadSave host binding remains pending. A callback
that only clears timers is insufficient for a live scene containing other waits.
Original-session/frame/timed-wait regressions pass3/3 in0.15s; logs under
H:/PAL4/objs/script-execution-cleanup-{build,tests}.log.

Native handler cleanup now follows each IDA virtual+8 operation: action reset
clears its loop count; turn restores idle; walk stops movement; talk hides the
scenario dialog. Camera and flash cleanup remove waiting records while keeping
separate tasks/effects alive. All native names/pending records use host RAII.
The application ClearNativeScriptExecution invokes the complete bound cleanup
set after stopping script execution. Scene replacement also resets handlers
before destroying its native adapters without cancelling the suspended story/event.
Opening-wait detachment clears its pending movie-stop completion signal.
This composition is currently exercised on shutdown and scene replacement;
LoadSave is still unbound. Camera/flash interruption and shared action-reset
probes accompany native/original application regression coverage. Native active
turn/walk/dialog interruptions remain additional coverage work.
