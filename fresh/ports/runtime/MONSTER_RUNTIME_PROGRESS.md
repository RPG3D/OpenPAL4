# 怪物场景运行时接入进度

## 已有真实对象与生命周期

- `MonsterSceneResources` 持有原始 `ArenaSource`、同代 `ScenePathways` 和 `SceneMonsterInstances`。怪物模型按共享资源 ID 去重，延迟动画和对象成员仍使用同一个原包源。
- `MonsterMovementWorld` 持有路径快照、原始 grid/member/helper 图像及空间关联；构造时检查路径代次，关闭时清除成员与 helper，再释放 grid。
- `MonsterMovement` 借用 `MonsterMovementWorld`。`MonsterSceneActors` 统一共享网格更新和怪物帧推进，并在关闭时逆序关闭 actor。
- `MonsterSceneActor` 还需要真实的 floor、碰撞查询、`MonsterOrientation`、原 actor package 和 `IMonsterPursuitSceneServices`。这些章节级服务没有可安全默认成功的替身。
- `MonsterInteraction` 借用现有玩家视图及脚本服务；`MonsterCombatCollector` 借用完整 actor group、玩家视图和墙碰撞网格；`MonsterCombatHandoff` 借用状态/参数文档服务。它们都不应早于这些所有者销毁。
- `CombatWorldResources` 独立加载原始 `PALWorld/CombatWorld/<area>.cpk`，通过 lease 阻止资源仍在使用时替换/reset。当前它不自动绑定怪物对象或战斗 host。
- `CombatStateLifecycleBridge` 只桥接已注册的 PALIV_COMBAT owner 和平台 services；要求 Leave 后先解除 dispatcher，再 Retire。它不能单独创建 combat state owner。

## 本次实现

新增 `MonsterSceneRuntime`，把场景资源、真实路径/空间世界和共享怪物移动服务组合为一个 owner。构造时使用同一个 generation 和 `ScenePathways`；`Close()` 先拒绝仍存在的 helper，再关闭世界；只有关闭成功后才销毁移动服务和场景模型/路径资源。被 helper 拒绝或 world 关闭失败时所有 owner 保持可用，便于先清理借用者后重试。析构沿成员声明的反向顺序执行既有 RAII 清理，不增加进程终止策略；调用方必须先关闭所有借用 world 的 actor/manager，再销毁 runtime。

它不伪造 actor 的章节回调，因此 ChapterHost 仍需提供真实碰撞、floor、orientation、玩家/脚本、actor package、接敌和状态切换服务后，才可构造非空 `MonsterSceneActors`。

focused 合同覆盖 owner 的 generation、服务借用关系、遗留 helper 时拒绝关闭且保持对象可用、销毁 helper 后成功关闭，以及访问关闭后的拒绝和重复关闭。非空原版场景可用该 CLI 契约加载并创建真正的资源/空间世界：

```powershell
pal4_portable_monster_scene_runtime_tests.exe <game-data> <world> <scene>
```

当前不含 ChapterHost、GameServices、SceneObjects 或 CEGUI 的改动。战斗 BSP/lease 与注册的 PALIV_COMBAT 生命周期仍由 ChapterHost 的状态/服务层接线。

## 尚未闭合

此 owner 不管理由外部创建的 `MonsterSceneActor`、`MonsterSceneActors` 或 collector/interaction/handoff。调用者必须先销毁这些借用 owner，再调用 `MonsterSceneRuntime::Close()`；下一步接入仍需真实实现 `IMonsterPursuitSceneServices`、floor/碰撞、orientation、玩家视图及 combat state 服务，并确认 ChapterHost 的声明顺序覆盖整条依赖链。
