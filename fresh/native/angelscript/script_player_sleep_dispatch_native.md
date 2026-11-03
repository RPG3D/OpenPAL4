# M03睡眠表情、角色碰撞与技能开关

2026-10-26。live142实际通过-254转向至40685后，native在giPlayerSetEmotion(0,101_by)报-182未实现退出，原版响应并正常关闭。直接读取用户M03.cs（GBK），func2002共45种直接gi调用的注册/分发审计另查出后续-72和-196；旧审计保留在objs/live142-m03-dispatch-audit.json。修复后静态缺项为零，但静态覆盖不证明服务、参数或完整运行正确。

## 实现与复用

所有权门禁后当前IDA5D7720从PlayerManager按索引取玩家，调用443D00，以object+8资源目录拼表情名、取FACE550材质，通过原纹理字典/RpMaterialSetTexture替换。新增IScriptPlayerMaterialRuntime与借用的NativeScriptPlayerMaterialRuntime；服务复用PlayerManagerObject::GetPlayerByIndex、BaseObjectMaterialTexture::SetFace和现有NativePlayerMaterialRuntime CPK/SDK缓存，未另写资源解析/缓存/材质算法。应用在创建角色材质缓存后创建服务，退休缓存前解除借用。

5D3A80复用现有ScriptPlayerActions::AttachCollision，原生Actions边界增加受所有权验证的actor20/496/2832字段访问、实际movement owner的Clump登记/释放，保留原参数1/1/0/0与flag708。两个adapter slot只是共享边界路由标识，不构造VC6管理器。重复附加使用现有登记，不产生重复borrower；退出由同一movement owner注销。

5D7AF0复用ScriptSystemUi::SetArenaSkillEnabled与既有UIFrameManager服务。三回调只补分发及真实服务边界，不以固定成功、改脚本或写剧情推进绕过。

## 资源回归暴露的初始目录缺失

resource-test1在编译Sleep回调失败，SDK/默认应用通过；test2细化异常定位到PAL4 material resource texture path。NativePlayerActorAssets初次Bind只发布name/model/Clump，未发布object+8；目录原只在reload activity的LoadClump设置，同模型无需重载时没有路径。

归属门禁665CC0与当前IDA证实初次GameResourceManager_LoadClump会分配/复制resource directory到object+8。现目录由NativePlayerActorAssets与其模型/name同寿命持有，初次/重载均发布，Clear/Unload仅清理自己发布的指针。删除reload activity重复的directory所有权，保留其face/leg字符串保存与恢复。不在表情回调中猜路径或绕过对象字段。

## 执行证据及范围

SDK真实脚本分别要求缺服务时报告-182/-72/-196 unavailable。原资源player_visibility probe额外运行编译的Sleep/Awake/Attach：真实101_by替换FACE，重复Sleep复用texture owner，101_zc切回；真实Clump附加flag708/borrower增加1，重复不增加，Detach后flag及borrower恢复；原FACE由SDK引用保留并还原。另有原可见性/场景转换及退出检查。

- player-sleep-20261026-build1成功，test1原资源失败；不是资源通过。
- build2/test2仅补异常细节，原资源仍失败，不算修复通过。
- build3成功，test3默认应用、原资源玩家、SDK回调3/3通过54.15秒；原资源50.54秒。
- 相关native player_model_reload_runtime重新构建，1/1通过0.04秒，日志reload-build/reload-test。

以上日志均在H:/PAL4/objs；自然睡眠流程待live143从真实20800档+同湖边事件前定位重放。整M05/M06/M07/M11及全游戏未验收。

live143已实际完成睡眠到20900解锁，闭眼/睁眼材质键、菱纱睡眠可见/碰撞与原版对应；真实夜间save双方writer已归档，live144双端冷读native来源限定字段通过。完整功能与边界见../input/comparison_m03_lakeside_native.md；所有角色API、全部字段/模块/全游戏未验收。
