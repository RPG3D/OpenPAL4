# 移动行为工厂与任务队列

MonsterMovementBehavior将路径0/1与随机3的构造、原Accept、地面/朝向回写、原Release和清理接到MonsterBehaviorTasks。替换旧MonsterPathBehavior名称；头文件monster_movement_behavior.h，源文件monster_movement_behavior.cpp。它使用SceneMonsterInstances实际ActorPlayback、实际XML行为内存和当前motion变换；随机取自同一MonsterMovementWorld，辅助ID由调用方共享序列提供。

class2及其他未实现种类明确拒绝。单轮队列与原事件处理器重复调度已接，见[事件调度](MONSTER_BEHAVIOR_SCHEDULER.md)；外层管理器初始化/状态门禁、真实静态障碍、完整场景渲染、接敌/战斗和ChapterHost仍待接，非空怪物场景门禁保留。

## 帧顺序与拥有者

UpdateMembers复用原65E820，按注册顺序更新全部成员。回调提供真实移动服务，允许查询、积分和位置通知，禁止成员创建/删除/重绑、帧重入及推进/取消角色队列。网格阶段结束后，才按实际管理器顺序推进角色队列；任务Update只做地面/朝向回写，不再次积分。动画阶段与再次发起一轮的时机由场景层提供。

Start()读取实例原定义，不筛掉不支持的种类。class3使用CreateRandom和RestoreMonsterRandom，退出使用SaveMonsterRandom；路径使用原端点恢复/保存。三种行为共用队列、拥有者、错误传播和当前角色状态，不另造随机专用调度器。

队列拥有action，世界拥有helper，实例拥有模型/播放状态。依赖与实际内存节点必须比行为拥有者活得久；场景关闭先显式Cancel，再关闭世界。正常退出走保存与删除；原inactive析构跳过Release时，仅清helper而不写行为内存。

Release的noexcept边界内保存异常记入拥有者，清理后由AdvanceAfterGrid或Cancel抛出。错误后禁止Start/Advance，须停止重建。析构只尽力清理，不能替代显式Cancel的错误观测。helper提前丢失也会报错。

## 验证

主机和安卓合同覆盖路径与随机并行、实际内存/播放、当前motion重启、三类独立范围缓存、正常保存/重复Cancel、inactive析构、class2拒绝、进入重入、XML/待机资源异常、helper丢失与清理。所有合同不执行CSB或推进游戏。

原资源：M01/1四只（含class3的m01-02-02）按未修改的原fuzzy定义完成1083帧；M02/1四只完成2462帧。每只保存一次，最终ready、live_actions=0；两端采样轨迹、动作、队列与内存逐位一致。M01/1另有600帧网格→地面→实例矩阵探针，2400次地面命中、0次未命中，两端报告一致。

```
pal4_monster_path_restore_tests task-source <gamedata> <缓存根> M01 1 <报告.json>
pal4_monster_path_restore_tests mixed-ground-source <gamedata> <缓存根> M01 1 <报告.json>
```

合同target仍名pal4_monster_path_behavior_contract，安卓对应monster_path_behavior_contract。最新证据：H:/PAL4/objs/pal4-android/prepared-data/chapter1-original-runtime/random-behavior-probe。旧path-behavior-probe只覆盖0/1及当时的class3拒绝，已由本轮替代。

源探针使用受控随机、新内存、模型半径、角色与动画阶段顺序、空静态候选，没有原管理器现场、渲染或真实CSB/战斗验收。无fuzzy定义的其他管理器逻辑也未假装完成。用户资源和缓存不进入公开运行时包；未更新已安装APK/NRO或自动推进剧情。

Android双ABI公共库和Switch完整NRO构建通过；Switch本轮仅验证编译，未执行此探针。

原事件处理器现可重复驱动本拥有者，见[移动行为事件调度](MONSTER_BEHAVIOR_SCHEDULER.md)。外层管理器与场景调度仍待接。

## 原包直读回归

`scheduled-source` 的缓存目录参数传 `--original` 时，怪物 DFF、纹理、ANM、地面及墙体均从原 CPK 读取并在内存解析，不读取 `.p4a` 或 `.p4c`。主机及安卓 x86_64 模拟器使用相同资源完成三轮调度：M01/1 的 4 只怪物共 3249 帧，M02/1 的 4 只共 7386 帧；两端报告逐字节一致。报告每 30 帧及结束时采样，覆盖运动变换、动作时间、任务队列和行为内存，不是每帧轨迹或实际画面比较。

```
pal4_monster_path_restore_tests scheduled-source <gamedata> --original M01 1 <报告.json>
```

该探针仍使用受控随机、空静态障碍和受控动画阶段；真实管理器、碰撞、玩家接敌、战斗与章节宿主未接入。报告只供开发验证，运行时不会写这些报告或资源缓存到 SD 卡。

同样使用 `--original` 的 M01/1 `mixed-ground-source` 联合探针在主机与模拟器通过，报告逐字节一致；它覆盖原地面碰撞和怪物变换回写，仍不覆盖真实静态障碍或完整场景玩法。
