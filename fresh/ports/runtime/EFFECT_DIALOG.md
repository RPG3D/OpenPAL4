# 原特效 Dialog 生命周期

本模块服务于原 CPK→CSB→游戏回调主线，不增加 CS→CSB 编译器。`EffectDialog` 位于公共 C++ 运行时，不依赖 Android、Switch 或 GLES；GPU对象仍由绘制层管理。

## 已接边界

复用 `DialogObject::StartPlayback/PreparePlayback/ResumePlayback/PausePlayback/ResetRuntimeState/StopPlayback/UpdatePlayback`，不另写生命周期状态机。所有权分类结果为 OWNERSHIP_REVIEW_REQUIRED；本轮 IDA 重新核对 62A1D0、62A900、62A9F0，并沿用已核对的 62A370、62A5B0、62A620、62A6B0、629660。源处理器的 Pause 名称容易误导：62A620 实际先 PreRender，再 BeginWaiting，开始执行任务。

- `Prepare()` 调原 Start，首次 Prepare、缓存后 Resume。仅准备容器，不推进任务。
- `BeginWaiting()` 调原 Pause，准备粒子后启动原 GOM 等待关系。
- `AdvanceTasks()` 调原 Update。自然 ready 时立即原 Reset，真正销毁任务容器，隐藏对象并从 primitive 更新队列移除，保留对象资源供重播。
- `AdvanceObjects()` 仅遍历登记的对象。与任务更新分开，允许后续 EffectSystem 按原帧阶段调用。
- `Stop()` 调原 Stop，清容器、设置 timed 模式并返回最大原 duration。粒子停止新增发射但继续模拟；不把它替换成 Hide。隐藏会冻结，不能用于这段耗尽期。
- Reset/Cleanup 的 ProcessEffectEvent 是出队，Activation/Resume 是入队，使用不同适配器避免同名接口混用。

EffectSequence 提供分阶段任务接口。它在构造时已创建被引用的粒子/圆柱，Dialog 复用这些 owner；尚未证明与原工厂延迟创建时机完全一致。记录/帧/容器使用局部受检查 token 映射，实际所有权由现代容器和 unique_ptr 持有，不截断 64 位指针、不重建 VC6 私有树或 SDK 对象布局。任务借用目标；释放对象前必须真正销毁任务容器。

## 父级与限制

粒子和圆柱持有本地矩阵及父级矩阵，使用公共列主序 affine 乘法传播到最终绘制矩阵。Dialog 本地位置、旋转、缩放复用既有 InstanceMatrix 数学边界，尚未独立证明与原 SDK 数值完全等价。非有限值、非 affine 矩阵明确拒绝。

裸 Dialog Stop(detach=true) 仅脱离父级；原句柄 UpdateContent 在脱离前保存世界 LTM 并在之后恢复，这属于尚未接入的句柄层。本类没有伪造这一步，也不拥有 active/pending/pool 定时器。

当前只支持全局特效启用、CameraStart=1、DistanceClip=false；距离裁剪、概率、跟踪目标及其他实际引用类仍明确失败。H_077 的 EFT 没有根属性，原 LoadConfig 默认 DistanceClip=true；本轮原包测试显式应用 Jeffect001 的 GOB DistanceClip=0 覆盖，不能把模板默认值当成 0。未来 SceneObjects 必须传入原 GobEffect 创建参数。

## 验证与未完成项

主机全套 54/54 通过；后续增加配置拒绝与任务初始化失败清理后，定向序列/生命周期测试再次通过。覆盖未开始时不执行、自然结束出队/容器销毁、缓存重播、父级变换、停止继续耗尽、脱离父级、非法引用/配置/时间步及失败清理。独立 EffectSequence 的透明死槽仍保留；耗尽检查实际 alpha，而非要求槽数组为空。

原 Effect.cpk 的 H_077 读取三对象十任务，180 帧自然 Reset，缓存重播 created 保持粒子 2/圆柱 1；Stop 返回 2 秒，随后原粒子模拟耗尽并 Reset 出队，析构 live=0。报告位于本地 `H:/PAL4/objs/pal4-android/prepared-data/H077-dialog-host.json.dialog.json`；不是原 EXE 数值对照。

Android arm64-v8a/x86_64 和 Switch ELF/NRO 编译验证覆盖公共代码。本轮没有部署新 APK，也没有在设备运行 Dialog 层；设备最后验证包仍为 9771。原完整剧情基线仍为 302C 的 21 句/136 调用，停第 97 个调用类型 8。

后续原 EffectHandle、active/pending 和缓存已接，见[原句柄与GOB显隐](EFFECT_PLAYBACK.md)。实际 SceneObjects 调用另行验收；初始环境效果、洞穴剧情、战斗和离开青鸾峰仍未完成。
