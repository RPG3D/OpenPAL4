当前2026-10-26 live148：4回调/名称生命周期/可选音乐及AI多候选修复后，实际双2582胜利/251抽取对应，进入40738和2L影片，后续剧情仍在测试。详见../renderware/combat_ai_candidate_ownership_native.md和objs/live148-work-state.json。以下为保留的历史失败及修复证据。

# M03 夜半猎熊回调接入

脚本直接读取用户提供的 `I:/Games/original/gamepatch/gamedata/EditData/script/M03.cs`（GBK），func2003；从真实 global20900 夜景档验证，不重跑已通过前缀。

## 当前 IDA 证据及复用

先运行 classify_library_ownership.py；5D3AD0、5D77E0、5D7810、443DE0、443F20、5E4520、5E7620 均返回 OWNERSHIP_REVIEW_REQUIRED。当前 IDA 已逐一确认属于游戏回调/对象算法，SDK材质调用仍调用已有 RenderWare 库代码。

- -73 / 5D3AD0：复用 ScriptPlayerActions::DetachCollision，原生已有受所有权校验的 movement Clump 解绑及 B10 清零。
- -186 / 5D77E0：固定 Player1，调用443DE0；复用 BaseObjectMaterialTexture::SetLeg、既有 CPK 纹理缓存和 RpMaterialSetTexture。原脚本纹理名是 `103_2 (ss)`。
- -187 / 5D7810：固定 Player1，调用443F20；复用 ApplyLeg，恢复554保存路径/560材质。无LEG材质时遵从原算法返回0，不伪造更换成功。
- -280 / 5E4520：复用 ScriptPlayerBlend::BlendNpcIn。8495AC+4 指向5E7620，已在IDA更正为 PlayerBlend_UpdateIn；alpha0→255，完成恢复不透明并关闭等待，不移除NPC。+8 指向5E3300通用清理；与out的5E44D0清理不同。

NativeScriptNpcBlendRuntime 同时接受 in/out vtable，按类型更新和清理，保持真实NPC入场、事件、材质及脚本等待所有权。取消in只恢复材质/关闭，不误用out离场。

检查发现前次新增窄材质接口覆盖同名旧共享声明；恢复 IScriptPlayerMaterialRuntime/ScriptPlayerMaterial，原生窄接口改名 IScriptPlayerTextureRuntime，保持旧共享模块可构建。

## 验证记录

共享淡入与材质回归2/2通过（objs/hunt-callbacks-20261026-shared-test1.log）；新增半程alpha及完成后不移除NPC检查。真实玩家资源probe将直接共享detach改为编译giPlayerDetachCollision，验证分发实际退还movement borrower。SDK新增4项缺服务检查与腿伤/治愈参数分发。

应用/SDK/真实玩家资源 build1/test1 全部3/3通过50.58秒（真实资源48.48秒），日志objs/hunt-callbacks-20261026-{build1,test1}.log。加共享回归合计5/5通过。尚未声称自然func2003、真实LEG材质或两只2582战斗通过。live144已正常WM_CLOSE两端，保留20900真实检查点；新live145正从该真实档启动。

## live145 自然复测新崩溃

原275108/native293504从20900真实近档冷读、实际Scene/control1/fullMT624；共同事件前定位(450,-10,-1360)+W24正常触发func2003。两端共同第一句40688后，driver相同文字结束/分页predicate逐句40689–40732，共44项journal，再无输入。首次NPC淡入native访问违例退出；原版仍响应并正常WM_CLOSE，回执objs/live145-original-close.json。

Windows Application1000/1001记录c0000005/RVA410462；本次Release符号表pal4_application_entry_Release.map映射VA810462到NativeScriptNpcBlendRuntime::ActorName+22。NPC+4为0，构造string_view导致strlen空读。当前IDA归属门禁43C960/4427C0/665CC0并逐一核实：原创建路径LoadClump应分配/复制实例名至+4。原生NPC builder借用Clump绕过该元数据；已有Monster builder已复用NativeBorrowedActorBinding::BindLabel，NPC现直接复用同一函数，仍由原资源内存owner析构释放。ActorName增加有名称前提检查。没有改为猜名或固定行为。

真实玩家资源回归入口新增VerifyNativeNpcBlendLabels：遍历实际M01场景NPC，以原生淡入runtime.ActorName返回值回查实际actor和行为info，要求同一actor/非空info。本轮label build1/test1待完成；自然复验仍待进行。

pairedGPU截图live145-loaded-night-601001.png显示夜空原暗/native蓝，既有sky资源/渲染集成仍开放，不能声称画面完全一致。只读live145-lingsha-leg-initial.json两端初始LEG560均0、stored554长度0；未将无材质回调无操作称作换肤成功。

