# 场景碰撞候选列表调查

## 调查范围

2026-10-29 使用 IDA 当前数据库核对 `I:/Games/PAL4_game/launch.exe`，image base `0x400000`。目标是石沉溪洞怪物移动所读的障碍候选列表，未推断未经验证的碰撞来源，也未修改 `ValidateArenaGameplaySupport` 门禁。

## 已证实的消费路径

- `MonsterManager_ProcessObjects_Extended` (`0x437CF0`) 在 manager `+0x5C` 为真时调用 `GridManager_UpdateElements` (`0x65E820`)，随后遍历怪物对象树并运行活动对象。
- 怪物 GridManager 状态从 manager `+0x38` 开始。主成员指针向量在状态 `+0x04/+0x08/+0x0C`。传给移动对象的 secondary 向量上下文地址是 manager `+0x48`（状态 `+0x10`）；嵌入的 begin/end/capacity 字段分别位于 manager `+0x4C/+0x50/+0x54`，与移动函数读取 context `+4/+8` 相符。
- `GridManager_UpdateElements` 逐个遍历主向量，并对元素调用 vtable `+0xA4`，参数为状态 `+0x10` 和 elapsed。怪物 path helper 的该槽为 `BehaviorPathEndpointHelper_UpdateMovementAndFlipDirection` (`0x42A760`)。它把传入 context 交给 `ObjectMovement_ProcessAndCollision` (`0x42AB30`)。
- `ObjectMovement_ProcessAndCollision` 从 context `+4` 到 `+8` 遍历候选指针，对每项调用 `Object_CalculateCollisionTime_4` (`0x42C220`)。该函数从候选 `+4` 读半径、`+8` 读中心位置。确认的是消费布局与时序，不是列表的 producer 或记录类型来源。

## 写入路径与生命周期

- `GridManager_InitializeMembers` (`0x65E3B0`) 初始化时把主、secondary 两个向量均设为空。
- `GridManager_AddElement` (`0x65E550`) 将元素追加到传入对象的主向量 `+4/+8/+0xC`。IDA 当前数据库中它的 9 个代码引用均来自 MST/NPC 行为 `AcceptRestore`，把移动 helper 加入其所属 GridManager 主成员集合；没有引用显示它写入 monster manager 的 secondary 向量 `+0x48`。
- 每帧的 `GridManager_UpdateElements` 只读取主向量并转发 secondary 向量地址，没有自身追加候选的路径。
- `MonsterManager_Initialize` (`0x437B20`) 先清理，再加载 MST 信息、创建 GridManager 空间网格，最后创建 MST 实例。MST helper 加入主向量的已见路径在各行为 `AcceptRestore` 中。
- `MonsterManager_Cleanup` (`0x437B60`) 先销毁怪物树中的对象，再调用 `GridManager_ClearElements` 清空主向量并对元素发出释放回调，随后复位 GridManager 状态并释放 secondary、primary 两个向量的缓冲区。初始化与清理都没有证实 secondary 候选如何填充。

## 易混淆的空间网格

`GridManager_InitializeGrid` (`0x65E450`) 创建的 `GridNodeObject` 持有空间网格。其 vtable 槽 `+0x14` 指向 `Grid_ProcessElementsInSphere_Multi_Wrapper` (`0x65E980`)，后者调用网格球查询；命中回调 `Grid_AddElement` (`0x65EA10`) 会将命中节点的 payload 追加到**调用者传入的输出向量**。这是引擎通用的空间查询 producer 能力，但现有静态 call/reference 证据没有证明输出向量是怪物 manager `+0x48`，也没有证明 payload 是 `Object_CalculateCollisionTime_4` 所需的半径/中心记录。

移动对象另有持有空间网格接口的动态邻居查询和临时输出路径。它与传给 `ObjectMovement_ProcessAndCollision` 的 secondary vector 是不同上下文；不能仅凭共享空间网格或相似的球体查询，将动态邻居、场景几何或所有网格对象认作静态障碍候选。

## 结论与后续证据要求

目前已证实候选列表的消费者、记录读取布局、每帧传递方式和 manager 清理顺序；没有证实候选列表真正 producer、筛选规则、写入时序或对象生命周期。便携/native owner 读取现有 secondary vector 的做法不能替代其 producer。不能据此填充猜测候选，也不能解除洞穴怪物玩法门禁。

继续调查需要找到至少一个能追溯到 monster manager `+0x48` 的实际输出参数调用点，并确认追加 payload 的布局与其存活/清理时序；否则本项继续保持未闭合。ChapterHost 组合若被该门禁阻止，也不能用空列表或通用世界几何冒充原版候选。

## 隔离运行尝试

2026-10-29 从 `I:/Games/PAL4_game` 复制到 `H:/PAL4/objs/pal4-android/scene-collision-live-copy`，未在复制过程中排除或改写游戏资源；排除项仅为 IDA 数据库、dump 与备份文件。隔离副本 `launch.exe` SHA-256 与 IDA 当前输入完全相同：`C0A8745111B30BB98AD08AADC7EDC53BEC8856102612AD96E8F970FEFC9F4DFF`。该副本独立启动一次后，PID 77156 在创建可见窗口前退出；副本 `pal4Logfile.log` 只有 `uiFrameManager`、`fsObjectFactory`、`logger` 的 singleton-destroy 记录，随后 `pal4Exception.log` 写入 `SEHException` 分隔符。没有进入菜单/场景，因而没有 manager 指针，也没有任何 secondary begin/end 的运行时样本。此启动失败只说明目标基线目前不能用于 M01 动态采样，不说明候选列表为空。

另核对了已有的 `about-page-reference-copy`：其 `PAL4.exe` SHA-256 为 `9B7F7824B47CB6A06893A544323BB4F9692A79321A052AEABB341CE4035F88C3`，与 IDA 当前 `launch.exe` 不同。逐字节比较显示仅 62 个文件偏移不同，但差异包含可执行代码区域，且该副本已有记录为建窗前 SEH 退出；故不把它当作可用原版运行基线。现有 `comparison-game-20261024-memory` 有用户运行进程，未读取或控制。当前没有可用的隔离动态结果，门禁维持未闭合。

## 2026-10-30 静态调用点复核

本轮重新枚举 `MonsterManager_GetInstance(0x437770)` 的全部直接引用，得到 28 个调用者；其中 27 个可生成伪代码，`game_world_ProcessFrame(0x519C50)` 未能反编译，不能将其算作已排除的写入者。快照与逐地址 ownership 检查保存在 `H:/PAL4/objs/scene-obstacle-static-20261030/`。

`MonsterManager_CreateMonster(0x437FA0)` 将同一 manager 的 `+56` 成员块写到怪物 `+516`，再调用原组件初始化。因此仅检查 singleton 的直接调用者不能穷尽后续写入：还须沿怪物持有的成员块别名及行为回调追踪。场景载入、更新和缓存恢复的已复核直接调用点未出现可证明的 secondary producer；这不是“原版候选始终为空”的证据。本轮没有变更候选来源或场景门禁。
