# 怪物基础运动与动画阶段

MonsterBaseMotion复用原442970调用顺序，连接SceneMonsterInstances的实际ActorPlayback、SurfaceMotion驱动的原CollisionActor和实例变换。它可作为AdvanceManagerAfterGrid所需UpdatePosition服务的具体运动部分；不是完整追逐/场景服务。

## 输入与拥有者

调用方提供已加载主体、原初始化后的MonsterBehaviorManagerState、稳定数值token、实际地面、移动高度、世界碰撞查询及资源更新回调。资源回调是必需服务，负责原资源schema/UV/影子等阶段，本模块不提供空成功后备；也不猜初始动作。状态、实例和地面必须比组件活得久。

内嵌+564的40个CollisionActor word由同一SurfaceMotion独占；Start/SetSpeed/Stop与每帧更新会把实际图像发布到管理器。资源回调不能直接覆盖它。SurfaceMotion新增受检查actor_token与原event参数转发，默认8000和事件1保持旧NPC/玩家调用行为；公开快照仅为便携word数组，局部路径token不是存档格式或宿主地址。

组件独占实例的变换绑定；行为辅助对象回写和基础路径运动均通过同一入口，同步管理器位置/朝向、包围盒和位置提交后的球心，再发布实例变换。构造采用管理器初始变换，不改源出生记录；绑定标识检查重复所有者、误解绑和重入。资源回调不能绕过入口改根变换。

销毁先解绑自己仍拥有的入口，再关闭运动并回写清理后的图像；所有权已交接时不覆盖新所有者状态。默认模式释放自己构造的路径；借用模式保留原成员及路径，由完整对象统一销毁。只清理本组件，不代表整个怪物管理器销毁。

## 接入已构造的碰撞成员

构造时传入 constructed_object_storage，可借用原 BaseObjectLifecycle 已构造的+564成员及同一ObjectMemory。仅接受原CollisionActor虚表、尚未放置/启用、空自链接路径。不会再次调用碰撞构造或分配哨兵；未传存储时若发现已有哨兵则拒绝，避免静默覆盖泄漏。

SurfaceMotion借用入口接收成员快照，调用者独占成员，并负责每次操作及异常后的图像回写；MonsterBaseMotion已执行该职责。Close调用原604700解除地面绑定，保留哨兵及剩余节点，最终交由原基础对象析构释放。构造失败不修改原成员或释放其存储。原对象/存储必须比运动组件活得久，禁止外部同时修改路径。

合同覆盖实际基础对象构造→借用路径移动→原对象销毁后零分配；另覆盖MonsterBaseMotion保留同一哨兵、路径运行、退出回写及动画绑定失败回滚。动画/字典服务仍受控，未证明完整怪物初始化或ChapterHost接入。

## 原帧顺序

活动字非零时依次执行资源回调、实际动画推进、原碰撞移动，再由原442970决定是否回写朝向和位置。资源异常留下的图像变化会保留；错误后禁止继续Start/Update，须停止重建，Stop仍可用于清理。网格辅助对象积分与本CollisionActor阶段是不同的原服务，不能合并成重复积分。

+5F4与CollisionActor.status是同一位置，不是独立暂停开关。起始点节点可能单独消耗一帧；零水平速度、到达、停止和原返回值均保留，不添加立即移动补偿。

朝向使用原GameVectorMath归一化与近似acos，保留X/Z旋转及+604反向规则。宿主变换使用已验证InstanceMatrix和原角度归一化；更新实例位置、原八角包围盒及D0→E0球心，单位缩放下保持球半径，返回E0的逻辑目标token。

当前支持单位缩放、单主clump；非单位缩放或附属clump明确拒绝，不能假装已经完成影子/全资源更新。调用方也不能在外层或动画阶段额外推进同一播放器。

## 验证与剩余工作

主机/安卓BaseMotion合同使用生成模型、真实播放器、真实地面路径与显式受控资源回调。覆盖inactive、顺序、首节点帧、路径到达/停止、反向朝向、保留X/Z、包围盒/球心与半径、管理器字段保留、资源异常、非法CollisionActor覆盖、销毁清理及外层入口转发。原SurfaceMotion和原BaseObjectMotion合同一并回归。

最新验证：主机6项相关合同、安卓模拟器2项合同通过。原包M01/1八只、M02/1十二只实例改用原CollisionActor构造及借用运动，730/1066帧追逐报告与改动前及主机/安卓逐字节一致。这里只构造主资源与碰撞成员；完整怪物、玩家/交互/战斗服务仍受控，未验证GPU或推进CSB剧情。

最新证据：H:/PAL4/objs/pal4-android/constructed-collision-motion-probe。早期生成模型证据保留在H:/PAL4/objs/pal4-android/prepared-data/chapter1-original-runtime/monster-base-motion-probe。

本次Android arm64-v8a/x86_64公共库及Switch完整NRO构建通过；Switch首轮缩进警告已修正。未更新设备安装包。

仍需原管理器初始化、真实资源服务、非单位缩放/附属clump、追逐/接敌、名称树和ChapterHost集成。非空怪物场景门禁保留，第一章与战斗未完成。

动画借用退出已改为恢复绑定前快照。完整对象拥有者须先检查Close返回true，再执行原基础对象析构；false表示外部改写导致无法恢复，错误现场保留。生成模型合同已组合基础对象、实际播放器及借用路径的构造/运行/销毁，并使用正式[基础成员服务](BORROWED_ACTOR_BASE_MEMBERS.md)；该合同的资源回调受控，上层场景服务尚未组合。详见[动画视图绑定](ACTOR_ANIMATION_IMAGE.md)。

新增[原成员移动组合](MONSTER_OBJECT_MEMBERS.md)已将同一实际行为拥有者、基础运动和原模型资源接入一条构造/追逐/关闭路径，不再另建队列或处理器。原包20个实例及生成合同验证先关闭运动再销毁成员；MST组件身份和上层场景服务仍受控，尚未接ChapterHost。

此前基础运动阶段主机3项合同、安卓独立合同通过；Android双ABI公共库及Switch完整NRO构建通过。GCC首轮严格缩进检查已修正为明确循环作用域。未更新已安装APK/NRO。

变换绑定合同新增：错误绑定标识/场景代次、重复绑定、字段规范化、回调异常不发布实例、重入拒绝、自动解绑，以及路径0/1和随机3的helper→实例→管理器变换同步。测试仍使用生成模型和显式受控资源服务，不代表真实场景已接通。

原包确认无UVA/独立阴影的怪物已可使用[资源逐帧组件](ACTOR_RESOURCE_FRAME.md)替代空资源回调；动态资源与真实初始化仍待接。

原单主体主资源构造/边界初始化已在专用原模型探针接入，见[主资源初始化](ACTOR_RESOURCE_INITIALIZATION.md)；完整管理器与场景服务仍待接，不扩大上述旧生成模型合同的范围。

内嵌动画现绑定对应ActorPlayback的实际状态，推进后及异常回写前重新载入，见[动画视图绑定](ACTOR_ANIMATION_IMAGE.md)。这不代表完整管理器或动画脚本记录载入已经完成。
