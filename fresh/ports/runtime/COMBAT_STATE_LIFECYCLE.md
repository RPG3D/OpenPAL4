# PALIV_COMBAT 状态桥接

`portable::CombatStateLifecycleBridge` 为 PALIV_COMBAT state ID 3 提供 ChapterHost 可拥有的 Entry/Render/Update/Leave 组合。它直接调用共享原逻辑 `CombatSceneCoordinator`、`CombatSceneFrame`、`CombatSceneUpdate`，不另写战斗流程：Entry 复制请求 XML 并执行原 UI/标志/相机/选择分支；Render 保留 state 0/非活动时先绘制底层场景的顺序；Update 保留 Combat、UI、结果文档和 state transition 次序。

`portable::CombatRuntimeOwner` 把该 state bridge、`CombatSystemService`、`CombatSystemColdRuntimeAdapter` 与 `CombatWorldResources` 放入同一可调用 owner。宿主为 PALIV_COMBAT 注册 state ID 3、分配 generation 并绑定下述真实 typed services 后，调用 owner 的 Entry/Render/Update/Leave/Retire；Entry 中 `BeginCombat` 会经共享 `CombatSystemStart::StartWithServices` 调用 CombatSystem Start，Start 的 `LoadWorld(scene, area)` 先从原 `PALWorld/CombatWorld/<area>.cpk` 解析 BSP，再同步调用宿主 `LoadWorld`。组合层持有 CombatWorld lease 到 Leave 完成；renderer 在活动期通过 `BorrowWorld` 取得临时 lease，并必须在 Leave 前归还。Leave 完成后组合层归还自己的 lease 并 reset world。若宿主仍持有 lease，reset 会显式失败；Leave 已完成后可归还宿主 lease，再重试 `ReleaseWorldResources`，然后才 Retire。`Retire` 要求 dispatcher 已解除绑定且 World 已释放。

Entry 复制的请求 XML 由 PALIV_COMBAT state owner 持有。传给 `CombatRuntimeOwner` 的 `RequestConfigResolver` 必须在 `AssignRequest` 后返回该 state-owned XML 的逻辑句柄，不能返回即将销毁的调用参数，也不能把 Android 指针截断为 x86 地址。原 Quick/选择对话框的 UI 回调仍由 `ICombatSceneServices` 负责；选择确认后可调用 `StartAssignedRequest(selection)`，它复用共享相机选择和 Start 分支。

owner 不伪造平台操作。宿主必须提供并在 readiness verifier 中核验：`ICombatSystemColdRuntime` 的 logger、SkillSystem、translator 之外的 console/event、MatFX 和 CombatSystem cold 标记映射；`ICombatSystemStartServices` 的真实 XML child/attribute 读取、collection 与 roster 清理、console、inactive 判断、game manager/UI、怪物创建、玩家创建/检查及 ready tail；`ICombatSceneServices` 的 UI、游戏标志、请求 XML 复制、相机、选择框和 Combat UI；`ICombatSceneFrameRuntime` 的状态/inactive/arena frame、完整 Combat renderer；`ICombatSceneUpdateServices` 的 Combat 与 UI 更新、结果 XML、状态路由；以及精确绑定当前 PALGameIV ID 3 owner 的 `ICombatStateReadiness`。Start 的 world callback 负责加载以外的宿主 world/game-manager owners，并可通过 owner 借用已加载 `Scene`。缺少任一真实绑定都应在 readiness 或调用处失败，不得把空操作解释成可玩。

cold translator 由 `CombatSystemColdRuntimeAdapter` 持有，并在共享 5754C0 的冷提交之前加载。若 Start 异常且平台 cold 标记仍为 0，组合层释放 translator catalog；提交标记后抛错则保留 catalog，因为系统已完成冷启动。world CPK 加载或宿主 `LoadWorld` 回调失败时组合层尽力释放自身 lease/reset 已解析 world，并原样重抛原异常；其它宿主 Start 副作用遵循现有 Entry 异常合同，不承诺整体回滚。translator 只能在 owner 已 Retire 且所有消费者停止后调用 `ReleaseTranslator`。

桥接对象借用同一个已注册 state owner 的 `generation`、四个服务面和 readiness verifier。verifier 必须确认 generation 对应的仍是当前 PALGameIV state ID 3，并检查 Entry/UI/camera/Combat Start、底层场景帧、Combat renderer、Combat/UI update、结果文档和状态切换服务已绑定；它还要在 dispatcher 解除绑定时报告 owner 已不再注册。verifier 在构造与每个回调前运行；缺失服务在进入共享算法前失败。请求 XML 和 frame 是 ChapterHost 的逻辑句柄，不编码宿主指针。

生命周期阶段为 Ready、Active、Faulted、Retired。Entry 返回值只代表原 PALIV_COMBAT Entry 接收了状态：原算法即使 Combat Start 回调返回 0 仍会返回 1，不能把 Active 当作 Start 成功。Update 的返回值原样保留共享实现的 XML destructor 返回值，也不能解释为战斗结果。IDA 0x58AFD0 先推进 Combat 和 UI、再读取状态508；仅 state 7 分支构造结果文档并同步调用 PALGameIV 状态/UI 路由。因此桥接只允许这段 Transition 回调内同步派发的 Leave；Combat 或 UI 更新期间其他回调若尝试 Leave 会在改动标志/UI 前被拒绝。Update 以外的宿主状态路由仍可直接 Leave。共享算法异常后桥接进入 Faulted，阻止继续 Render/Update/Entry；若 owner 已成功进入，仍允许一次原 Leave 清理已保存标志和 UI。只有确认无待 Leave 且 owner 已从 registry 移除后才可 Retire。Entry 抛错时，ChapterHost 的 Entry adapter 仍须按原服务所有权处理部分副作用，不可假设桥接回滚。

