# 石沉溪洞战斗与离开青鸾峰缺口

> 2026-10-30 分工更新：本文保留历史接线调查。当前任务只负责移植层；游戏逻辑疑点的唯一交接清单为[游戏逻辑缺口](../../../游戏逻辑缺口.md)，不依据本历史文档在移植侧补写游戏规则。

更新时间：2026-10-29。本文依据 Android 移植工作树当前状态、用户原版 `M01.csb` 和 `I:/Games/PAL4_game/launch.exe` 对应的活动 IDA 数据库整理。这里的“离峰”指完成石沉溪洞剧情后离开青鸾峰；本文不把模块合同、原包解析或 Windows 诊断流程记作 Android 实际通关。

## 当前可确认的实际边界

- 2026-10-29 的原 CPK 受控 `func2001` 探针确认入口先调用 service 180 `giHideGASkillObject`。ChapterHost 现将其路由到共用 `ScriptArenaSkillState/ArenaSkillManager`，但 portable owner 目前只能处理空记录 manager；非空 actor、clump 或 effect handle 会在修改前显式失败。它不创建或接管真实技能对象，因此不构成完整 180 实现。后续遗漏的 257 `giPlayerWalkTo` 已分派到 ChapterHost 既有 `PlayerWalks::Start`；受控原包探针验证了真实参数与 wait token，但未验证地面移动。`func9001` 的 154 现读取真实 M01/1 `SceneObjects`；item200 在主机及 emulator-5558 的原资源场景集合中均不存在，按原语义返回 false，随后到尚未接线的 210 `giGOBReset` 分支停止。该 CLI 只解析 SceneResources，不提交 Arena，不改变下述设备剧情边界。详细合同见 [第一章脚本进度](CHAPTER_SCRIPT_PROGRESS.md)。
- Android 最新设备记录从真实新游戏、取弓和室外移动到 `Q01 func1006`，原脚本请求 `giArenaLoad(M01, 1)`。宿主在提交场景前因尚无非空怪物场景运行时而拒绝，错误为 `populated monster scene owner is not implemented`。故实际设备尚未进入石沉溪洞，后续战斗和离峰路线均未在 Android 验收。详见 [安卓移植状态](../android/STATUS.md) 和 [场景怪物进度](SCENE_MONSTERS.md)。
- 原版反编译文件 `I:/PAL4/projects/pal4_script_parser/.test_tmp/decompiled/M01.near.cs` 的 `M01_1_init` 在入洞初始化中设定 M01/1；战斗剧情位于后续脚本函数。第一场战斗由原脚本依序加入 `2579/1`、`2580/0`、`2580/2`，再配置 BGM `P39`、相机 `CA7/CA12`、战斗参数并调用 `giStartCombat("M01")`。学会法术后，第二场按序加入 `2602/1`、`2580/0`、`2580/2`，同样调用该入口。之后脚本继续魁召退场、调查密室、发现望舒并推进出洞剧情；不能用“战斗返回”代替这一整段。
- 原 CSB SHA-256：`FCCC520C2EA44FD278958509980474218983B27C6EE120E3ABC8530AA9314113`。这是用户安装目录中的已解压 M01 文件，摘要用于标识本次核对对象，不代表该包覆盖所有版本。
- IDA `0x4621F0 giStartCombat` 的主体只注册 `Combat_Start` 回调；接敌侧 `0x434720` 读取 `AAB_MST_ESSBEH_combat_pos_%d` 配置并追加每项 12 字节记录，`0x434810` 在距离、垂直差及碰撞门控通过后收集玩家附近怪物，`0x434E30 GameManager_InitializeCombatSystem` 构造 `CombatParam/CombatWorld/CbMonster` 并请求状态 3。它们支持现有交接模块的局部契约，不证明状态 3 可运行。
- 活动 IDA 的 `0x58ADB0`（现有分析称 PALIV_COMBAT Entry）显示 Entry 会保存并修改全局标志、复制请求 XML、配置战斗相机，然后按 Quick/普通/对话分支启动战斗。`0x5756D0` 的现有 IDA 记录还表明 Start 会重置调度和管理器、准备 CombatWorld/角色，之后才安装渲染回调、发布活动状态并派发准备事件。这些步骤依赖完整生命周期服务，不能以状态注册或空回调替代。

## 已有可复用组件

