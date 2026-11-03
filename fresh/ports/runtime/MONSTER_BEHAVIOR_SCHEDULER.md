# 移动行为事件调度

MonsterMovementBehavior新增SetSchedulingEnabled和AdvanceScheduledAfterGrid，复用原42E240事件处理器与42D470延迟条件文档，对实际MonsterBehaviorTasks重复分派。内部适配器位于monster_behavior_scheduler.h/.cpp，不提供空操作服务；就绪、分派、更新和随机均绑定拥有者的实际队列与共享world.Rand。

当前每实例按实际fuzzy定义保留一个MST-Fuzzy-Behavior事件视图；没有fuzzy时事件列表为空。条件文档是C++拥有的实际BinaryXmlNode，首次过滤时调用原延迟创建函数建立，后续复用。仅绑定4318A0安装的594E00常量真回调，不伪造其他条件成功。数值token是作用域内受检查的视图，不编码宿主地址或模拟旧STL树。

原顺序保留：禁用则不推进；容器未空闲时只推进现有任务；空闲时过滤事件、选择候选、清候选end、调用原原子分派，然后按返回低字节决定是否本帧推进。只有一个事件时事件选择不额外取随机，组内原比例/随机选择仍由原分派器执行。队列Advance现返回原GomTaskContainer::Update结果，没有制造更新成功值。

默认调度关闭，已有Start/AdvanceAfterGrid继续用于单轮调用。启用后使用AdvanceScheduledAfterGrid，不能同时调用单轮接口。禁用不取消当前任务，也不会移除网格成员；它只停止事件处理器推进，不能当作暂停整个怪物。暂时取消可禁用后Cancel；终结生命周期使用Close。任务保存后原phase4还需下一次推进才删除任务节点并就绪，不能以saved计数代替Ready。

## 验证

主机/安卓合同验证路径0与随机3三轮、禁用/恢复、延迟条件文档只创建一次、无fuzzy不造任务，以及保存资源失败后的停止/清理；原单轮和队列回归通过。

原M01/1四只（含class3）三轮3249帧，M02/1四只三轮7386帧。每只dispatches=3、saved=3、condition_documents=1、live_actions=0；轨迹、动画、队列、调度和最终XML报告两端逐位一致，关闭后网格块归零。

```
pal4_monster_path_restore_tests scheduled-source <gamedata> <缓存根> M01 1 <报告.json>
```

安卓对应monster_path_restore_contract。证据：H:/PAL4/objs/pal4-android/prepared-data/chapter1-original-runtime/behavior-scheduler-probe。

这里完成事件处理器与真实任务队列的重复运行适配，4317F0门控与实际队列已有[外层入口](MONSTER_MANAGER_FRAME.md)，但真实管理器初始化及追逐/碰撞/基础动画/位置服务仍待接，也未接437CF0名称树遍历、实际静态候选或ChapterHost。调用方最终必须遵守这些原门禁，不能无条件逐帧调度冒充完整管理器。源探针仍使用受控随机/初始内存/半径/角色和动画阶段顺序、空静态候选；未执行CSB、渲染或战斗。

Android双ABI公共库与Switch完整NRO构建通过；未更新设备安装包或自动推进剧情。第一章与无资产正式发布仍未完成。

处理器、事件与逻辑分配现有[原生命周期适配](MONSTER_PROCESSOR_LIFECYCLE.md)；上文旧探针的完整管理器限制保持。
