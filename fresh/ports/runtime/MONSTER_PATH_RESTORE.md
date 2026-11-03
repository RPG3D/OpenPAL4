# 怪物路径行为恢复

已将Win32配置组合提到公共BehaviorHelperConfiguration，并用RestoreMonsterPath适配原C02/C03 Accept。原配置会实际修改辅助对象和ActorPlayback；空间关联、注册和随机服务必须由调用方提供，没有空操作生产后备。

生产空间网格已实现，见[怪物移动空间世界](MONSTER_MOVEMENT_WORLD.md)；原逐帧端点转向/碰撞已独立适配，见[路径逐帧移动](MONSTER_PATH_MOVEMENT.md)；地面/角色回写也已有独立适配，见[地面与角色回写](MONSTER_PATH_ORIENTATION.md)；保存/删除已见[路径行为保存与释放](MONSTER_PATH_SAVE.md)；真实障碍和完整行为工厂仍待接。ChapterHost的非空怪物/路径门禁继续保留；本模块通过不等于洞穴可玩。

## 原顺序与边界

重新经ownership gate核对42A500/42A580、435530/435F20。公共配置器继续调用原路径端点缓存、初始化、基类Dispatch/Reset以及向量/标量访问器；不是另写配置公式。端点初始化先绑定关联，调用原三随机数拒绝采样，再额外抽一次路径方向；非端点基类Reset不抽随机数。

原临时向量地址改为受检查的数值上下文token，数据保存在同步适配器中，不截断64位宿主地址。Configure不可重入，异常后可重新调用；配置器本身保留原部分进度，由拥有者清理。Win32壳只保留实际内存/空间关联边界，关联payload始终引用真实辅助对象，不引用临时大图像；标量与向量实现两端共用。

ConstructEndpoint复用原GridManager与端点构造。标识符由调用方的共享ID序列提供；仅接受未拥有资源的存储，不能替代析构或重置已有对象。基类复制限制在186字节，保留派生尾部。

RestoreMonsterPath只接受class0/1的已构造、未关联、未激活端点对象：

1. 按原4318A0取行为XML首个子节点，解析原路径引用并执行完整初始化。
2. 原Accept自行查找对应ObjectType的AAB_MST_parameters，恢复walk/run速度、外力及pathfollow。
3. 从调用方提供的实际行为内存属性恢复速度、方向、wander和动画时间；不假定mst缓存根就是行为内存。
4. 复制调用方提供的实际角色变换行，插入网格，设置B8；选择C02/C03，失败时按原规则选择Z02，并设置动画循环。保存行为ID与当前class ID匹配时才恢复时间。

调用方必须提供manager半径、实际变换、行为内存及完整世界绑定。失败时移除已成功注册的元素、释放关联并恢复辅助对象图像；动画操作没有事务接口，若动画阶段抛错，调用方须丢弃/重建播放器再重试。AddElement若自身失败，必须自行回滚其未完成注册。缺少参数时原函数会提前返回false，但可能已建立关联；调用方仍负责释放。

## 验证

- 主机与安卓独立合同：两种Accept、默认/保存状态、动作时间匹配、不匹配、Z02回退、构造/端点缓存、零路径缓存保留、基类尾部、随机拒绝次数、无效随机值、非重入、关联/更新/插入/动画阶段失败清理。
- 原M01/1与M02/1配置、路径和已核验怪物模型：共7次恢复，辅助对象全部56个word、动作名、随机次数和采样顶点数两端一致。M01三次class1按原规则回退Z02；M02四次class0选择C02。
- 原资源探针显式使用合同空间世界、空的新建行为内存与固定随机输入；半径取模型绑定包围球，并非原程序manager现场观测。未模拟移动世界、未执行CSB或推进真实剧情。
- Win32隔离构建复用现有protected-page五类辅助对象测试，仅将NPC manager创建改为直接建立原SpatialGrid。短BC/C0对象位于不可访问页边界，真实关联payload、重配/清理、物理步骤和资源归零通过；新增公共端点构造与原Native构造整图像逐字比较。未重跑完整原生package剧情测试。

证据：H:/PAL4/objs/pal4-android/prepared-data/chapter1-original-runtime/path-restore-probe。原包探针命令：

```
pal4_monster_path_restore_tests source <gamedata> <缓存根> M01 1 <报告.json>
```

安卓程序为monster_path_restore_contract，无参数运行合同。Android双ABI公共库及Switch完整构建通过。Switch首次因新代码三处控制流排版触发GCC警告失败；已拆分语句并在保持-Werror的情况下通过。Switch未执行本轮独立恢复探针。APK/NRO交付包未替换，不公开分发探针使用的用户缓存或XML。

新增world-source探针将同场景多只原资源怪物同时注册到生产网格，区别于上述source合同世界。位置更新仍是显式受控通知，未执行原逐帧移动链。


路径0/1工厂已接入原单轮任务队列，见[路径行为队列](MONSTER_PATH_BEHAVIOR.md)。class2及管理器重复调度、完整场景宿主仍待接。
