# 怪物行为任务队列

MonsterBehaviorTasks复用原原子选择与四类任务的单轮时序；具体移动辅助对象和重复调度已有独立拥有者，本文旧合同仍只验证队列与显式辅助接口。完整管理器/场景接敌未接，ChapterHost保持非空怪物停止条件。

## 复用范围

- BehaviorAtomicDispatcher（IDA 0x42D4E0）：GroupMode 0遍历、1按原百分比累计、2单项/随机取模。权重不重新归一化；加权未命中保留未处理结果。单项模式2不消耗随机值，空随机选择明确抛出除零错误。
- GomTaskBase/Container（0x5ECEE0等）：等待、进入、运行、完成和回收。等待结束当帧只进入运行态，下一帧才更新；完成后再过一帧回收。串行后继使用原finish_latch及严格大于的等待比较。
- PalBehaviorClassOne及PalBehaviorClass模板：复用原构造、绑定、辅助对象创建、更新、完成和销毁逻辑。class 0/1按GOM时长结束；class 2看辅助对象完成；class 3需有效辅助对象并满足GOM时长。

任务数值状态通过受限句柄和memcpy/字节副本适配，不把64位宿主地址塞入32位字段。名称/节点临时存储用现代容器拥有；任务析构保留原辅助对象激活标志的判断。实际原游戏移动代码仍应通过明确接口接入。

## 辅助对象边界

IMonsterBehaviorActionFactory必须明确支持所需class，并返回具有Enter、Active/SetActive、Update、Complete和Release的真实辅助对象。接口无默认空操作；未支持的class、空返回或生命周期异常均明确失败并清理队列。

Enter失败须自行回滚外部注册；Release不抛异常；辅助对象析构释放剩余拥有资源。factory晚于队列销毁。队列只在单游戏线程使用，Start/Advance/Cancel禁止重入；失败后Advance被拒绝，新的Start可重建队列。

Start先清理上一轮，再按原定义分派。未定义模糊事件返回false；尚未写入组模式的部分定义暂不执行，不猜测原事件构造默认值。队列只执行一轮，不自动重复，也不代替MonsterBehaviorManager/ScriptEventProcessor的事件选择与循环。

## 验证

主机与安卓原生monster_behavior_tasks_contract通过：四类完成规则、首帧/回收延迟、串行等待、三种组选择、随机调用次数、未命中、空组、未知辅助实现、等待中取消、运行中销毁、激活标志，以及创建/进入/更新失败与重入清理。

原CPK探针使用显式合同辅助对象，只测试时序和资源生命周期，不驱动怪物位置、动画、网格或剧情：

| 场景 | 完成单轮的原行为组 |
| --- | --- |
| M01/1 | 4 |
| M01/2 | 0 |
| M01/3 | 7 |
| M02/1 | 4 |
| M02/3 | 5 |

共20组在主机和安卓的帧数、更新/回收次数、队列状态一致；结束后辅助对象和任务为0，临时块为0（队列仍保留一个哨兵）。原定义、来源和合同辅助对象标志保存在`H:/PAL4/objs/pal4-android/prepared-data/chapter1-original-runtime/behavior-tasks-probe`。这些报告不能证明真实巡逻、追逐、战斗或章节通过。

Android arm64-v8a/x86_64开发库和Switch构建通过；未替换APK/NRO交付包或自动操作剧情。具体移动/调度进展见[移动行为](MONSTER_PATH_BEHAVIOR.md)及[重复调度](MONSTER_BEHAVIOR_SCHEDULER.md)。实际脚本容器已可绑定管理器，见[容器视图](MONSTER_SCRIPT_CONTAINER.md)；整对象和ChapterHost仍待接。

工厂选择现经[原注册定义与现代表](MONSTER_FACTORY_REGISTRY.md)解析，再复用原工厂调用/提交尾部；本文旧合同不扩大为全部辅助对象或整章验收。
