# M07对象动作脚本接入

当前IDA5E6B60（已更名giObjectDoAction_CreateNamedAnimationWait）查找对象，实际Type6才操作object+476动画。当前记录名忽略大小写相同则SetTime0，否则vtable+4选片，然后Pause0/Restart1。原具名等待为{name}-OBJ-Action，phase4，目标次数与对象指针进入44字节8495F4处理器；5E6D90按完成标志计循环，正次数达到后SetTime0/Pause1并关闭，-1则SetTimeLength/Pause1关闭。0与其他负数不在Update内自动关闭；5E6E30清计数并关闭，不暂停/归零对象。IDA已纠正原Dialog_Create/Update的误导名称。归属检查为PAL4域胶水，优先复用已有ScriptObjectAction，共享RenderWare动画和原生队列服务，未另造定时等待。

NativeScriptObjectEffectsRuntime已有每SDK上下文的对象具名队列。现增加ActionAdapter/Entry，借用NativeSceneGobAssets实际对象、NativeGobAnimationBinding的Select/SetTime/Length及共享GameObjectEmbeddedAnimation的Pause/Restart/Completed，不在等待器里推进动画时钟。主、事件、并发与定时上下文既有OpeningWait object-effects端点同时推进动作等待；新-298回调使用独立object_action服务，调用共享ScriptObjectAction。

新增实际Q01 entrance1 SDK资源用例，检查同片重置、等待-1末帧停留、两循环归零、非等待0循环继续注册、同名替换、取消不暂停及缺失对象边界。build1通过；resource1用例错误放在入口初始场景，该场景无Q01 entrance1，已移至后续确有实际OT01 HAnim资源的Q01检查点，build2通过，完整resource2通过GUI12996/12996正常shutdown。既有共享script_object_action focused1通过。日志H:/PAL4/objs/m07-object-action-20261027-*.log。

真实M07仍从40900近档重新求解，未写结果/时钟/故事/entrance2。live257旧版在成功后-298退出；新实现在live258已越过-298，原始成功分支执行到Scene/control1。钥匙B记录和restart1/completed1/pause1双方对应，位姿/镜头float32 bits/fullMT一致；没有重写CSB或写入剧情。成功帧25803为480000/480000像素相同；后续场景帧有明确门位移差，不能推广像素一致结论。


## 后续Machine任务差异

live258最终GPU25805原版门抬起，native仍关闭；原门position[-5.8694000244,230.8972015381,-1680.1715087891]，native[-5.8694000244,90.8972015381,-1680.1715087891]，差140高度。门本体没有动画记录，两边钥匙B动画已经相同，因此问题不在新-298末帧等待。脚本自然启用entrance2但不证明门实体已经移动。

NativeActorControlInteractionRuntime::TouchObject仅处理Action6/Effect/普通Type0，Machine5直接落到非0返回，未调虚表+50。当前IDA读取848F58+50实际5A22C0 soGOBMachine_TryTriggerTaskRun，已反编译并核对现共享GobMachineTrigger::TryStartTaskRun：检查运行/使用次数、清两容器、库存条件、扣正使用次数、vtable+44构建任务，再开始处理/更新标记与可选GameEvent。已有GobMachine TaskBuilder/TaskFactory/Control/Inventory及各GOMTask共享实现应优先复用，原生仍缺对应组合/逐帧任务更新。不能直接设置door Y或在GOMTouch时只选动画。

实际只读内存证据H:/PAL4/objs/live258-door-animation-states.json及live258-door-gobs-native.json；旧扫描初稿对Action偏移取错，最终比较统一+1DC，结论以door-animation-states为准。未尝试穿过未正确打开的门，未保存不一致新档。双方WM_CLOSE正常退出，launch driver终止，无pendingGPU。完整验收边界H:/PAL4/objs/live258-object-action-verdict.json。下一近档仍40900。


2026-10-27后续：Machine Start16/Move0已按原XML接入，live259首门实体精确升高140并进入通道一；两writer冷读门位姿对应，但镜头位值仍partial。当前进展及未完范围见fresh/native/renderware/m07_machine_tasks_native.md，-298和首门Machine触发已越过。