- [战斗移植进度](BATTLE_PORT_PROGRESS.md) 的 `CombatHealthState` 及 `portable::BattleHealthState` 只实现生命状态分类；不含伤害、行动选择、回合推进、AI 或战斗结果。
- [场景怪物进度](SCENE_MONSTERS.md) 已有原记录解析、场景怪物资源/拥有者探针、接敌过滤、战斗位置收集、逻辑地址遭遇记录和 `MonsterCombatHandoff`。原 M01/1、M02/1 数据的受控合同覆盖了怪物成员交接；仍使用测试提供的战斗文档、位置/障碍、状态入口和初始化服务，不可解除 Android 场景门禁。
- 主程序中已存在 `NativeCombatStateOwner`、`NativeCombatStateRuntime`、`NativeScriptCombatStart`、战斗协调器以及大量 CombatSystem 子域。Windows 文档记录的状态/回合/指令/结果组件不等于 Android portable runtime 的组成，也不表示第一章自然流程已通过。

当前没有发现一个新增的小型战斗算法可以单独解除首个真实阻点：可从 M01 原脚本确认的接敌记录、配置与 XML 交接已经分别有模块；首先必须由 ChapterHost 提供并拥有非空怪物场景所需的实例、路径、帧调度和碰撞/玩家服务。战斗入口则必须拥有真正的 PALIV_COMBAT dispatcher 和战斗会话依赖。将任一测试替身或状态注册冒充这些组合会造成假成功，因此本轮不添加伪生产逻辑。

## 新增 portable 状态桥接

新增 `CombatStateLifecycleBridge`，固定 PALIV_COMBAT state ID 3 和一个注册 owner generation，直接复用 `CombatSceneCoordinator`、`CombatSceneFrame`、`CombatSceneUpdate` 原实现，提供 ChapterHost 可持有的四个状态回调入口。构造和每次派发前都要求 readiness verifier 确认同一注册 owner 及其全部运行服务；允许 outcome Update 同步回调 Leave；原算法异常后熔断为 Faulted。合同覆盖了真实共享函数顺序与 XML 销毁前 Leave 次序，不声称具备真实 Combat 服务。

此组件尚未注册到 PALGameIV，也未接 ChapterHost。ChapterHost 还须提供实际 Entry/UI/camera/Combat Start、底层场景帧、Combat renderer、Combat/UI 更新、结果 XML 和 state transition 服务；ID 3 owner/registry 与 generation 的创建、dispatcher bind/unbind 顺序也未接线。完整依赖与生命周期约束见[PALIV_COMBAT 状态桥接](COMBAT_STATE_LIFECYCLE.md)。本模块合同不能解除非空怪物门禁、不能称已进入真实状态 3，更不代表 Start、战斗、结果或 M01 后续剧情已验收。

## 当前关键阻点与依赖顺序

1. **Android 场景入口：** `ValidateArenaGameplaySupport` 遇到 M01/1 的怪物/路径数据仍拒绝提交。须有生产 ChapterHost 组装层承接现有场景怪物运行时，保持对象、碰撞、脚本和帧生命周期一致；仅让资源加载成功不够。
2. **真实接敌：** 受控 `MonsterCombatCollector`/`MonsterCombatHandoff` 尚未接生产玩家视图、真实场景碰撞候选、原顺序和正式脚本/文档服务。接敌触发需要与原 `0x434810` 距离、垂直与墙体判定一致，并将实际 M01 配置位置和三字段怪物记录传入战斗请求。
3. **状态 3 生命周期：** Android runtime 侧须绑定同一已注册状态拥有者的 Entry/Render/Update/Leave，并提供真实 CombatSystem 冷启动、Start、角色与世界资源、游戏事件、输入/渲染和结果返回服务。任何依赖缺失都应在切离 Scene 前显式拒绝。
4. **两场自然战斗和脚本续跑：** 按原 CSB 的两次独立配置实测战斗进入、玩家/敌方行动、胜败分支及奖励/恢复，然后验证第二场返回后剧情动作、对白、物件/特效和离洞路线。当前设备停在第 1 项，无法声称已到第 2 场。
5. **离峰验收：** 从真实新游戏按原脚本与玩家移动自然推进，完成 M01 剧情并回到青鸾峰出口后的目标场景/剧情状态。不能直接改坐标、剧情变量、怪物出现状态或合成战斗结果来跨过前置。

## 本轮实施结论

本轮把 service 180 接入 ChapterHost，并用原包 `func2001` 在空 ArenaSkillManager owner 上验证路由；非空技能 actor/effect/clump 生命周期仍显式失败，不能据此称完整 180 或洞穴运行。没有修改怪物门禁、combat entry 或真实剧情状态，也未部署。当前 Android 实际阻点仍是非空怪物场景生产 owner；要推进自然流程，先补足该场景 owner 和技能对象真实资源/生命周期，再以原 M01 CSB 回归。后续局部合同或受控探针不得标记石沉溪洞战斗或离峰通过。
