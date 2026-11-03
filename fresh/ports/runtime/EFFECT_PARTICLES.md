# 原PTank粒子与公共绘制

`EffectParticle` 从用户原Effect.cpk读取配置和纹理，复用原粒子配置、发射、模拟及五路流输出。Android/Switch共用CPU模块和GLES后端，不发布原EFT、DDS或反编译脚本。

## 模块边界

- 原配置：`RtkPrtStdAtomicConfiguration::Load`（6129B0）。保留CP936发射器名称、原缺省值和布尔语义。`Attr_Prt_AlphaBlend_off=1`经原SetProperties生成源5/目标6；不单独套用原文件的原始blend字段。
- 原算法：`ParticleVisualObject`构造、Initialize/InitializeRender、EmitTankParticles、SimulateParticles、RenderTank、Destruct。发射→模拟→流输出顺序与原645480的无骨骼PTank分支一致；停止发射仍继续模拟，粒子自然到期。
- 资源：有检查的token适配原数值处理器。PTank/材质仅保存所需语义属性，未分配SDK对象副本；粒子记录、曲线、流由有界CPU缓冲拥有。容量1..4096、总缓冲4MiB。配置失败和析构释放计数/资源；文档拷贝归实例所有，纹理共享，缓存/随机服务/计数拥有者须覆盖实例寿命。
- 绘制：原五路位置/颜色/尺寸/UV/旋转流转换为面向相机的四边形。VBO按容量一次分配，逐帧更新顶点；提交已分配槽数，透明死槽保留，不能把原204槽数替换为208存活数。沿用DrawEffect的混合/深度与状态恢复。
- EGL重建只重传GPU数据，不重建CPU粒子、重置随机数或丢弃寿命。

`ParticleRandom`明确提供时间、种子和15位样本服务。开发预览用标准mt19937、固定1234种子对照跨ABI；没有重写VC6私有随机算法，也不声称与原CRT序列相同。帧变换沿用已有公共矩阵边界；原SDK矩阵、quad朝向/UV和最终像素完全等价仍未证明。公共shader沿用现有alpha裁剪规则。

当前明确拒绝collection/billboard、骨骼发射、父级挂载、空纹理和XML文本转数值。已接原Rtk Show/Hide/Advance/Reset/timed及属性/Decress接口，见[EFFECT_SEQUENCE.md](EFFECT_SEQUENCE.md)。预览SetEmission仍仅用于发射诊断，不能替代原Dialog/GobEffect激活、停止和回收。

## 原H_077验证

P_01和lz均为长方体发射器，容量100，共用原`tex\\HY\\smoke 1.dds`，解码RGBA CRC32=2333410632。两模板按原配置产生不同方向、速度、寿命、重力、颜色/尺寸/旋转变化。主机60Hz测试峰值存活70/55、已分配槽91/55；停止发射后全部自然耗尽。

主机53/53通过，Android双ABI、Switch完整ELF/NRO编译通过，未部署Switch设备。安卓每模板180帧，按设备实际double时间步重新读取原包、用主机同一固定随机服务重放，位置/颜色/尺寸/旋转报告值最大差均0。这证明该输入下跨ABI一致，不证明原PC/SDK随机或像素等价。

实际画面与耗尽空白帧比较：P_01变化2577像素，lz变化32554像素；截图已目视。后台冻结、同进程EGL恢复、停止后耗尽、重新发射和切Q01实例数回0通过。反例核对器拒绝修改位置、颜色、槽数及用空白帧冒充粒子画面。

命令：`verify-particles.ps1 -Device emulator-5556`。开发场景为`effect_H077_particle_P01`和`effect_H077_particle_lz`，不增加产品菜单。报告在本地objs/emulator/particle-verification.json、particle-{P01,lz}-{frames.jsonl,comparison.json}；主机原包报告prepared-data/H077-particles-host.json。时间步重放与像素工具为verify_particle_frames.py。

## 下一步

原Rtk控制、Show/Hide/Decress和H_077三对象/十任务已验证。继续连接原Dialog与GOB父级、停止/待回收队列及切场。H_077的Clump模板没有被引用，不需为该序列创建；其他环境效果的实际引用仍需对应分支。完整剧情仍以302C包21句/136调用、停97类型8为证据；本轮不放行97。洞穴剧情/战斗、章节出口、自动缓存与正式无资产发布尚未完成。
