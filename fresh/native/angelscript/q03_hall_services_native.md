# 刘府大厅 func2005：材质、阴影与已有脚本服务

2026-10-26，live157整段自然流程完成，live158/live159冷加载检查。

脚本直接读取 `I:/Games/original/gamepatch/gamedata/EditData/script/Q03.cs`（GBK），不重复反编译已有脚本。原版函数的静态语义以 IDA 为准。func2005 有110次 giTalk，起始40958、末尾41068；包含 XN04Y 和 XN03Y 转场，最终剧情 global22100。静态覆盖后又实际走完本函数；验收范围仍限于所记录的流程和状态。

## 已修复的阻断与模块边界

live156 原版到达40958，重编译版在未实现 -214 退出。随后补齐 -214 giEnableShadow、-256 giPlayerFaceToNpc、-134 gi2DSoundStop、-188 giNpcSetEmotion、-189 giNpcResetEmotion。所有回调均调用已有共享算法：ScriptShadowControl、ScriptPlayerTurn、ScriptSound2d、ScriptNpcMaterial、BaseObjectMaterialTexture、BaseObjectAtomicProperties。缺少服务会明确报错，不静默成功。

新增 NativeScriptNpcMaterialRuntime 负责将当前场景 NPC 和已有材质缓存接入共享算法。NativeNpcMaterialBinding 拥有目录及默认脸/腿字符串，向原始布局借用字段发布数据，退出时还原9个原字段。场景 reset 先恢复默认脸，再撤销绑定；最终 Clear 先释放绑定，再销毁对象和演员。玩家与 NPC 复用提取出的 NativeAtomicMaterialMetadataRuntime，避免重复材质元数据实现。

NativeScriptShadowRuntime 使用已有 PlayerManagerObject::ForAllPlayers 遍历四槽，并调用 NativeActorShadowBinding::Enable。arena 由当前场景 getter 获取，检查世界归属，避免转场后保留旧场景指针。

## IDA 证据

- 5D8590 / 5D85C0：阴影入口与逐玩家回调；检查空玩家，使用 arena+0x10 世界。
- 441430：ForAllPlayers；复用已有四槽循环。
- 5D7830 / 5D7870：NPC 材质 Set / Apply；不存在 NPC 时无操作。
- 5E19F0：按索引玩家面向 NPC；复用已有转向算法。
- 5D5A30：检查并停止当前二维声音句柄。
- 443C60 / 443CB0：默认脸/腿路径由 object+8 目录和材质纹理名构成。

上述函数均先执行库归属分类，再检查 IDA。5D85C0 是内部 loc，语义由精确反汇编确认。

## 验证与限制

`H:/PAL4/objs/liufu-hall-services-20261026-build1.log` 构建通过；test1 四项通过。完整 cave-memory-smoke 正常退出，GUI10866/10866；该探针不覆盖新增材质/阴影断言。

`--player-visibility-smoke` 新增真实资源探针：model101 脸替换101_by、默认脸恢复、九字段还原，以及四玩家槽阴影关闭/打开和原状态恢复。resource2 的失败来自错误比较 SDK 纹理调试名称：PalTextureDict 会重命名为十进制CRC。探针改为比较实际缓存资源句柄，未改生产算法。build2 和 resource3 通过，GUI1488/1488，正常退出。

live157 从真实 global21901 存档加载，双方实际 Scene/control1，位置/方向/镜头 float32 bits 及完整 MT624 相同；正常进入大厅，双方显示40958，已越过 -214 旧阻断。逐句使用相同 Space，仅在双方对白完成或可翻页且文本散列对应后推进；入口首句以成对 GPU 图像和原版文本、native文本散列确认，尚无 native edit predicate 的限制单独记录在 step0。

现场输入/截图/日志保存在 `H:/PAL4/objs/live157-*`，状态入口 `live157-work-state.json`。live157已实际完成两次转场、全部110次对白与最终解锁；112次空格包含长对白翻页。去除连续同一caption的翻页记录后，文本及ID完整对应原始Q03.cs的110句。结束双方actual Scene/control1，位置/方向/镜头float32 bits及完整MT624相同。正常System分别保存证实global22100/arena42；原版220396字节，native220009字节，完整序列化差异未验收。检查点为 H:/PAL4/objs/checkpoints/q03-liufu-hall-global22100-20261026-live157；native来源live158与原版来源live159均已两端冷加载限定通过；字段范围为Scene/control1、位置/方向/镜头bits及完整MT，不证明全部序列化字段或RNG持久化。自然流程验收记录 H:/PAL4/objs/q03-liufu-hall-20261026-verdict.json。动画相位不锁步，不宣称像素或整个游戏完全一致。
