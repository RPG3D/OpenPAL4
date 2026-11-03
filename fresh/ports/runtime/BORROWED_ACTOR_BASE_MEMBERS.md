# 借用模型的基础成员服务

BorrowedActorBaseMembers 将原 BaseObjectLifecycle 和 BaseObjectMemberRuntime 接到实际 ObjectMemory。原构造442450仍依次建立三个资源管理器、空动画成员、效果句柄、两个字符串和碰撞路径；不使用测试动画/字典回调。

模型和动作由 SceneMonsterInstances/ActorPlayback 拥有。基础成员只拥有原构造分配的字符串和路径，不拥有播放器的局部记录身份。空原动画集合直接初始化/清理；非空动画数组、监听器或脚本记录必须先交还。资源绑定仍由 BindConstructedActorResource 检查用户原包中的DFF、UVA与独立阴影，不支持的动态数据明确拒绝。

## 使用顺序

1. 用稳定的32位逻辑身份构造基础对象。身份区间必须与ObjectMemory分配区间分离；同一服务拒绝重复图像或重叠对象身份。
2. 绑定静态资源，再用同一存储建立MonsterBaseMotion。不要再次构造其碰撞成员。
3. 结束时检查运动Close返回true，确认动画快照已恢复。
4. 结束借用的模型身份，再调用成员Destroy，最后销毁存储。

Destroy先验证动画、UV和阴影均已交还，失败不进入原析构、不修改图像或释放存储。服务不拥有调用方图像，不能自动销毁外部状态；必须先Destroy所有存活对象，再销毁服务。构造失败沿用原逆序回收，并撤销本次身份登记。

## 验证边界

生成模型合同已改用正式服务，覆盖播放器/运动组合、未关闭时析构拒绝、完整回收、重复构造/析构、错误与重叠身份、动态数据拒绝和预算失败后重试。

新增base-lifetime-source原包探针在M01/1八只、M02/1十二只实例上构造真实基础对象，绑定实际缓存模型并完成730/1066帧追逐，结束后原析构检查零分配。主机/安卓报告一致；相对旧主资源探针，仅初始主虚表从GameResourceManager变为BaseObject，其他报告字段一致。

这些探针本身只构造基础对象。原管理器的成员组合另见[怪物构造成员](MONSTER_OBJECT_MEMBERS.md)，尚未接入此真实模型追逐路径。玩家、随机输入、交互与战斗等场景服务仍受控，尚无ChapterHost接敌、CSB推进或GPU验收。正式发布仍为新运行时加用户原资源，本模块不增加发布资产。

主机6项相关合同、安卓模拟器2项合同通过；Android双ABI公共库与Switch完整NRO构建通过。未启动游戏或更新设备安装包。

证据：H:/PAL4/objs/pal4-android/borrowed-actor-base-probe。
