# 原H_077三对象、十任务

`EffectSequence` 直接消费原EFT计划，按ObjectList创建对象，再按TaskId绑定完整任务列表。复用原Dialog任务初始化、GOM调度器及Show/Hide/Decress/Scale/ChangeColor处理器，不将每个对象的任务拆开播放。

## 引用与所有权

H_077的ObjectList依次为P_01、lz、yzt3。模板字典另有一份ClumpObject，但原`DialogObject::ActivateEffects`只遍历ObjectList（629BC0）；未引用模板不能当作必须创建的对象。此前把这份Clump列为H_077播放前提的描述已纠正。N01/H_076确实引用mx Clump，但原包 Attr_Dumy=1；零售模式保留对象身份并跳过装载、任务和绘制。真实非dummy Clump仍明确不支持，不能整体跳过该类。

当前工厂按原EffectSystemFactoryRegistry类表选择已验证的粒子/圆柱owner；未知或未接的**被引用**类型明确失败。对象、文档副本和CPU数据归序列所有，任务借用对象，先销毁任务再销毁对象；纹理缓存、随机服务及计数拥有者须晚于序列销毁。重播调用原Reset，复用对象和资源，保留原全局随机服务的后续状态，不重新构造或重新播种。

## 原控制

- Show/Hide复用RtkPrtStdAtomicControl::Enable/DisablePlayback（614320/614340），同时控制更新和待绘制状态，不能只停发射。
- Advance复用原614260的配置/运行状态检查，timed模式跳过发射但继续模拟。Reset复用6140B0与ParticleVisualObject::Deactivate，恢复原变换/颜色并清掉活动粒子。
- Decress复用61DE50/61DEA0与614A50/614A70。减量模式按原最大容量/时间计算整数速率；原处理器决定何时更新限量，不能替换成每帧线性alpha淡出。Scale/Color同样走原粒子属性接口。
- GOM等待关系保留整个十任务顺序。P_01的Show等待0.5秒，lz和圆柱立即开始；圆柱颜色等待0.3秒，Hide按前一任务完成状态启动。某对象已经隐藏时，其他持续任务仍可能运行，不能提前宣告整个任务容器结束。

主机60Hz下约2.333秒十任务全部结束。`TasksFinished`只表示任务容器结束，**不表示原Dialog句柄、GOB待停止队列或剧情97已经完成**。诊断Stop立即清任务并Hide；生产GobEffect的pending/drain/回收生命周期仍待接。

## 验证

主机54/54，包含跨对象等待、延迟Show、Decress、未引用模板、缓存重播、非法引用回滚，以及隐藏时冻结、Reset和timed不再发射。Android双ABI、Switch完整ELF/NRO编译通过；未部署Switch设备。

安卓三对象十任务记录180帧，按设备实际float时间步在主机重新读取原包重放。任务名/顺序、等待/持续参数、阶段、Rtk控制/减量字段均匹配；两份粒子位置/颜色/尺寸/旋转及圆柱矩阵/颜色报告值差0。约2.32355秒自然完成，started/completed/deleted均10，三对象均隐藏。此结果未证明原CRT随机、SDK矩阵或最终像素完全等价。

运行与结束截图对比有7372变化像素，已目视。后台1.325秒处冻结，EGL1→2保持同一组对象；重播后created仍为2粒子/1圆柱；切Q01两类live计数均0且进程不变。反例核对器拒绝修改任务顺序、提前Show、Decress速率及伪造十任务完成。

开发命令：`verify-effect-sequence.ps1 -Device emulator-5556`，场景名`effect_H077_sequence`。报告在本地objs/emulator/effect-sequence-{verification,comparison}.json及frames.jsonl；主机原包报告prepared-data/H077-sequence-host.json。未增加或发布原资产。

## 剩余主线

公共原Dialog启动/等待/自然重置/停止及父级矩阵已接，主机原包验证见[Dialog生命周期](EFFECT_DIALOG.md)；独立序列预览继续使用原诊断模式。本轮未在设备执行Dialog。继续接GOB句柄/待回收队列，覆盖重复启用及切场释放，再验证真实97调用。其他原环境效果仍需实际引用类型的工厂分支。当前完整剧情证据仍为302C包21句/136调用、停97类型8；后续包尚未重跑完整剧情。洞穴剧情/战斗、章节出口、自动缓存与正式无资产发布未完成。