portable runtime 现提供 `portable::CombatSystemService`，把宿主绑定的 `ICombatSystemColdRuntime` 与 `ICombatSystemStartServices` 组合起来：它在原 Start 算法请求冷启动时调用 `CombatSystemSession::Initialize`，其余操作继续经宿主服务执行 `CombatSystemStart::StartWithServices`。这复用 5754C0 冷初始化和 5756D0 Start 的共享原顺序，不创建占位角色、怪物、世界或结果。传入的 system owner 是逻辑句柄；Android 宿主必须由真实服务映射它，不能把 64 位宿主指针截断成句柄。`Start` 的 `true` 仅表示原 ready tail 完成输入回调、state 1、prepare 事件和音频原点复位；`false` 仍是原 Start 拒绝，异常仍向上传播，均不能解释为胜利或已进入可玩战斗。

Android 还提供 `portable::CombatTranslationCatalog`：`ReadFromGameData(<game root>/gamedata)` 由原 `PackageReader` 从 `database.cpk` 读取 `combatDb.db`，再调用共享 `GameDataFileLoader::Load`、`GameDbArchiveReader::ReadPropertyRecord` 与 `ReadTable` 解析链。只接受精确 `CombatTranslation` 表及 `English`/`Translation` 两个字符串字段，复制各行原始字节并维持原行序，`Find` 保持大小写敏感且不伪造 fallback。它不使用 NativeGameDatabase 的 32 位地址模型，也不保留包文件或流；返回的 `string_view` 在 catalog 销毁后失效。原安装包校验为 229 行，`EASI_NONE` 对应原 GBK 字节 `CE DE D2 EC B3 A3 20`。

`portable::CombatSystemColdRuntimeAdapter` 现把 catalog 接入真实共享 `CombatSystemSession::Initialize`：它验证原 translator 子对象 `owner+380` 与 `combatDb.db` 路径，在 `LoadTranslator` 中加载并持有 catalog；SkillSystem、console、MatFX、日志和冷标记写入仍由平台 owner 执行。适配器只在 catalog 已成功加载后允许 `owner+517=1` 冷提交；加载失败时共享冷初始化会抛错，且不会写 cold 标记。平台 Combat UI 可通过该 adapter 的 `FindTranslation` 使用同一份原表。`ResetTranslator` 只能在 Combat session 已结束且消费者都停止后调用；它释放表并使已返回的视图失效。

`CombatRuntimeOwner` 只组合已重建算法与平台 typed service 面，不注册 PALGameIV owner、不实现这些服务，也不会单凭 Start ready 标记证明用户可进行战斗。攻击输入、技能/目标选择、伤害与角色状态推进、敌方 AI、行动次序、战斗结束与奖励仍须各自接入真实 CombatSystem owner；这些功能没有由本组合层补造。

## 接线边界

- PALGameIV 初始化必须创建并注册唯一 ID 3 owner，并为其分配单调 generation；readiness verifier 检查 registry 仍指向该 owner。
- ChapterHost 或等价宿主持有真实平台 service adapters 和 CombatRuntimeOwner；所有借用对象必须比 owner 活得久。
- ChapterHost 的状态路由须把 Entry/Render/Update/Leave 四个 PALIV_COMBAT 回调派发给该实例；Update 引发的同步状态转移必须在结果 XML 释放前派发 Leave。
- `CombatRuntimeOwner` 的宿主服务须由真实 XML、CombatWorld 后续 owners、game manager、UI、怪物和玩家 owners 组成；只在 readiness 已核验后启用。任何缺少的操作都必须显式失败，不能用空回调或测试替身标记 ready。
- 离开时完成 Leave 后解除 registry dispatcher，再退役桥接和各服务；重入 Combat 创建新 generation。
- 构建系统需将 `src/combat_state_lifecycle.cpp`、`src/combat_system_service.cpp`、`src/combat_system_cold_runtime.cpp`、`src/combat_translation_catalog.cpp` 和共享 `combat_system_session.cpp` / `combat_system_start.cpp` / database archive parser 加入 Android portable runtime；将专属合同 `tests/combat_state_lifecycle_tests.cpp`、`tests/combat_system_service_tests.cpp`、`tests/combat_translation_catalog_tests.cpp` 加入测试目标。数据库合同验证损坏/截断输入和原包读取；cold-init/service 合同可通过 `pal4_portable_combat_system_service_tests I:/Games/original/gamedata` 验证 catalog 在原冷初始化序列中加载。

生命周期合同验证共享算法的调用顺序、owner/readiness gate、outcome XML 销毁前同步 Leave、Combat 更新期 Leave 拒绝和异常熔断。Combat service 合同验证冷初始化仍调用共享 `CombatSystemSession`，Start 仍通过共享 `CombatSystemStart`，缺少玩家时不提交 ready，且原 package translator 通过 cold-init adapter 在 `cold+517` 提交前加载，database 缺失不会写冷标记。翻译合同验证原解析算法、原包 229 行/GBK 字节语义、截断和 schema 错误及 catalog 重建。state3/Start 之外的 ChapterHost 服务仍未接线；这些合同不证明真实 Combat 进入或完整战斗可玩。
