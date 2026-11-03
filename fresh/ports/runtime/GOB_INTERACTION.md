# 普通物件交互

SceneObjects已支持普通GOB（type0）的距离筛选、脚本压栈和使用次数管理。已通过SceneInteraction接到章节确认键；实际取弓流程尚未验收。

## 共用原处理器

FindInteraction复用PalGameObjectManagerVisit::FindClosestRay的坐标查询入口，按原名称顺序遍历。可见性、research标记、剩余次数和8450动态许可共同筛选候选；不把绘制数量当作atomic许可。距离查询复用GobObjectBase::IsWithinInteractionBounds与原近似模长，保留水平范围包含边界、垂直范围严格排除边界的规则。调用者必须提供已核验的角色垂直半径。

服务122/giGameObjectSetResearch复用原5D5720处理器，只修改research标记。回到小木屋的原初始化会显示MO002/MO003并开启research；不能仅显示模型而漏掉这一步。

普通GOB交互复用5A4490及59A920：使用次数为0则退出，负数表示无限；活动中的同名脚本直接清理返回，不重复扣次数。注册原场景CSB模块、压栈和释放解释器后才扣正数次数。模块注册失败的原分支仍保留。已拆出IGobNormalScriptRuntime与IGobResearchUseRuntime，原重建和便携端共用实现，避免依赖模型装载及快照接口。

原接口把模块注册、压栈和解释器释放误称为异常处理，现已按IDA调用链修正命名。没有重写脚本SDK算法。

## 验证

5项定向主机回归通过；另补的8450动态许可测试通过。覆盖脚本服务122、隐藏/禁用/用尽、垂直边界、同距名称顺序、活动脚本不扣次数、正次数耗尽及原负数行为。Android ARM64/x86_64构建通过。

真实原包探针在主机与安卓一致：N01选择MO002，从原script.cpk准备func2002，使用次数1→0；Q01选择物件1，准备func1012，次数FFFFFFFF保持不变。探针显式开启目标的显示/research作为隔离夹具，未执行场景初始化、取弓剧情或修改正在运行的游戏。两端仍使用既有经CPK哈希核验的缓存。

报告位于H:/PAL4/objs/pal4-android/prepared-data/event-collision-runtime中的gob-interaction文件。

## 尚缺

共用SceneInteraction已复用原PalActorControlInteraction的NPC/物件/机关优先级、按下/释放及模态清理，主机/安卓原生合同通过；实际应用按键和原文字提示窗口已接线，见[确认键控制器](SCENE_INTERACTION.md)。本模块的Interact是对象回调，不能跳过控制器中的教程/提示门控。其他GOB类型当前明确拒绝。

耗尽后的自动消失仅记录disappearance_requested，尚未接原活动对象队列与材质淡出。取弓物件的原配置不请求该淡出，但仍不能把一般物件淡出算作完成。物件动作重置、存档点、NPC真实对话和第一章流程仍待完成与验收。EVF机关附近选择已接公共控制器。

本轮开发APK未安装；模拟器继续使用上一版场景事件开发版。用户原资源及转换缓存不进入公开运行时包；启动时生成和失效机制尚未完成。
