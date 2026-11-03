# 原特效句柄与 GOB 显隐

`EffectPlayback` 是每个场景的公共 C++ 所有者，管理 Dialog、active、pending 和可复用实例；不持有平台/GPU对象。`GobEffectInstance` 只桥接原 GOB 配置与显隐调用。原包与 BinaryXml 解析沿用 EffectDocuments，不增加 CS→CSB 编译或发布原资源。

## 原处理器与容器边界

复用原 CreateEffectObjectWithoutPause（630E10）、EffectHandle 查询/Release/UpdateContent（639FA0/63A740/63A820）、UpdateActiveEffects（631AF0）和 UpdateSecondaryTimers（631F40）。本轮所有权分类与 IDA 核对后，增加三个可覆盖的容器接口：NextActiveNode、NextPendingNode、QueuePendingCompletion。原 x86 默认路径保留原存储访问，公共实现使用 std::map 的查找/后继/emplace，不构造 VC6 私有树。四项原处理器回归仍测试默认路径。

创建时先准备 Dialog，原 GobEffect::SetVisibleAndCreateEffect 再设置父级、BeginWaiting。自然结束先原 Reset，下一次 active 更新才递减次数并回收。-1 按原无限重播，0 按原立即退役；不是额外的粒子定时器。

隐藏通过原 UpdateContent 保存世界 LTM、原 Stop(detach=true)、恢复世界矩阵，再从 active 转入 pending，并使 GOB 句柄无效。粒子继续耗尽。原 pending 在 remaining>0 时只减时间，即使减到零也要下一遍才回收。重复显示会 force Release 原 active 句柄；若旧对象已经 pending，新显示必须使用另一份实例，不能重置仍在耗尽的对象。

句柄含独立组编号和单调 ID，均为受检查 token，不含宿主指针；跨场景/旧 ID 不会误命中新实例。自然回收保留 CPU 实例，重播恢复原池的 position=(0,1,1)、rotation=0、scale=1 和缓存标志。现代缓存目前最多 1024 个槽，未接原按使用次数淘汰策略。场景清理释放 active、pending、缓存和 GPU；不是存档/读档机制。

## 平台与场景

Android 只负责读取用户 Effect.cpk、绘制公共粒子/圆柱、EGL 丢失后的重新上传。GPU 缓存按 Dialog token 分开；透明效果在世界、GOB、NPC 和玩家之后、屏幕淡入淡出之前绘制。

SceneObjects 的类型 8 分支读取原 GobEffectParameters，使用原 GOB 位置/旋转/比例挂接 EffectPlayback，并沿用原 ScriptObjectVisibility 及场景增删回调。无模型的特效 GOB 仅有变换根；不把它计为可绘制 Actor。

2026-10-29 修复碰撞系统已绑定时显示无模型特效 GOB 的报错：变换根不含碰撞几何，显隐回调不再尝试注册其模型碰撞。主机回归覆盖绑定碰撞后的显示、隐藏、零碰撞形状和特效生命周期。安卓 x86_64 无资产构建通过；模拟器真实新游戏从第 21 句继续，两次触发 `Jeffect001` 后执行 NPC 动作、移动和相机，进入第 22 句。未以画面对照验收特效，也未验证 Switch 实机及后续全章。

初始可见环境特效经 SupportIssue 预检后自动激活，不支持的引用类仍保留 initial_effect_pending 和具体原因。N01/H_076三组粒子及dummy Clump已接入；DistanceClip调用共享校验，默认半径1200。CameraStart非1、真实非dummy Clump等分支仍明确不支持。Q01其他效果及完整章节未因此验收。

## 已验证

主机 59/59，包括新增场景类型 8 的原脚本显隐桥接，以及原句柄/Create/active/pending 四项默认接口回归。原 H_077 主机测试覆盖十任务自然回收、保持世界矩阵的停止、两次重播、过期/其他组句柄和清理。

安卓独立完整预览 effect_H077_playback：原 GobEffect 显隐驱动，471 帧按设备实际 float 时间步与命令在主机重新读取原包重放，全部报告数值差 0；两个 pending 重叠耗尽、缓存复用、后台 0.889 秒处冻结、EGL 1→2 和同进程切 Q01 两类实例归零通过。实际变化像素 8621，截图已目视。此预览证据属于 49C1 包/进程 3746；后续真实场景包需单独核验。

篡改 active 数量、任务顺序、pending 时间和父级矩阵均被核对器拒绝。Switch 章节入口已部署测试，音频首段视频失败正单独修复；此处生命周期设备证据仅属于安卓。

命令：verify-effect-playback.ps1 验证完整独立生命周期；verify-scene-effects.ps1 验证真实 CSB 的两次 Jeffect001。后者复用 verify-dialogue.ps1 的 CollectInputOnly 模式收集原 21 句实际输入，再独立核对后续原调用顺序、实际绘制与自然回收；输入检查点本身不充当场景通过结果。

## 真实CSB场景验收（2026-10-27）

最终公共宿主 APK 8ADD7F31…，模拟器进程8320：两段视频自然完成、21句原对白与实际输入、原调用顺序145次一致；两次 Jeffect001 分别有64/65个非零绘制帧，最大210/238三角形。两个生命周期均观察到十任务全部完成/删除，再自然回到缓存；第二次复用实例。最终停原273/giNpcEndAction，尚未接该等待。

核对器原先仅在非零绘制帧查 started==10，漏掉最后 HideobjTask 已隐藏、句柄下一遍才回收的帧。现对整个active生命周期要求finished、started/completed/deleted均10、十任务全部phase4且非live；绘制证据仍单独要求。缺失完成、零绘制和错误CSB调用三类负例全部拒绝。

同进程切Q01后粒子/圆柱实例均0。两次效果截图已目视；报告位于输出目录 emulator/scene-effect-{verification,comparison,negative-verification}.json。该证据不覆盖原SDK整体等价、初始环境特效或第一章后续战斗。
