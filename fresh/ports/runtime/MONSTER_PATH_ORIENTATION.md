# 怪物地面与角色回写

MonsterOrientation已把原C02/C03与class3帧后处理接到公共移动世界与SceneMonsterInstances：探测主地面、修正辅助位置，再更新实例朝向和位置。它尚未接入ChapterHost或完整行为调度，不能据此认为洞穴已经可玩。

## 原行为与接口

复用435D30/436730/437550、600020、443F80和4440C0。Update必须提供原manager的+DC字段；443F80返回其两倍，再乘0.35/2.53形成上下探测距离。不能把helper半径直接当作manager字段。调用方必须保留运行时级MonsterOrientationCaches；C02/C03和class3各自只初始化首次范围，即使当前helper未激活也会先初始化，换怪物或换场景不自动重置。

辅助对象B9非零时才执行地面和角色回写。SceneMovement新增CheckPrimaryAndAdjust，通过原ConfigCompositeCollision竖直线段入口调用已有FindFloorFromAbove；它不切换玩家地面层，也不覆盖玩家LastHit。命中仅替换Y，未命中保留候选位置并继续原朝向/位置回写。只拆出所需的单一clump探测接口，没有空碰撞后备。

SceneMonsterInstances分别保存原始记录快照与可变motion变换；InstanceTransform使用后者，原出生坐标不受运行影响。朝向复用原归一化和yaw计算，保留原X/Z旋转，使用选定宿主CRT acos；未证明原EXE/RenderWare逐位等价。世界Owner句柄检查保证辅助对象回写到创建它的实例，不截断平台地址。

保留原通知顺序：网格先处理全部辅助对象，随后角色行为处理地面/朝向。地面修正的slot1C不再次通知空间节点；节点仍保留该帧网格阶段的位置，下一次移动再通知。场景宿主尚未接此调度，不能在每个helper移动后立即插入其角色回写。

## 验证

主机与安卓模拟器合同覆盖主/次地面隔离、命中与未命中、独立行为缓存、首次范围复用、未激活时初始化、原记录与实例隔离、保留X/Z旋转、空间节点不重复通知、无效范围/句柄/拥有者和删除后调用。旧场景碰撞、世界和怪物移动定向回归通过。

原M01/1三只、M02/1四只路径怪物各连续600帧，包含移动、原地面缓存查询和实例矩阵回写。源探针先更新全部helper，再按显式受控顺序处理角色；不是实际管理器树调度。模型半径作为受控manager输入，不冒称已观测原manager现场。静态候选仍为空，动画帧/渲染尚未参与该探针。

主机：pal4_monster_path_orientation_contract。安卓独立程序：monster_path_orientation_contract。原资源探针：

```
pal4_monster_path_restore_tests ground-source <gamedata> <缓存根> M01 1 <报告.json>
```

报告位于H:/PAL4/objs/pal4-android/prepared-data/chapter1-original-runtime/path-orientation-probe。主机/安卓逐帧完整报告比较结果见verification.json，IDA读取记录见ida-evidence.json。Android双ABI公共库与Switch完整构建通过；Switch未运行本轮探针。已安装APK/NRO未替换，没有自动执行CSB或推进剧情，用户缓存不进入公开运行时包。

下一步仍需原静态候选来源、实际manager边界/半径、完整行为工厂/调度、动画及场景渲染，再接第一章战斗与出口。

原C02/C03保存/删除/待机混合与同内存重新Accept已独立适配，见[路径行为保存与释放](MONSTER_PATH_SAVE.md)；完整工厂、管理器调度与ChapterHost仍未接。

路径0/1工厂已接入原单轮任务队列，见[路径行为队列](MONSTER_PATH_BEHAVIOR.md)。class2及管理器重复调度、完整场景宿主仍待接。

## class3接入

公共接口统一为MonsterOrientation（monster_orientation.h/.cpp）。random缓存独立于kinds[0/1]，调用原MonsterRandomPointUpdate，manager偏移为+4。辅助类型与世界拥有者校验一致；地面修正仍不二次通知空间节点，不修改原出生记录。

主机/安卓合同新增随机独立缓存、首范围复用、命中/未命中、混合类型拒绝；原M01/1四只含class3的600帧地面/矩阵报告逐位一致，2400次命中。证据见random-behavior-probe；任务工厂见[移动行为队列](MONSTER_PATH_BEHAVIOR.md)。这不代表完整场景或战斗已经接通。