名称修复test1真实资源失败13.59秒：`PAL4 loaded player reset needs resource services`。新增的名称allocation还必须在场景reset释放；NPC UnbindBorrowedActor此前只清Clump借用，后接空资源tail检查+4失败。按照已有MonsterObjectOwner同一路径复用GameConfigState::FreeAndNull，在NPC unbind时释放/清零自身名称，避免泄漏与重复释放；此失败保留于hunt-npc-label-20261026-test1.log。build2/test2验证名称发布与资源释放完整生命周期。

label-test2越过此前卸载失败并正常完成M01开场，但新增probe误用已经移动给second_scene的opening_scene指针而访问违例；该探针错误不作为游戏修复失败证据，也不算通过。已改为受非空检查的当前native_second_scene_npcs，build3/test3重新执行。记录保留于hunt-npc-label-20261026-test2.log。

最终label build3/test3通过2/2，49.16秒（真实资源47.18秒）。真实M01全部NPC名称由淡入runtime回查到同一actor及真实info，且此前资源卸载通过。日志objs/hunt-npc-label-20261026-{build3,test3}.log。新live146从相同20900真实档冷启，待自然跨过40732原失败点；不将资源回归当作自然复验通过。

## live146 自然跨越NPC淡入及战斗音乐新阻塞

原291020/native295896从20900真实档同setup/W24；共同40688–40737，MN001/MN002淡入完成，两端40733“呀！是妖怪！！”截图objs/live146-hunt-step44-724410.png，真实模型可见，原失败已跨过。双方分开采样并非动画时间锁步/全部像素一致。driver1首次bootstrap在对白尚不存在时收到Space，随后因尚无mark25在任何journal输入前停止；同现场确认40688两端自然完成后一个共同Space初始化，driver2采用新request继续，无重启、无重复journal输入。

40737之后原版进入战斗，native错误`PAL4 script function is unavailable`正常异常退出，driver停止，原版正常WM_CLOSE于objs/live146-original-close.json。追至RunMusicInitializer：原始Music.cs有M03A_COMBAT_MUSIC/M03C_COMBAT_MUSIC而无M03B_COMBAT_MUSIC，原生无条件Queue缺失函数。归属门禁5140A0/513EE0/7DE4C0/7DE860/7DEE60；当前IDA5140A0及7DEE60证实缺函数asNO_FUNCTION在插入上下文前返回，音乐缺初始化不阻断原战斗。

修复在RunMusicInitializer中复用已有StartSceneInit的asNO_FUNCTION门限，不伪造音乐脚本。真实应用资源probe追加实际M03B不存在时保留track/story、实际M03A存在时通过SDK回调写入P38.mp3；仅fixture恢复原saved track。hunt-music-20261026-build1/test1正在进行。真实双2582战斗入口尚未通过。

音乐修复build1/test1最终2/2通过50.43秒（实际资源48.42秒），真实Music.csb缺M03B保留track/story、存在M03A写P38.mp3；日志objs/hunt-music-20261026-{build1,test1}.log。新live147原296428/native293876从同20900近档复验，入口objs/live147-work-state.json；尚待自然进入战斗。

## live147 实际战斗入口与下一AI阻塞

原296428/native293876同20900真实档与setup/W24，再次正常40688–40737/两NPC淡入。音乐修复自然跨过原失败，两端actual combatstate1/worldfalse，完整MT index5及624词相同；角色1001(522/100/74)、1002(347/100/52)、双2582各(1000/15/150)，阵位/yaw对应。稳定包装objs/live147-hunt-combat-wrapper.json。配对GPU捕获分开取帧，原版仍黑转场/native已显示战场，动作条进度亦存在相位差，不能称锁步或战斗画面完全一致。

进入战斗后尚未发送任何玩家战斗输入，native自动AI抛`AI already owns an unpublished action`退出。原版仍正常，已只读归档live147-original-after-native-ai-failure.json并正常WM_CLOSE（live147-original-close.json）。两端均结束，无driver。

静态定位NativeCombatAiActions::Impl只保留一个unpublished任务/owner；共享AiActionPipeline::SelectMagicAttack会先创建多个特技/法术候选，再随机选择并释放其余。原2582技能5703/5804触发了第二候选门限。归属门禁560280并当前IDA确认候选vector、random%count、先析构未选对象、返回所选的原流程。此AI缺陷本轮尚未修改代码；下一步原生改用候选任务到实际创建domain的持有表，继续调用现有共享选择器/队列/SDK，不改概率、RNG或HP。需要实际双候选/注册失败清理回归，再从20900近档验证本战；整场胜利及后续脚本未完成。

汇总objs/night-hunt-20261026-verdict.json；当前入口objs/live147-work-state.json。完整游戏、完整模块、天空/时序/像素一致性均未完成。
