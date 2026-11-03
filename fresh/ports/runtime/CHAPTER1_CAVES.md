# 第一章洞穴资源检查

2026-10-28：M01/1和M02/1已从用户原CPK重新准备并核验图形/怪物模型输入。怪物拥有者、巡逻/路径和战斗尚未接入，未开放完整洞穴玩法；离开青鸾峰仍未验收。

| 场景 | 当前证据 | 后续工作 |
| --- | --- | --- |
| M01/1 石沉溪洞 | 原包257份输入全部重新提取核验；28份模型缓存（含401/402），8条原怪物记录已通过主机/安卓对照 | 实例资源拥有者已独立验证；继续接世界注册、行为、路径与接敌 |
| M01/2 | 两个sector的UV0共13条含非有限值；SDK和便携原包解析均保留 | 对照原版实际绘制与 GLES 行为，不直接归零或删三角形 |
| M01/3 | 缺Box71LightingMap；SDK及便携原包世界加载成功 | 对照原版纹理搜索和缺失光照贴图的绘制效果 |
| M02/1 | 原包104份输入全部核验；25份模型缓存（含405/407/408），12条原怪物记录可解析 | 实例资源拥有者已独立验证；继续接世界注册与玩法，其余分区继续准备 |

## M01/3 纹理缺失的原版证据

同一原包中 `1\Box71LightingMap.dds` 存在，`2\Box71LightingMap.dds` 和 `3\Box71LightingMap.dds` 均不存在。不能仅凭名称相同将区域 1 的贴图用于区域 3。

IDA 中 `RW_TextureStreamRead`（0x76FC10）在 `RW_TextureRead` 找不到纹理时跳过纹理扩展并返回空指针；`Material_StreamRead`（0x782920）将其结果写入材质，未在该位置直接因空指针中止。`GameWorld_Cleanup`（0x661030）销毁世界后，根据 BSP 等资源路径生成字典键并从管理器移除。上述证据只说明普通纹理读取和场景清理的局部行为，尚不能确定 lightmap 插件缺失贴图时的加载结果、绘制效果或跨场景字典复用。需要用原版或 SDK 对实际 M01/3 做加载和绘制对照后再改变便携解析器。

PC SDK 测试 `--cave-worlds` 使用项目的 CPK 纹理回调，逐区加载并释放世界，统计 SDK 内存中的 UV0 和纹理请求。用户 `I:/Games/original` 的 `m01.cpk` SHA-256 为 `B7C1F7FE2D6EBC919ECF3977BAD9A1A14BF03B63D534258EB56F080A96F5CF5B`：

| 分区 | SDK 加载 | sector | 含非有限 UV0 的顶点 | 缺失纹理请求 |
| --- | --- | --- | --- | --- |
| 1 | 成功 | 38 | 0 | 0 |
| 2 | 成功 | 3 | 13 | 0 |
| 3 | 成功 | 77 | 0 | 15，首个为 `3\Box71LightingMap.png` |

另一份 `H:/PAL4/Games/PAL4_original_steam` 的 M01 包大小不同，得到相同统计。SDK 保留异常 UV，且缺失纹理时世界仍可创建；这不是原游戏实际帧的证明，也不能确定 GLES 对非有限纹理坐标的效果或缺失 lightmap 的管线行为。诊断未绘制画面，缺失计数是请求次数而非不同文件数。PC 测试用 `/DWIN32 /D_WINDOWS /EHsc /utf-8` 构建，旧 AngelScript 等第三方源文件有编码警告；不作为发行构建。

便携 BSP 解析现对 UV0 保留原始浮点位值，位置、UV1 和其他数值仍按原有规则校验。主机和安卓 x86_64 模拟器从同一 CPK 直接解析 M01/2，均得到 5250 个展开顶点引用，其中 43 个引用含非有限 UV0；该统计按三角形引用计数，与 SDK 的 13 个源顶点不直接相等。输入包 SHA-256 为 `0e285b9493601115bf4a984b8888a0b2fc290c69edbe812f5425915c06ce2e1e`。主机场景装载合同测试通过，Switch 共享世界库交叉编译通过。未进行 GPU 绘制或完整章节验证。

M01/3 从原包解析时，光照贴图不存在表示为无第二纹理；贴图存在但解码失败仍报错，普通纹理缺失仍报错。原版重建的 `WorldSectorLightmapPipeline::RenderCallback` 在双重纹理为空时走普通绘制；现有 GLES 路径在无光照贴图时不应用光照参数。主机和安卓模拟器对上述同一包均得到 78729 个展开顶点、802 个绘制批次，其中 23 个无光照贴图；该数字包含原本未引用光照贴图的批次，不等同于缺失请求次数。测试覆盖缺失及解码异常，主机 N01 原包装载通过，Switch 共享场景库交叉编译通过。实际 M01/3 的 PC 与 GLES 画面尚未比较，不能据此声称画面一致。

## 原始输入准备

`prepare_original_arena_inputs.py`复用`collect_inputs/package_location`，按转换清单定位原CPK成员，在新的独立目录提取；不修改游戏、旧解包目录或已有输出。每份输入保存来源、大小和SHA256，失败清单保持complete=false；完成后重新转换并运行已有`verify_arena_sources.py`核对，不沿用旧缓存的通过状态。

例：

```
py -3 fresh/ports/tools/prepare_original_arena_inputs.py <用户gamedata> <旧解包根> <新原始输入根> --manifest <场景manifest.json> --reader <CPK读取工具>
py -3 fresh/ports/tools/prepare_android_arena.py <新原始输入根> <新缓存根> --world M01 --scene 1 --inspector <场景检查工具>
py -3 fresh/ports/tools/verify_arena_sources.py <新场景manifest.json> <用户gamedata> <新原始输入根> <报告.json> --reader <CPK读取工具>
```

这条准备链仍以已有转换清单列举输入，尚非运行时自动解包/缓存生成。若原模型引用集合变化或原成员不存在，保持失败，不能把成功提取当作图形正确。独立工具4项测试覆盖原包字节来源、目录隔离、失败记录和清单越界。

本机新原始输入分别在`prepared-data/chapter1-original-inputs`与`chapter1-original-inputs-m02`，新缓存及核验报告在`H:/PAL4/objs/pal4-android/prepared-data/chapter1-original-runtime`。M01的104份、M02的39份输入与旧解包目录不同，旧失败报告保留在chapter1-caves，不再作为新缓存的状态。

## 怪物模型与权重

按原记录中的模型ID去重生成`arena/<world>/<scene>/monsters/<id>.p4a`，保留全部已有ANM。实例名称、坐标、状态和配置继续读原mstInfo.mst，不写入替代出生点。

407原DFF的3条权重为1.0000001192092896，已与palactor.cpk提取SHA256核对一致。原转换器/运行时逐权重<=1误拒绝此资源；现保留原始值，只检查非负、有限与既有总和0.002容差。不归一化、不钳制；测试检查0x3f800001位模式不变及1.003总和失败。

401/402/405/407/408在主机与安卓独立工具中读取，并对每个缓存动作五个时刻采样，标量诊断相符；报告`monster-model-probe.json`。这是有限值/装载检查，未进行原RenderWare逐顶点或实机渲染对照，不代表怪物已显示或参与战斗。

新缓存只传到安卓独立探针目录，没有替换当前游戏场景；未传Switch。目录含用户资源，不属于公开运行时包。怪物记录读取见[场景怪物](SCENE_MONSTERS.md)。
