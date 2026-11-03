# 原NPC脚本动作

272注册名称为`giNpcDoAction`，此前文档中的giNpcSetAction已纠正。模块直接消费原CSB参数，不增加CS编译或发布资产。

`NpcActions`借用当前`SceneNpcs`，通过集合索引句柄访问同一个`ActorPlayback`。场景每帧推进动画一次，动作模块只推进原处理器和完成令牌；GPU仍从该播放器采样。切场/停止先销毁动作任务，再销毁NPC和模型，EGL重建保留CPU任务。

## 原处理器

- IDA5E3340按名字查NPC，选择原动作、设置restart=1，再创建0x2C/84952C处理器；NPC缺名直接返回，不挂起。
- execute_flag=1将restart清零，只播一次；5E0370观察完成标志后恢复C01，缺失时尝试Z01，再恢复restart并关闭任务。
- wait控制脚本是否阻塞；非阻塞任务仍逐帧更新。execute_flag=-1复用原HOLD监听器，正次数与HOLD算法不另写。
- NPC分支不执行玩家武器/装备显示逻辑。重复同名任务使用已有队列替换规则，旧阻塞令牌收到失败；缺失缓存动作明确报错。

玩家和NPC的原`ScriptPlayerAction`共用一个处理器库，避免两边复制实现。场景API只暴露借用播放器和动作存在性，不暴露二进制XML或GPU类型。

## 验证与边界

主机47/47通过，NPC扩展覆盖同一播放器、阻塞/非阻塞、隐藏冻结、自然完成、默认动作恢复、替换令牌和缺失片段。Android双ABI、Switch公共arena交叉编译通过；Switch首次构建发现for后return的缩进警告，已加明确块后通过，未关闭警告。

用户原Q01的MN001/408包含J01，原408/J01.anm的SHA256为3665d29733f7a8967e9697e79404b40211f5939b7c9affe92da62ea2111b27ad，已与用户CPK逐项核对。时长2.66666675秒；原palactor.cpk无408/J01.amf，按已有可选事件规则读取零事件。主机支持显式动作和同时间采样，设备`npc-action-frames.jsonl`记录实际场景姿势；`verify_npc_action_report.py`检查参数、逐帧时间/顶点变化、非阻塞并行及末帧完成。最终设备结果以[安卓状态](../android/STATUS.md)为准。

目前绑定272与273，273设备证据见下节；269/270运动另见[NPC行走](NPC_WALK.md)。NPC重复服务、自动行为与交互仍需后续接入。HOLD路径有公共原处理器覆盖，但本轮真实CSB只触达execute_flag=1、wait=false。下一类型8场景特效不可用空回调跳过；整章尚未完成。

设备最终验证：APK302CF715…，连续3826、冷重启6147、源码对照6212。21句/136调用/10100/栈深2，272(MN001,J01,1,false)后真实执行1秒等待，停97 Jeffect001类型8特效；NPC非阻塞任务继续自然完成，92个J01帧、任务耗时2.71919227秒、末帧2.66666675秒，缺C01后恢复Z01。原J01同时间主机姿势误差0；动作中与恢复后截图已目视。EGL6→7、CPU显隐/材质保持及设置恢复通过，GL0。篡改完成时刻和全部帧姿势的两个负例均被拒绝。当前开场任务未触达NPC阻塞/HOLD分支，不将主机覆盖当作设备验收。报告见emulator/npc-action-verification.json、npc-action-negative-verification.json与dialogue-verification.json。

## 273结束等待（2026-10-27，安卓设备通过）

分类后复核原5E3540与5E04F0：giNpcEndAction只读取NPC名字，不读取注册的bool；缺失NPC直接返回。存在NPC时构造0x24字节、mode=1的结束等待，清除暂停，再等待真实CompletedFlag。完成后尝试C01，缺失时Z01，恢复restart、清理武器效果、关闭等待。

`NpcActions::End`复用原EndNpc/UpdateEnd，独立保存结束令牌，不重复推进播放器。公共宿主同时路由272/273，bool只检查类型、不改变原阻塞行为。清理借用SceneNpcs的拥有者检查，当前仅支持四个武器/效果拥有者均为空；非空明确失败。

主机扩展验证提前结束不立即成功、解除暂停、动画由场景单次推进、真实末帧后恢复默认动作、缺失NPC不挂起及完成令牌只发一次。59/59通过，Android双ABI与Switch公共宿主构建通过。APK98D1751F…/进程13246从新游戏自然完成两视频、21句实际对白，原CSB146调用顺序一致，跨过273后停未绑定270。273到达时J01已回Z01（time=0.215458989，duration=2.66666675）；实际等待90帧/2.45288682秒，跨过该循环周期边界后CompletedFlag为真、时钟归0，恢复脚本。原处理器并不锁定上一J01，核对器按调用时当前动画和周期核验，不假定必须等J01末帧。提前完成、错误回绕时刻、缺失等待和错误调用四类负例均拒绝。报告为emulator/npc-end-{comparison,negative-verification}.json，末端截图已保存。

`verify-npc-end.ps1`采集原21句输入及之后实际状态；`verify_npc_end.py`独立核对原CSB顺序、实际结束等待/末帧与后续270未实现边界。读取原CPK/CSB，不新增编译脚本或发布资产。
