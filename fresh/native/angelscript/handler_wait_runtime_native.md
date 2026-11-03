# Script handlers and state-owned screen updates

INativeScriptHandlerWaitRuntime narrows INativeScriptFrameWaitRuntime: Tick may
advance handler-owned work but must not advance the shared screen fade. The
native Logo owner accepts this capability and advances PalScreenEffect after
processing script handlers. This is the live IDA58FC00 order.5E5550 only checks
phase+48 and closes the handler; it does not advance the fade.7DF4B0 removes
closed handlers and calls ExecuteCurrentModule.7DE6C0 sets the execution flag
only when wait_mode and script context are nonzero. All four addresses passed
the ownership gate; existing shared/native algorithms and actual libraries are
reused. Misleading IDA log/string names7DF4B0/7DE6C0 were corrected with evidence.

NativeScriptFlashRuntime splits ProcessWaitHandlers from its legacy combined
Tick. Handlers() exposes a borrowed stable adapter; it validates delta and only
checks/closes waits. Retained handler records preserve blocking wait_mode, so a
nonblocking fade does not prevent subsequent SDK ExecuteStep calls. The old
Tick still advances fade before processing handlers for existing owners.

NativeScriptOpeningWait similarly exposes Handlers(), using one TickServices
implementation for both modes. Timed waits, scene action, camera, turn, walk,
tutorial and dialog use the same existing services. The handler capability omits
movie decoding and shared fade advancement: NativeLogoStateRenderer owns decode/
presentation, while legacy combined Tick retains decoding for existing callers.
Natural completion and Esc stop pending state
is retained and consumed by the next script tick. The aggregate cannot be
copied because its adapter borrows its stable owner. Keep all borrowed services
alive; unbind scene services before destroying them. Never tick both variants
for one frame. NativeScriptTimedWaitRuntime implements the narrow capability
and reports its blocking state to TickOriginal, preventing early single stepping.

The actual application fixture uses an isolated real SDK engine/context and
original DLLs/resources with actual RenderWare GPU textures and muted Miles.
Two cycles cover timed wait, blocking flash, nonblocking flash, suppression,
next-frame completion, original1A Bink decode/presentation, Esc stop and VM
continuation. Only one movie frame is decoded per cycle without wall-clock
playback in the earlier handler-only increment. The fixture now uses actual
registered state12 and also runs855 natural-end frames; see
../renderware/opening_video_state_native.md. Final-movie and persistent main-loop
ownership remain open. Scene/camera/dialog wait services
remain available through the existing aggregate but are not newly proved by
this Logo fixture. Active Preterit and registered state dispatch remain open.
Native Logo rendering is covered in ../renderware/logo_state_renderer_native.md.

Existing flash and SDK runner tests plus an added timed-wait single-step gate
check pass. All log/build artifacts are under H:/PAL4/objs/ using prefix
M03-logo-handlers-20261016-. See docs/progress.md for final regression results.
