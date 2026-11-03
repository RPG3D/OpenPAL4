# Monster graphics adapter

`MonsterSceneGraphics` borrows `SceneMonsterInstances` and `DistanceOptimization`.
Both must outlive it. It owns per-instance GPU meshes and atomic-distance/material
state, uses the instance's existing `ActorPlayback`, and never advances clocks.
The original scene render dispatcher must request each draw; this adapter does
not infer visibility, attach actors to a world, choose actions, or replace AI.

Lifecycle:

1. Construct CPU state after original models exist.
2. `Upload()` with the current GLES3 context; failed uploads release partial GPU
   resources and leave `Uploaded()==false` for retry.
3. `Draw(handle, projection, camera)` samples the current playback and transform,
   then uses existing `AtomicDistanceInstance` and `GlesRenderer`. Generation is
   validated before indexing GPU storage.
4. Before normal teardown, `Release()` with the owning context current. Before
   destroying instances/distance state, destroy this adapter.
5. After context loss, `Abandon()` before any destructor can delete stale names.
   `Upload()` rebuilds graphics while retaining CPU playback/material state.

ChapterHost owns the adapter and uploads after the existing gameplay gate commits
a scene. Stop/replacement/failure retire graphics before the monster runtime.
Android surface recreation and Session destruction abandon old graphics names.
There is deliberately no unconditional monster loop in either platform renderer:
the production world/render membership dispatcher is still unbound.

Validation executables:

- `pal4_portable_monster_scene_runtime_composition_tests
  --original-gamedata <dir>`: M01 all-or-nothing group publication, four partial
  construction failure types, transform/grid cleanup, retry and close ordering.
  External callback side effects and consumed identifier values are not rolled back.
- `pal4_chapter_host_monster_scene_integration_tests`: empty-scene graphics owner
  lifecycle without GPU allocations; original-data mode retains the populated gate.
- `pal4_portable_monster_scene_graphics_tests <dir> <capture-dir>`: Android EGL
  pbuffer, 800x600, original M01/1 and M02/1, 20 instances/5 model occurrences,
  explicit Z01 at 0.125 seconds, local-origin fixture. Checks visible pixels,
  emitted triangles, unchanged clocks, current transforms, stale generation,
  release/reupload and actual context recreation with exact same-device pixels.

The model fixture disables distance optimization explicitly to observe all models.
It is not a real scene frame, PC visual oracle, active-world membership proof,
combat test, or authorization to remove the populated-scene gate. User-facing
screenshots and limits: `../android/怪物绘制移植验证_20261030.md`.
