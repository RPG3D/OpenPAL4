# 随机行为恢复与保存

公共RestoreMonsterRandom/SaveMonsterRandom复用原436C40/4373E0，不重写另一套行为规则。它们使用CreateRandom创建的E0辅助对象、实际行为XML内存与ActorPlayback；共享配置、数值属性写入、空间拥有者及动画适配与路径行为共用。

恢复按ObjectType 3查找AAB_MST_parameters，设置最大速度、力、范围与manager半径×1.5，再应用内存速度和当前角色变换。上次行为ID为3时，仅当三个起点都小于FLT_MAX才覆写构造起点，并恢复动画时间；其他ID使用辅助对象当前位置。原Sound命名只是共享常量3的旧名字，这里没有新增声音机制。C02不存在则选Z02，并设置循环标志。

保存写last_behavior_id、speed、actiontime和start_x/y/z共六项，保持已有属性类型及无关属性/子节点；随后清活动字节、从网格删除辅助对象，最后0.5秒混合C01，缺失时尝试Z01。不会增加C03特有的restart调用。

无匹配参数时返回false，配置可能已绑定关联，调用方必须销毁辅助对象；不能把false当作无资源。恢复异常补偿成员和关联并还原辅助字段；动画已发生的变化不保证回滚，重试须重建播放器。保存并非事务：XML异常时可能已经部分写入，待机资源异常时辅助对象已经删除，调用方必须停止并重建。注册但暂停的对象仍可保存。非有限输入由移植边界明确拒绝，不宣称与原异常浮点行为完全等价。

## 验证范围

主机和安卓独立合同覆盖新进入/同类恢复、FLT_MAX坐标保留、C02/Z02与C01/Z01回退、六字段保存/再恢复、暂停对象、无效类型/未注册对象，以及注册前后和删除后的异常边界。

原scenedata.cpk中M01/1记录3的class3配置，原palactor.cpk动作消息及用户生成模型缓存，已在主机和安卓模拟器完成恢复→保存/删除→待机混合→再恢复。原动作实际走Z02/Z01，3636个采样顶点有效；全字段、XML字节和动作报告两端逐位一致。

命令：

```
pal4_monster_path_restore_tests random-cycle-source <gamedata> <缓存根> M01 1 <报告.json>
```

安卓对应monster_path_restore_contract。报告保存于H:/PAL4/objs/pal4-android/prepared-data/chapter1-original-runtime/random-state-probe。

探针使用受控随机、新内存、模型包围球半径与动画seek，没有执行移动、CSB、战斗或整场景。class3帧更新已接MonsterMovement；地面回写和单轮工厂已接通，完整场景仍待接；整场景门禁保留，第一章未完成。用户资源与探针报告不进入公开运行时包。

Android双ABI公共库与Switch完整NRO构建通过；未更新已安装APK/NRO。
