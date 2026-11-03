# 原场景特效文档与任务结构

`ReadGobEffectParameters`复用原`GobEffect::ApplyConfigValues`；`EffectDocuments`通过用户原Effect.cpk读取`script/<name>.eft`，沿用现有BinaryXmlConfig解析器和包解码器；`ReadEffectPlan`复用原DialogObject::LoadConfig及EffectSystemTextManager::LoadFromXml。本模块只装载配置；播放与GOB生命周期由[EffectPlayback](EFFECT_PLAYBACK.md)处理。

## 所有权与语义

文档缓存持有原字节与解析树，返回只读共享引用。每个播放实例将拥有独立任务、变换和效果状态；这些不写入缓存。装载失败不留缓存，可修正源后重试；当前只有内存缓存，没有磁盘缓存失效机制。

原DWORD接口使用受检查的节点索引，不截断64位指针。最终任务结构持有文本/节点副本和STL容器，不模拟原VC6容器布局。保留ObjectList和TaskList顺序、复制任务参数、按标签登记OxnList及目标坐标；原DistanceClip默认1、CameraStart默认1且负值钳0。缺OxnList或重复模板标签沿原处理器失败。原XML文本转数值边界尚未接入，遇到此类型明确失败。

GOB缺配置节点返回空值，保留调用者已有状态；存在配置节点时才使用原属性缺省值：times=0、saver=0、distance_clip=1。不能将这两种情况混合。负times保留原DWORD位模式，-1为4294967295。

EFT可能包含CP936字段，例如H_077粒子发射器类型为“长方体”。诊断JSON逐字节转义，原字节保留在source中；JSON里的非ASCII字符串不能当成已经解码的游戏文本。浮点报告另保留原位模式。未知对象/任务完整保留，`playback_bound=false`，不冒充支持。

## 原资源结果

直接从用户scenedata.cpk重提GameObjs.gob，并从Effect.cpk逐份读取、核对输出字节与SHA256：

| 场景 | 类型8对象 | 原EFT |
| --- | --- | --- |
| N01 | 1个，初始可见 | H_076 |
| Q01 | 15个，其中13个初始可见 | H_077、H_224、Y_006、M_011 |

Jeffect001：H_077，times=1、distance_clip=0、初始隐藏；Jeffect002也使用H_077但distance_clip=1。初始环境特效times=-1。不能只接剧情中的Jeffect001而遗漏环境特效生命周期。

H_077原文档10719字节：ObjectList依次P_01/lz/yzt3；4份模板包括两个RtkPrtStdAtomicObject、一个虚拟ClumpObject和一个CylinderObject。10项任务依次为Show/Decress/Hide（对象0）、Show/Decress/Hide（对象1）、Show/Scale/ChangeColor/Hide（对象2），3项对象引用与4份模板不是同一数量，不能按序号直接配对。每项TaskId保留原对象索引。

报告位于`H:/PAL4/objs/pal4-android/prepared-data/effect-original-documents/`：source-verification.json保存5份原EFT来源/哈希、类清单和原处理器生成的任务顺序；N01/Q01子目录保留诊断JSON、任务TSV和用户本地原字节。这些文件不进入发布包。

## 验证与后续

主机48/48；覆盖配置节点作用域、缺省/负数、原解析、缓存身份/失败重试、路径拒绝、任务顺序、默认标志、重复模板与缺OxnList。真实N01/Q01全部文档经原配置算法读取成功，并核对H_077中文发射器原字节。Android双ABI、Switch公共scene_data交叉编译通过；Android首次暴露旧IGobEffectRuntime的三处缺失override，已修正且未关闭警告。JSON首次导出CP936原字节导致UTF-8读取失败，已修复转义后重读全部原文档。

后续原圆柱、PTank粒子、完整三对象十任务和Dialog生命周期已分别验证，见[EFFECT_SEQUENCE.md](EFFECT_SEQUENCE.md)、[EFFECT_DIALOG.md](EFFECT_DIALOG.md)。原句柄、active/pending与GOB显隐见[EFFECT_PLAYBACK.md](EFFECT_PLAYBACK.md)。独立播放与实际CSB场景分别验收，旧302C包21句/136调用仅属于此前边界。

原CPK→CSB→执行优先，暂不实现CS→CSB。剩余环境特效、实际引用的Clump/嵌套类、距离裁剪与切场仍需逐项验证；RenderWare私有实现继续遵守所有权清单。发布目标保持新运行时+用户自备原包，不分发原资产或恢复CS。
