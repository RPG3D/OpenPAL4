# 场景路径

已支持直接读取原scenedata.cpk中的pathway.pwy，并在公共ScenePathways中建立可查询网格。怪物的C02/C03配置恢复已有独立公共适配；生产碰撞、角色回写和行为帧仍未接入ChapterHost；非空路径场景的生产门禁保留，第一章尚未跑通。

## 读取与所有权

ReadScenePaths复用原PathwayRecord::Load（IDA522C80），按原序读取名称、网格标记、半径位模式、闭合字节、点数和坐标。网格标记为0时没有后续网格字段；非零原值保留。循环路径的闭合首点只在建立网格时追加，源点不改写。数据读取器与原场景读取器共用有界Reader，拒绝截断、尾随、超量与非有限值。

ScenePathways复制源记录，复用PathwayGrid::Initialize/FindClosestPosition/CoordinatesAt/ProjectToTube（65DB70/65DE70/65DF20/65DD50）。原32位数组地址改为受检查的数值句柄，实际数据由C++容器拥有，不截断宿主指针，不重写查询公式。每条路径保有独立查询暂存字段；只在游戏线程调用。

名称查找保持原顺序首次命中，保留重复名和无网格记录。句柄包含调用方提供的场景generation，重载时必须递增且不复用；错代、越界和对无网格记录进行查询均明确拒绝。源读取保留有标记但零点的记录，网格拥有者拒绝建立这类无效网格。

已有PathwayGrid对空数组、零长度段和分配失败采用安全处理，与原版部分异常数值行为不同。本轮未改这些算法，不宣称完整原二进制数值等价。单点路径投影仍保留原无段命中规则：输出原点，方向参数保持输入，不擅自改成唯一源点。

## 验证

主机与安卓模拟器独立合同覆盖开/闭合路径、单点、无网格、重名首次命中、源副本、失效句柄、查询边界及每字节截断。场景数据和装载合同也通过；Q01/N01、Q01/Q01从原包联合装载回归通过。

| 原包分区 | 路径数 | 源点数 | 怪物显式路径引用/成功解析 |
| --- | --- | --- | --- |
| M01/1 | 8 | 19 | 4/4 |
| M01/2 | 0 | 0 | 0/0 |
| M01/3 | 10 | 52 | 7/7 |
| M02/1 | 13 | 33 | 4/4 |
| M02/3 | 12 | 48 | 5/5 |
| Q01/N01、Q01/Q01 | 0 | 0 | 0/0 |
| Q02/Q02 | 5 | 32 | 0/0 |

共48条路径、184个源点。两端完整源字段和每条路径五处位置/投影查询报告一致；源字段重新序列化后与独立CPK工具提取成员逐字节一致。20个显式怪物路径名称全部找到有网格记录；这是原配置引用诊断，尚未把怪物注册到移动世界。

运行独立原包探针：

```
pal4_scene_pathways_tests source <scenedata.cpk> "M01\1\pathway.pwy" <报告.json>
```

安卓对应程序为scene_pathways_contract，合同无参数执行。证据保存在H:/PAL4/objs/pal4-android/prepared-data/chapter1-original-runtime/pathways-probe/verification.json。只在本机保存用户原字段，不放入运行时发布包；未执行CSB或推进游戏剧情。

Android双ABI公共库与Switch完整构建均通过；Switch未执行本轮路径查询探针。当前安装的APK/NRO没有被这些独立探针替换。原C02/C03 Accept/config恢复已复用，见[路径行为恢复](MONSTER_PATH_RESTORE.md)；下一步继续提供生产移动世界与完整辅助对象。

移动空间世界已能持有路径拥有者、提供只读原路径数值视图并检查空路径场景的generation；PathClosest/PathCoordinates/PathProject通过保留的ScenePathways提供原查询，M01/1与M02/1共52个采样点的主机/安卓报告一致；见[移动空间世界](MONSTER_MOVEMENT_WORLD.md)。
