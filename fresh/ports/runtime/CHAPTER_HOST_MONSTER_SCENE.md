# ChapterHost 怪物场景接线

`ArenaServices` 为每个有效装载请求分配递增场景代次。`ChapterHost::AdvanceArenaLoad` 在真实 `ArenaResources` 解析完成后，用原始 MST、pathway 记录及共享 `ArenaSource` 构造 `MonsterSceneRuntime`，随后仍调用 `ValidateArenaGameplaySupport`。被拒绝的 staged owner 在返回脚本错误前显式关闭；通过原门禁的场景才转为活动 owner。

独立集成合同默认以无资产空 arena 验证 ChapterHost owner 生命周期，明确不创建怪物或路径。原包模式接受 `--original-gamedata <path>`：确认 M01/1 的 8 条怪物记录、8 条路径、原怪物模型读取和路径 owner，再确认同一生产准备入口仍被门禁拒绝并释放 staged owner，同时保留之前的 active owner。该合同验证资源所有权接线，不代表可进入或运行 M01/1。

`MonsterSceneRuntime` 现在可接管完整的 `MonsterSceneActor` 集合；接入前检查 actor 实例 owner 身份、场景代次和句柄，并要求顺序与原 MST 名称树遍历一致，避免不同 runtime 恰好复用相同代次/索引时误接管。接管后由 `MonsterSceneActors` 持有真实 helper、actor 帧和 `MonsterSceneFrame`，统一先更新网格成员再逐 actor 推进。原版 `MonsterManager_Initialize` (`0x437B20`) 清理后读取 MST、建网格再建实例；`MonsterManager_ProcessObjects_Extended` (`0x437CF0`) 每帧先更新完整网格成员，再遍历 actor。这与现有 `MonsterSceneActors::Advance` 的网格/actor 阶段一致。`CloseActors()` 先关 actor 帧、基础运动及行为成员并释放 helper，调用方再释放这些 actor 借用的原包、碰撞、方向和场景服务，最后调用 `Close()` 销毁 movement、world 和资源 owner。关闭时仍存在未归属 helper 会拒绝并保留 runtime，可清理该 helper 后重试。

ChapterHost 生产链当前仍只 staged 怪物资源/path owner，M01/1 在原有 `ValidateArenaGameplaySupport` 处拒绝并清理；没有构造或推进 M01 actor group。生产 RNG、静态障碍候选 producer、原玩家/碰撞/交互与战斗服务仍未绑定。未绑定的 obstacles 会令 scene frame 显式失败并禁止后续帧；不可用世界几何、NPC/GOB 集合或空列表替代真实候选。下一生产接口需提供 RNG 与障碍 producer 的来源和生命周期，以及原 MST 管理器/helper 身份、玩家和场景服务；齐备前保持门禁。

新增组合合同的 `--original-gamedata <path>` 模式从 M01/1 原 CPK 构造全部八只 `MonsterSceneActor`，将其按真实遍历顺序交给 runtime owner，并用显式未绑定障碍回调检查失败关闭与 helper 回收。合同不代表主机 RNG/障碍服务或真实游戏帧已接入。
