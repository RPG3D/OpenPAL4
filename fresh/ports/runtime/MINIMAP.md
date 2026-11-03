# 场景小地图

`SceneMinimap`接收原包成员读取回调，直接装载 `scenedata.cpk` 的 `<world>\<scene>\MiniMap.df` 和 `MiniMap_<i>.png`。公共C++拥有RGBA图层和对象图标，不依赖Android、Switch或GPU。游戏图像不写入仓库或发布包。

## 复用依据

| 处理 | 原入口 | 公共适配 |
| --- | --- | --- |
| 图层装载 | 0x40CDD0 | 复用MinimapFileLoader；每层保留有符号scale与24字节元数据，默认选择0层 |
| 坐标转换 | 0x40C950 | 复用MinimapGridState；取世界X/Z，按原点和有符号scale换算 |
| 对象图标 | 0x40D3E0 | 复用MinimapSpriteLoader；marker、visible、uses均非零才处理，按原类型选图标，坐标截断为整数 |
| 探索遮罩 | 0x40C420 | RevealAt仅复用原30像素半径、alpha递增及中心0xFE；不代表完整玩家图标/方向更新 |
| 玩家更新 | 0x40E210 | 复用MinimapManagerObject::UpdateGridDisplay；活动图标/图层存在且坐标在图内才更新 |
| 玩家图标 | 0x40EA50/0x40EA80 | 复用居中定位及原朝向三DWORD复制；原actor.png尺寸与RGBA由LodePNG读取 |

PNG使用固定提交的公开[LodePNG](../../third_party/lodepng/PROVENANCE.md)，不重写RenderWare私有图像解码。图层图片和图标列表供后续HUD消费；当前不含小地图GPU绘制。

## 玩家状态与场景接口

`UpdatePlayer(position, rotation)`由250位置服务和66就绪服务共同调用。无图层时按原管理器条件返回，不虚构图标；有图层必须绑定用户ui.cpk中的`minimapIcon/actor.png`。423CC0原控制器选择玩家时添加并激活LeadingRole/actor.png，公共场景装载保留这个玩家图标拥有者。对象图标刷新不会覆盖它。

坐标越界时保留上次位置及朝向；有效坐标先截断，按原图标宽高居中，再逐位复制三项朝向。manager默认模式2执行原30像素探索遮罩。当前没有HUD视口纹理拥有者，按原texture==0分支完成CPU更新并跳过纹理刷新；报告明确`viewport_texture_bound=false`，不宣称HUD已绘制。

`Player/PlayerImage`暴露CPU图标状态和原RGBA供后续HUD消费；`ActiveImage`保留实际修改后的探索图。Android与Switch复用相同模块，文件读取和最终绘制由平台层提供。

## 装载与失败

- 缺少MiniMap.df时没有图层；缺少后续PNG时保留已装载图层，诊断 `missing_image=true`，沿用原装载器的前缀结果。
- 文件截断、PNG损坏、零scale、非有限元数据和预算超限明确失败。最多32层、每层2048×2048、总RGBA最多64MiB。
- 原文件句柄与图像资源改由宿主容器拥有，异常时自动释放；不复制原缺图分支的文件句柄泄漏。
- 有效对象需要图标但没有活动图层时明确失败，不制造坐标。此分支尚未证明原版如何处理未初始化坐标。
- RefreshObjects接收本次对象快照；调用结束不保留外部span。97从同一个公共可变对象集合生成快照，不重新读取GOB静态初值覆盖脚本显隐。

## 验证及接入边界

合成测试覆盖PNG像素、图标筛选、八种图标、正负坐标截断、显隐/uses变化后重建、探索遮罩、无层、缺图前缀、损坏/截断与非法层选择。测试数据为自行生成PNG，不含游戏资产。

新增玩家测试覆盖原图尺寸居中、朝向位模式、边界外保留状态、负scale、无层和未绑定图标失败。主机原包结果：(-575,61,-576)→网格(715,258)，16×16玩家图标左上角(707,250)；探索后RGBA CRC32=1294946805。原actor.png为556字节、RGBA CRC32=1270317509，与独立Pillow一致。`verify_minimap_player_report.py`直接读原MiniMap.df/两份PNG重算坐标、朝向和整张探索图CRC；篡改CRC样例会被拒绝。报告`prepared-data/Q01-minimap-player-independent.json`仅证明CPU原资源更新，设备进展以STATUS及连续报告为准。

主机直接从用户原scenedata.cpk读取Q01小地图：一层、scale18、1317×399、PNG796854字节。LodePNG输出RGBA与独立Pillow解码的CRC32均为3406972356；原PNG SHA256为 `d8bb04a8b1a0213ceeaa6410e2acf844688701909c0c95ffb74acfc63fac078c`。报告位于 `H:/PAL4/objs/pal4-android/prepared-data/Q01-original-minimap.json` 及 `Q01-original-minimap-comparison.json`。此校验只证明该图像解码一致，不证明原RenderWare或完整小地图等价。

已绑定安卓场景对象及97，按原显隐/场景增删顺序刷新CPU图标。设备Q01入口的三次97执行后仅保留entrance1，Entry.PNG坐标(619,208)，随后67返回并恢复调用者，进入55立即模式再隐藏entrance1与title01，图标归零，累计136调用完成258/267及第21句对白及272 NPC动作后停97类型8特效。250的玩家坐标/朝向/探索已通过独立设备核对，66再次执行有层更新；HUD绘制待接入；详见[场景对象](SCENE_OBJECTS.md)。

当前APK与进程见[安卓状态](../android/STATUS.md)。250独立核对网格(715,258)、偏移(707,250)、原朝向(0,186,0)与完整探索CRC；随后253设318、56退出立即模式、120关闭距离插件，66完成输入重置/小地图更新及三个相同时间戳。258行走期间每次实际移动更新同一地面属性和CPU小地图，最终位置(-670,54.5208206,-465)，136调用完成272后停97类型8特效。`minimap-placement-probe.json`保留250当时快照，`minimap-player-probe.json`反映最新更新，避免用行走后的结果覆盖早先证据。HUD仍未绘制；行走后地图全像素尚未独立核对。报告见emulator/minimap-player-verification.json与dialogue-verification.json。

验证工具按阶段分别检查原转向结束186与后续253设置318。完整状态快照继续比较原AMF字节；接续用动作帧的source_hex为空时，仅比较成员/字节数/事件及姿势，并明确记录source_bytes_in_device_snapshot=false。完整本轮记录该标志为true。输入完成后的同进程/安装包/UI哈希检查点允许重试结果核对；跨进程检查点已验证会被拒绝。一次对白轮询不符仍保留证据，原因未定，完整重跑通过；未更改游戏输入。
