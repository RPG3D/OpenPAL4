# 客房休息尾段的已关闭等待记录

live234九句对白通过后原版解锁、native黑屏。live235从同31500近档复验，在最后一句自然完成之后，共同Space之前启动只读原版队列采样；没有改写故事、角色、计时或完成标志。证据 `H:/PAL4/objs/live235-rest-final-wait-lifecycle.json`。

原版约3.615秒出现未完成的`0x849538`结束动作记录，随后是已关闭的`0x849544`位置记录和`0x849550`朝向记录。NPC此时隐藏，动作C10未完成。约3.670秒后两条已关闭记录被移除，脚本记录+264从1变0，结束动作仍未完成、仍在队列中；后续相机等待、淡入等待分别加入和移除，最终执行解锁并销毁整个上下文。先前“必须等NPC完成”不是原版的行为，不能通过强制完成NPC或删掉等待解决。

所有权分类后核对IDA：`0x7DF4B0`只在删除的记录成为队尾时清执行标志；`0x7DFAE0`按非负marker和名称替换记录。`0x5E08B0`与旋转家族创建的32字节记录在加入时就关闭，但不能提前丢弃，因为它们会影响前面未完成的等待。IDA7DF4B0已补充实际队列证据注释。

native位置/朝向适配器原来各自持有并独立删除完成记录，动作队列因此看不到已关闭队尾。现以`IScriptCompletedHandlerQueue`将共享位置/朝向算法实际成功注册的完成记录传入当前脚本上下文的动作等待分发。根脚本与event/scene/timed上下文分别使用各自动作owner，避免全局队列或串改其它上下文。继续复用共享`AddWaitHandler`和`ProcessWaitHandlers`，未完成NPC记录保留，队尾释放不等于NPC完成。失败分配/缺失角色不产生完成记录。

回归覆盖相同marker/name替换、不同marker共存、已关闭队尾释放未完成前项、关闭的前项不得释放后来阻塞项、旧记录不得反复释放后续等待。真实SDK使用客房尾段签名验证IMM中结束动作→位置→朝向→IMMEnd的顺序与下一帧继续。共享位置/朝向测试2/2通过(.06秒)；应用build2通过，SDK队列/帧调度/回调3项通过(.12秒)，完整resource1通过GUI13214/13214并正常shutdown。

范围：此次接入位置/朝向完成记录与结束动作队列的实际顺序；不代表所有其它类型等待已统一，也不代表全游戏或逐帧/像素等价。床边碰撞滑动差、System Present间歇错误及完整存档/RNG持久化仍开放。

## live236—238 验收

live236从31500同档、授权同床边位置与共同Space，九句41538–41546按源序/编号/hash和双方自然打字翻页边界全部通过；终点双方Scene/control1，native事件finished、wait0、fade_active0。位置/方向/全部镜头字段float32 bits与完整MT624/index对应，配对GPU确认场景恢复。随后双方正常菜单分别写出40100档，326968字节、header场景41、当前仅101。

近档H:/PAL4/objs/checkpoints/q03-rest-global40100-20261027-live236。live237加载native来源、live238加载original来源，分别在两个进程冷读恢复Scene/control1，全部位姿/镜头bits/fullMT对应。此为限定字段冷读验证，不代表完整存档或RNG持久化通过。验收live236-rest-story-verdict.json，冷读证据live237-rest-cold-verdict.json、live238-rest-cold-verdict.json及档案provenance.json。

live235、236、237两端均正常关闭；live238原4160/native39580仍在客房，未运行输入driver。克隆slot0当前为original来源40100。下一主线从func1044出客房，经Q03X出口func1038触发func2011，无需重跑休息。
