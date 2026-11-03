# 怪物场景接入进度

## 原管理器遍历顺序

原 `MonsterInfoManager::ForEach`（IDA `0x524730`）从树头左链接的最小节点开始，随后按红黑树 successor 前进，因此顺序按 MST 名称升序，而不是依赖 `mstInfo.mst` 文件顺序。`MonsterInfoRecord::Read`（`0x524050`）把首个字符串读入记录 `+0`；怪物树插入（`0x525B20`）通过 `VC6_BasicString_CompareRange` 比较输入与节点名称。比较器（`0x406270`）对共同长度调用 `memcmp`，再以字符串长度决胜；这是大小写敏感的原始字节序。原管理器和树实现分别见 `fresh/src/monster_info_manager.cpp` 与 `fresh/src/info_name_tree.cpp`。

名称键来自 `MonsterInfoRecord::Read` 的记录 `+0` 字符串；便携读取器将相同 `mstInfo.mst` 字节存入 `SceneMonster.name`。管理器初始化将 tree `+8`（manager `+28`）清零，`InfoNameTree::Insert` 因而拒绝重复键；便携 `ReadSceneMonsters` 同样拒绝空名和重名。`OriginalTraversalOrder` 现显式复用 `memcmp` 字节比较与长度决胜，避免不同平台字符有符号性造成排序差异。

## 本次实现

`SceneMonsterInstances::OriginalTraversalOrder` 返回实例索引的名称升序排列，供战斗收集器默认构造使用。既有显式顺序构造仍保留，适用于低层合同和受控诊断；生产调用可直接从实例记录取得原 `ForEach` 顺序。

## 障碍候选来源

IDA 核实的消费时序如下：`MonsterManager_ProcessObjects_Extended`（`0x437CF0`）在 manager `+0x5C` 开启时调用 `GridManager_UpdateElements`（`0x65E820`）；它遍历主成员 `[manager+0x3C, manager+0x40)`，对每个成员以虚调用 `+0xA4` 传入 `manager+0x48` 和 elapsed。路径更新 `BehaviorPathEndpointHelper_UpdateMovementAndFlipDirection`（`0x42A760`）把该 context 传入 `ObjectMovement_ProcessAndCollision`（`0x42AB30`），后者遍历 context 的 `[+4,+8)`，逐项交 `Object_CalculateCollisionTime_4`（`0x42C220`）。球候选记录由该消费者按 `+4` 读半径、`+8` 读中心。对应 state 位于 manager `+0x38`：主成员向量是 `+4/+8/+12`，secondary 向量是 `+0x14/+0x18/+0x1C`，空间网格 wrapper 是 `+0x20`。

已查到的专用 manager 写入时序：`GridManager_InitializeMembers`（`0x65E3B0`）将两向量置空；`GridManager_AddElement`（`0x65E550`）只向主成员向量追加；`GridManager_GetPreviousElement`（`0x65E7D0`）返回 secondary end 并将 end 复位为 begin；`GridManager_CleanupMembers`（`0x65E3E0`）释放 secondary begin。怪物初始化 `0x437B20` 顺序为 cleanup、读 MST、建网格、创建 MST 实例，已确认路径未出现候选追加。

另有通用空间球查询：`Grid_ProcessElementsInSphere_Multi_Wrapper`（`0x65E990`）调用 sphere 遍历，命中后由 `Grid_AddElement`（`0x65EA10`）追加 payload 到调用者传入的输出向量；`GridNodeObject` 的 vtable 单元 `0x84B6C8` 指向该 wrapper。它说明引擎能生产某个调用者指定的空间查询结果，但目前没有 callsite 证据表明输出参数是怪物 manager `+0x48`，也没有证据说明被命中的 payload 具备静态障碍球记录布局。原怪物移动另有通过 helper 的 vtable `+0x18` 查询动态邻居并写入独立临时向量的路径，不应与 manager secondary 输入混为一谈。

便携和 native owner 目前都只能读取 secondary 当前内容；怪物/ NPC 行为 session 将这份当前内容直接交给已验证的 movement consumer，未代替生产者填充。producer、筛选对象和跨帧时序尚未闭合，因此不能把全部网格成员、动态邻居或场景几何猜作静态障碍，不改动生产候选接入，也不放开玩法门禁。

## 原包与跨平台验证

`SceneMonsterInstances::OriginalTraversalOrder` 返回升序实例索引，战斗收集器默认构造使用该顺序；显式顺序仅保留给受控合同。host/Android x86_64 原包 instance-probe 报告含完整实例名称且逐字节相同：M01/1 为 8 项，M02/1 为 12 项；两场景报告顺序都与下列原名称树顺序一致。

| 场景 | 名称树顺序 |
| --- | --- |
| M01/1 | `m01-01-01`, `m01-01-02`, `m01-02-01`, `m01-02-02`, `m01-03-01`, `m01-03-02`, `m01-03-03`, `m01-04-01` |
| M02/1 | `m02-01-01`, `m02-01-02`, `m02-01-03`, `m02-01-05`, `m02-01-06`, `m02-01-07`, `m02-02-01`, `m02-02-03`, `m02-02-04`, `m02-02-05`, `m02-03-01`, `m02-03-03` |

证据文件位于 `H:/PAL4/objs/pal4-android/prepared-data/chapter1-original-runtime/instance-probe`；M01 host/android SHA-256 均为 `B044A1984ADBAEB5258AEF19B52F1D20CC92A5E18D3AA336CC4DFD10B1E1B2FC`，M02 均为 `5E6051B16B8DB7186681B5CDF4EADBF8E3165AE80DD75ED12D243B0507B9D2B6`。`verification.json` 标记 host contract、Android native contract 通过，并明确 `original_csb_executed=false`。这些是已有原包实例报告；本次新增原始字节合同需在主机重新构建验证。Android 不部署新包、不启动剧情。报告一致证明输入记录排序与平台一致，不证明游戏中注册顺序、静态障碍来源或真实战斗。

怪物接敌、参数移交和战斗状态仍没有 ChapterHost 生产服务；本改动不放开 `ValidateArenaGameplaySupport`，不表示已能真实战斗。

## 验证

本轮重建后的主机原生合同覆盖乱序名称、大小写、高位原始字节和前缀长度决胜，通过 CTest。历史 host/Android x86_64 原包 instance-probe 报告逐字节一致，精确名称序列及校验和见上表；`verification.json` 记录两端已有的 owner contract 通过。Android 模拟器本轮不重部署或启动新包，因此新的高位字节单元合同尚未在设备端重跑。
