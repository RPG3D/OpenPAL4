# 玩家动作与原AMF事件

`PlayerActions`连接原CSB服务245、118和246，复用 `ScriptPlayerAction::StartIndexed/UpdateAction`（0x5E0100、0x5E0370）。传入wait=true时，脚本栈保留请求令牌；只有原动作处理器关闭等待记录后才恢复，不按估算时长放行。

## 资源与播放

ANM沿用已验证解析器生成的用户本地P4AR缓存。101开发缓存包含既有Z01/Z02/C01/C02/C03，以及原开场J08/J13/J14/J16/J17/C08；11段原包ANM与转换源SHA256一致，报告 `prepared-data/actor-101-opening-source-verification.json`。AMF直接从用户 `gamedata/palactor.cpk` 读取，不加入APK。包由共享PackageReader装载，事件复用原 `GameObjectAnimationEventLoader`（0x669DB0）；AMD存在时使用已有现代STL字节解析器，缺失时保留原返回分支。

`ActorPlayback`复用原切换、插值时钟及事件派发规则（0x669750、0x669010、0x66A0D0），用检查过的数值令牌连接拥有所有权的动作、事件与骨架数据，不截断宿主指针。事件按原触发时间和一次性标志派发。当前按动作模式绑定HOLD监听器。未匹配监听器且没有默认回调时，原派发器只标记事件已触发；需要默认处理的所有者可显式传入332字节事件回调。已接入转向结束的首姿势混合；自动下一动作及RPE仍未接通。

真实J08的AMF为400字节、1个HOLD事件，触发时间约2.448276秒；ANM时长约3.600000秒。HOLD先暂停插值器，原动作等待处理器再移除监听器并关闭阶段3等待。暂停后的骨骼姿势保留，不跳到首帧或空闲动作。

资源预览仍可循环采样。游戏播放使用插值器的当前时间和端点钳制采样，防止动画末帧被fmod变成首帧。CPU姿势更新后上传同一玩家绘制资源，EGL重建保留播放对象和当前姿势。

## 所有权与边界

动作处理器拥有原44字节等待状态、名称、阻塞标记和脚本令牌；同名阶段3动作替换会取消旧阻塞请求。每个玩家每帧只推进一次动画，不按等待对象数量重复推进。位置/旋转通知仍使用其共享阶段1/2队列。

当前只允许清理确实为空的命名武器特效；非空特效及装备视觉分支明确失败。NPC、完整动画混合、动态包围盒及其他AMF消息的实际监听器尚未接通。骨骼变形尚未独立证明与原SDK逐位一致；原图形缓存和源码探针仍仅用于开发，最终发布约定不变。

## 解除HOLD与结束等待

118复用 `ScriptActionHoldRelease::Player`（0x5D55B0）：清暂停标志并移除HOLD监听，不立即切空闲。246复用 `ScriptPlayerAction::EndIndexed/UpdateEnd`（0x5E0440/0x5E04F0）：建立独立36字节等待记录，清暂停，等待实际动画完成标志，再切C01（原处理器保留Z01回退）、恢复循环、清空武器特效并完成脚本令牌。动画每帧仍只推进一次。

默认事件回调的旧便携实现不符合原初始化：0x668E04将该槽清零，0x66A17D只在非零时派发默认回调。此前人为报错回调导致正次数J14的HOLD元数据在没有监听器时中止；已移除该人为注册，保留显式绑定接口。失败日志 `emulator/positive-action-hold-failure.log`，IDA两处注释已同步纠正；不是将所有HOLD强行暂停或无条件忽略。

主机覆盖HOLD后继续原动画、完成前不放行、C01切换、重复完成拒绝、非阻塞正次数动作以及显式/缺省默认回调差异。`hold_events`是播放对象累计次数，J17继J08之后为2，不能当作当前片段的独立计数。

## 验证

主机覆盖AMF截断、触发前不放行、一次性HOLD、暂停/重播、动画末帧、原动作处理器回归，以及脚本令牌的挂起/完成/重复完成拒绝和替换取消。

`verify-media.ps1`核对设备原角色包SHA256、主机与设备AMF全部原始字节、实际触发时间、动作期间的不同姿势、后台冻结、恢复及一次性完成。`player-action-frames.jsonl`记录时间和实际采样姿势校验值；`player-action-probe.json`保留完成快照，`status.json`记录实时状态。只证明报告中的实际执行边界，第一章和石沉溪洞战斗仍未验收。

2026-10-27上一轮实测（报告已归档through-unbound290）：APK `122F2425192B5F30993FAE20FEB77F9BF880F8AFA70F5F14B30EB649C9308E8C`，模拟器进程24984，J08在2.47848105秒收到HOLD，完成1次等待；之后完成装备、001/002镜头、WJ001和淡入，完成十四句对白与动作等待后停在未绑定290镜头等待。原触发时间2.44827604秒；1.41114461秒切后台时脚本仍挂起且尚未HOLD，后台播放时间、推进次数及姿势保持。EGL恢复后完成数和暂停姿势保持。

主机在相同设备时间采样，姿势校验误差0；预设容差为 `max(.01, abs(host)*1e-6)`。这验证共享采样与设备实际上传顶点的对照，不是原SDK蒙皮等价证明。292淡入后木屋画面已可见，完整开场尚未完成。

证据位于 `H:/PAL4/objs/pal4-android/emulator/media-verification.json`、`player-action-pose-verification.json` 及 `prepared-data/J08-current-pose.json`。上一轮245终点的报告存为 `media-verification-through-245.json`、`player-action-pose-through-245.json`。主机36/36通过，包含独立原动作处理器合同；Switch共享模块交叉编译通过，尚未实机验证本轮动作。

此前十四句设备流程已执行118两次、246三次并逐一恢复CSB。J08从HOLD时间继续至真实完成标志，切回C01；J17在第八句对白中以非阻塞模式达到原2.31034493秒HOLD，其累计HOLD计数为2，同时间主机/设备姿势误差0，AMF全部400字节一致。正次数J14的未匹配HOLD不暂停，前十四句通过后停290。详见 `emulator/dialogue-verification-through-unbound290.json` 的action_frames与j17_pose。

上一轮APK `882EFC4590E34E2D915B83BB2D5DFAD1830A8513A070B568E13C393AFEA60E86`、模拟器进程26828从新游戏连续完成十七句。118两次、246五次结束等待完成，J16正次数HOLD未被强制暂停，J17同时间主机/设备姿势误差0；下一未绑定服务为255。该轮证据归档为 `emulator/dialogue-verification-through-unbound255.json`。

逐帧转向通过独立[PlayerTurns](PLAYER_TURN.md)复用原处理器，使用PlayerActions选择C02及0.2秒混合回C01。动画仍只在PlayerActions.Advance中推进一次；不把转向伪装为普通245动作，也不另建第二份ActorPlayback。

当前连续21句验证完成118两次与246六次，转向与混合见PLAYER_TURN.md及最新dialogue-verification.json。第20句J14截图已目视，原角色姿势、中文与右侧101B头像可见。

272 NPC动作复用同一ScriptPlayerAction处理器，拥有者与播放器由场景提供，详见[NPC脚本动作](NPC_ACTION.md)。
