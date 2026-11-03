# 怪物移动空间世界

MonsterMovementWorld已作为公共生产后端实现辅助对象存储、空间关联、成员注册、位置提交、球形邻居查询和清理，可供C02/C03原恢复适配使用。它尚未接入ChapterHost。逐帧端点转向/碰撞适配已独立完成，见[路径逐帧移动](MONSTER_PATH_MOVEMENT.md)；原地面/角色变换也已独立适配，见[地面与角色回写](MONSTER_PATH_ORIENTATION.md)；保存/删除也已有[独立适配](MONSTER_PATH_SAVE.md)；真实静态障碍收集、接敌与战斗仍待接，非空怪物场景门禁保留。

## 复用与拥有者

继续调用原GridManagerMembers、SpatialGrid、SpatialGridWrapper、GridNodeObject和端点关联/析构函数。网格链表和查询公式没有另写，邻居不排序、不排除自己，保留原严格距离小于半径的规则及桶/链遍历顺序。

世界拥有辅助对象图像和所有关联，保持path owner的shared_ptr。外部通过场景generation加单调token访问辅助对象；错误代次、已删除句柄、无效地址和非有限查询明确失败。存储使用受检查的数值地址与类型明确的C++对象，不把宿主指针塞进原32位字段。路径点是原ScenePathways的只读视图，不再复制一份顶点数组；未暴露的路径查询暂存字段明确报错，路径查询通过PathClosest、PathCoordinates、PathProject类型接口转发给保留的ScenePathways，不在只读映射上伪造查询暂存区。只接受主路径token，拒绝副接口和无效token；关闭后不可查询，异常会释放重入保护。

构造时需要实际共享随机服务；没有固定随机或空更新后备。辅助对象ID仍由调用方的共享序列提供。原成员帧更新已通过UpdateMembers接入，每次调用必须提供真实帧服务，误用或重入明确失败。回调期间禁止成员生命周期变更，网格阶段结束后再推进角色队列。单游戏线程使用，随机回调期间拒绝重入修改。

正常DestroyHelper调用原成员移除，其release回调按原规则析构并删除辅助对象；Restore失败的RemoveElement只作补偿，移除成员而保留图像，随后释放关联和恢复字段。成员向量容量按原规则保留复用。Close先清成员、再清未注册辅助对象、最后销毁网格和数组，数值块归零；可重复关闭。Helper引用在删除/关闭后失效，必须重新通过句柄获取。

max_helpers与mapped_bytes是有界资源策略。后者统计数值映射的逻辑字节，包含只读路径视图，不能当作精确进程驻留内存；额度失败不遗留成员或关联。

## 汇编纠错

本轮复核65E450汇编，确认网格范围仍为[-40,40)^3，维度是X20/Y1/Z20，共400个桶头、1600字节。旧重建代码的20×20×20错误：反编译伪代码将x87栈上的不同值混为v5。实际先计算floor(20.5)、floor(1.5)、floor(20.5)，随后按Z20/Y1/X20压入CreateGridData。

已修正公共grid_manager_members.cpp、原成员合同及两份旧Win32 owner测试预期，并在IDA添加证据注释。旧20^3报告保留在pre-grid-dimension-correction，不作为最终正确性证据。Win32独立原成员合同重跑通过；两份完整native owner目标及PC剧情本轮未重跑。

原网格还有已知边界细节：浅负侧重叠可能因截断到0而不访问溢出链。本后端保留这个结果，不用全表扫描悄悄修正。原共享算法对部分损坏输入已有安全保护，本轮不宣称全部异常数值行为与原二进制等价。

## 验证

主机与安卓独立合同覆盖同桶逆插顺序、半径等值排除、跨桶/溢出、重配不重复、正常删除、回滚保留、失效句柄、空路径代次检查、资源限额和随机重入失败清理。空路径代次检查最初漏验，已增加Generation接口并在分配前检查。

原M01/1三只、M02/1四只路径行为怪物分别同时进入同一生产网格。两端全部辅助字段、动作、随机次数、邻居报告和逐只释放过程一致，关闭后数值块为0。恢复后关联仍位于原初始化通知的原点，直到显式位置通知；未在AddElement添加额外同步。

新增路径查询验证：主机与安卓模拟器使用同一原资源，M01/1的8条路径、M02/1的13条路径共52个采样点，最近弧长、弧长坐标与管道投影的完整报告逐位一致；还覆盖无效/副接口token、非有限参数、异常后重试与关闭后调用。结果见同目录path-query-verification.json。本次未改原路径算法。

原资源探针使用固定随机、新建空行为内存、模型包围球半径和受控位置通知；不代表真实管理器内存、逐帧运动或剧情已经验收。未执行CSB或自动操作游戏。

```
pal4_monster_path_restore_tests world-source <gamedata> <缓存根> M01 1 <报告.json>
```

安卓对应monster_path_restore_contract。证据：H:/PAL4/objs/pal4-android/prepared-data/chapter1-original-runtime/movement-world-probe，含grid-init-ida.json和verification.json。Android双ABI公共库与Switch完整构建已通过；Switch未运行本轮空间世界探针。APK/NRO交付包未替换，探针与报告中的用户资源不属于公开运行时包。

原逐帧路径转向/碰撞已通过独立MonsterMovement接本后端；地面与角色变换已接独立MonsterOrientation；下一步接真实静态候选、实际manager状态，再接行为工厂和场景宿主。

路径0/1工厂与任务队列已接通，见[路径行为队列](MONSTER_PATH_BEHAVIOR.md)；其他行为和完整场景管理器仍待接。

## class3随机辅助对象

CreateRandom复用原42C8C0构造、65ECD0球内随机向量和42CA00清理。与端点对象共享E0数值图像、关联和成员列表，但保留独立生命周期；不把随机对象的BC字段当成路径地址。公共配置沿842AE8基类分派，保留BC及其后随机半径、计时、方向和起点字段。构造使用共享随机流，校验0..32767；不增加私有随机序列。

本后端在发布句柄前完成构造；随机异常、非法值与回调重入不会留下辅助对象或数值块。生命周期选择由拥有者记录，不依赖调用者修改后的表字段。构造通过同尺寸typed副本复用原算法，不将两种C++类型引用互转，也不编码宿主指针。

主机/安卓monster_path_restore合同新增RandomWorldContracts：端点与随机成员混合查询和原注册顺序、配置不覆写派生尾部、补偿保留/正常删除/Close清理、构造失败与重试、越界随机和重入。没有执行CSB或推进游戏。存储/配置/生命周期已完成；class3恢复与保存现已独立接通，见[随机行为状态](MONSTER_RANDOM_STATE.md)。帧更新已接MonsterMovement；地面回写和单轮工厂已接通，完整场景仍待接。不能据此移除整场景门禁。证据见chapter1-original-runtime/random-world-probe。

本轮Android双ABI公共库和Switch完整NRO构建通过。未替换已安装APK/NRO，FTP安装仍等待当前地址。
