# 寿阳揭榜：IMM内多个结束动作等待停滞（2026-10-26）

## 最新修复状态

live152双方已正常WM_CLOSE退出0，旧现场不再运行。当前live153从同21800原版writer启动自然复验。
原始诊断与图片全部保留，下方“当前现场”属于修复前历史。

classify后复核当前IDA7DF540/7DFAE0/7DE6C0/7DF4B0/7DE6E0：
IMM进入循环后不重新检查execution_flag，直到IMMEnd才退出；AddWaitHandler仅对非负marker
和相同name去重，EndAction的marker为-1，因此三条等待均保留。7DF4B0删除closed记录后，
当删除位置等于新end（本次删除尾项）时清执行标志并继续脚本，即使前面仍有未完成记录。
7DF511及7DF6B1已添加此次真实证据的IDA注释。

修复去掉NativeScriptFrameRunner的IMM循环内BlocksScript提前退出，保留显式state boundary。
NativeScriptPlayerActionRuntime的单active_end改为NativeScriptEndActionQueue，直接调用已有
共享ScriptExecutionContext::ProcessWaitHandlers处理尾项闭合和执行标志；保留未完成记录，
Tick只报告由阻塞转为可继续的变化，不反复唤醒后续其他服务。上下文与场景解绑仍清理创建方对象。
未修改原NPC可见性、动画完成标志、游戏数值、时钟或剧情值，也没有把bool=false解释成不等待。
原生各等待服务仍分开持有，跨服务的全局注册顺序没有在本修复中全面验收。

新增native_script_end_action_queue_tests：中间闭合不释放尾项；隐藏第一项未完成而可见尾项完成；
未完成记录保留；不重复唤醒；后续新等待重设阻塞；清理；实际SDK编译三条giNpcEndAction的IMM
脚本，并证明三个回调在退出IMM后才等待、可见尾项闭合后继续，以及state boundary仍能截断IMM。

首次应用编译暴露既有IScriptWaitHandlerRuntime重名（计时与基类释放两个不同接口）；
基类接口改名IScriptWaitHandlerBaseRuntime，更新其实现/测试，测试同时包含两头文件防止再次冲突。
build1失败保留，最终build2/test2四项4/4通过2.14秒；独立base1/1通过0.04秒。
完整实际资源cave-memory-smoke退出0，GUI11306/11306及正常shutdown通过。
日志objs/imm-endwait-20261026-{build2,test2,base-test,resource1}.log。
以上不等于完整揭榜或所有IMM组合通过；自然复验仍待完成。

## live153 自然复验完成

同21800原版writer冷启，正常调查MO005，同输入完整推进61段（40874–40933另含47002），
无其他操作强行清flag或推进脚本。step46双方实际40919，GPU图
objs/live153-poster-step46-1184610.png证明越过旧停滞点且camera007对应。
之后正常回Scene/control1，位置/方向/镜头eye-target-angles float32 bits、完整MT624均对应。
汇总objs/q03-poster-20261026-verdict.json。只读双方MN004仍state0/J01/完成0，
MN002/MN003均C01；结果live153-after-IMM-{original,native}-npc-animation.json，
证明没有把隐藏对象强行完成来解锁。

随后正常F7 System保存界面，确认提示覆盖后按writer串行提交并独立归档：
objs/checkpoints/q03-poster-global21900-20261026-live153。两writer实际global21900/arena20，
各219503字节，原版913e6809…、native2bfb4288…。保存前旧scratch完整归档。
live153双方正常WM_CLOSE0退出，live154正在从native来源冷恢复；完整字段与RNG持久化仍待验。
本段揭榜自然闭环通过，不代表跨服务全部wait顺序、所有剧情或全游戏通过。

## 当前现场

live152原版278432/native288592均仍响应，HWND见objs/input-samples-20261026-live152.json。
两端从21800原版writer冷启动，正常Space调查MO005。首次40874由配对GPU完整文字/结束箭头
及mark26哈希核对后推进；之后每次输入前核对两端实际OIRAMLOOK type/page状态。
同输入完成40874–40918（中含47002），driver已正常报错终止，最后journal step45已发送，
不得重发。原版下一句40919，native尚未显示下一句；无额外输入、重启或内存写入。

pair截图objs/live152-poster-step46-observe-1170000.png；原始超时和所有poll保留在
live152-poster-driver1.log及step46文件。超时后重新观测同一pair，差异持续存在；
不是把观测超时误判为进程退出。当前没有active driver。

## 已修复并通过的前一缺口

本轮补齐-278 giNpcFaceToPlayer分发，复用既有ScriptPlayerTurn::FaceNpcToPlayer。
IDA5E4050核实，SDK callback/默认应用2/2通过2.08秒。live152真实越过旧40875失败点，
含false及true wait的该调用均已执行。此修复不代表整个揭榜已通过。

## 停滞点与只读证据

直接GBK读取原Q03.cs func2003：40918后等待0.5秒，IMM中UnHoldAct MN004/MN003/MN002，
再一个IMM中依次EndAction MN004/MN003/MN002，之后camera007和40919。

通过native相关actor地址和+4已核实名称读取动画31词，原版通过原NPC管理树查找读取同字段。
工具/结果：objs/live152-read-{stalled,original}-animation.py，
live152-stalled-{native,original}-npc-animation.json。均为只读，不调用目标函数。

- native MN004 state0、J01、animation words4/5=0、word18完成=0、word19暂停=0。
- 原版MN004同state0/J01/word18=0，但words4/5有值；原版已经进入40919。
- native MN002/MN003仍J01；原版MN002/MN003已C01。

先classify再核实IDA5E3540：giNpcEndAction只读名称，确实忽略注册的bool参数，
始终创建24字节849538等待（wait_mode1）、Flush、清暂停再AddWaitHandler。
因此不能把false当成不等待来绕过。IDA5D55E0核实NPC UnHold与原生复用路径一致。
IDA669C30确实直接读word18；IDA5E04F0只在word18非0时切C01/Z01、restart1并关闭等待。
不能把隐藏对象或空interpolator强行视为完成来掩盖差异。

## 待验证假设和下一步

原版可在MN004未完成时继续，提示IMM批处理/多个等待所有权与解除执行标志的语义需核对。
现有共享script_execution_current.cpp的IMM循环不在循环内重新检查execution_flag；
NativeScriptFrameRunner::ExecuteStep却在BlocksScript时提前退出。
NativeScriptPlayerActionRuntime又只允许一个active_end，FlushEnd总Suspend；
直接仅移除runner的BlocksScript检查会撞上第二个End分配保护，不能只做局部放行。

这些是尚未最终证实的调度假设，不是修复结论。下一步先classify并查当前IDA：
7DF540 ExecuteCurrentModule、7DFAE0 AddWaitHandler、7DE6C0 Flush、7DFAA0/7DFAC0执行标志，
及等待collection update/close的实际语义，再设计模块化所有权和有意义的批处理回归。
保留状态边界阻断；不得跳过脚本、外写完成标志或删掉仍有效的等待来制造通过。
当前未改runner或end-wait实现。需重编译时先正常关闭当前pair，从21800近档复验。

最终21900检查点限定冷读：native writer在live154两端冷读通过，原版writer在live155两端冷读通过；均actual Scene/control1、位姿/镜头float32 bits、完整MT624对应。两份冷读verdict保存在checkpoint目录，当前采用原版source，live155仍在可操作Scene。完整存档字段/RNG持久化未验收。
