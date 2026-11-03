# Portable UV animation

`UvAnimationDictionary` is a CPU-only adapter to MIT librw, pinned at
`18532c2e13efbc1aa43f0b00b4459831f9414de0`. No RenderWare SDK binary is linked
into portable builds. Original SDK public APIs are used only in the separate
Win32 diagnostic oracle under `../tools/uv_animation_oracle`.

## Ownership and integration

- Input is an in-memory span from the original CPK reader. The dictionary owns
  decoded data after construction; input storage may be released. No extraction
  cache or Python preprocessing is required.
- Material plugin 0x135 is read as a nested STRUCT, 8-bit slot mask and 32-byte
  names for occupied slots. BSP/DFF draw metadata preserves these names. DFF
  groups with different bindings cannot be merged merely because textures match.
- `ReadUiWorld(..., true)` opts into loading its BSP and trans dictionaries.
  Default callers retain static loading without requesting UVA. Production
  `ChapterMainMenuUi` enables it. Water clumps are not attached as menu draws.
- `Evaluate(scene, absolute_seconds)` preserves draw colors/visibility and
  returns two UV channel matrices per draw. Library interpolation, matrix apply
  and matrix composition perform the math. Missing named animations fail rather
  than creating dummy data. GPU shader uniforms consume the affine matrices;
  immutable vertex buffers are not regenerated each frame.
- The NULL library backend uses no platform window, graphics API or device.
  Its global state is guarded by one mutex; dictionary lifetime is independent
  of EGL. Each menu retains its own time and dictionaries. Existing default
  scene/actor draw appearances have identity UV transforms.
- Shader UV uniforms initialize once per scene draw and change only when a
  material's transform changes, avoiding two extra uniform updates per static
  material. Matrix state resets on subsequent default draws.

## Bounds and library differences

The adapter validates chunk lengths, counts, names, supported schemes/flags,
finite time/value data, channels, initial pairing and unbranched predecessor
chains before invoking the legacy library reader. Initial links must not
prematurely terminate node discovery; later links must be valid array entries.
Channel-map validation covers only the derived active node count, not all eight
reserved slots. Original BJ_trans has one node, map[0]=0 and seven unused slots
with non-channel values; rejecting those prevented the System background from
loading. The public library applies only numNodes entries. Synthetic tests retain
active-channel rejection and verify that an invalid unused tail changes neither
channel. Original SDK and portable BJ matrices match exactly at seven times;
evidence: `H:/PAL4/objs/android-role-ui-20261103-06/BJ_trans-parity.json`.
The vendored reader maps invalid unused initial links to null instead of forming
an out-of-bounds C++ pointer. This is a documented patch, not evidence that all
PAL4 animation streams have been accepted. Unsupported topology fails explicitly.

Upstream `addTime` resets to zero on duration overflow. The adapter reduces
absolute time first, preserving the remainder and exact duration endpoint
observed in the SDK oracle. See `fresh/third_party/librw/PAL4_PATCHES.md` for all
vendor modifications and image-codec exclusion.

The legacy P4SC diagnostic transport still has 32-byte serialized draws. It is
decoded into the expanded in-memory Draw explicitly; strings must never be read
with a raw `sizeof(Draw)` transfer. Memory budgets count the in-memory size.

## Evidence, 2026-11-02

- CPU synthetic tests: cyclic time, independent channels, preserved colors,
  absolute-time reset, missing names, every truncation boundary, invalid links
  and unsupported channel rejection.
- SDK comparison: original trans/water dictionaries at 7 times each have zero
  matrix delta. Independent linear and rotated/scaled/skewed parameter fixtures
  at 7 times each have maximum delta 0 and 2.4e-7 respectively. Results:
  `H:/PAL4/objs/menu-uva-portable-parity/`.
- Original `ui.cpk` integration checks the trans material name and U=-0.25 at
  time25, preserves all world counts, and retains the no-UVA default-path check:
  `H:/PAL4/objs/menu-uv-cpk-contract.log`.
- Offscreen GLES tests verify diffuse and light-channel transforms, row/column
  interpretation, default identity restoration, existing winding and state
  restoration. `H:/PAL4/objs/menu-uv-render-final-test.log`.
- P4SC corruption/camera tests: `H:/PAL4/objs/menu-uv-scene-contract.log`.
- Android emulator800x600: actual material U changes -0.050911844 over5.0911856
  seconds; expected -0.050911856. About-page and HOME lifecycle records and both
  screenshot comparison/difference pairs:
  `H:/PAL4/objs/android-original-pair-20261102/menu-uv-fixed/`.

This establishes main-menu animation integration, not full gameplay UV support,
same-frame particle parity, complete menu behavior or Switch device acceptance.
