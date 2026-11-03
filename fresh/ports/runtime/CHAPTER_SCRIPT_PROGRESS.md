# 第一章脚本服务进度

## 2026-10-29 M01 func2001 服务 180

- 原包探针从 `M01.csb` 执行 `func2001`（story=10300），前置服务依次为 74/103/105/55/250/253；绑定空的 portable owner 后，CSB 越过 180，继续通过受控相机/对白服务到 `giPlayerWalkTo(257)`。剧情值保持 10300。
- 复核发现 `GameServices` 之前只分派 258 `giPlayerRunTo` 和 267 `giPlayerEndMove`，遗漏 257。现已将 257 接入同一个 `player_walk` 回调；ChapterHost 已绑定的 `PlayerWalks::Start` 选择普通走路路径。原 M01 探针确认参数为 player 0、(-353,40,2282)、不冲刺，脚本收到外部等待 token 后暂停。受控 owner 接受请求并返回 suspend，不代表真实地面移动完成。
- `func9001` 的 154 `giGetVisibleObject` 路由到 ChapterHost 当前 `SceneObjects::IsVisible`，按真实对象名读取现有 GOB 的 `+0x184` 对应 visible 字段；不存在返回 false。新增 210 `giGOBReset` 路由复用 retained 场景 XML 和共享 `GobObjectBase::ApplyParameters`，当前仅接受 portable owner 已验证的 Normal(0) 与 GetItem(3)；Action、Sound/Machine、Marker、Effect 类型仍明确报未实现，不伪装为已重置。名称查找未命中时按原虚调行为无操作。
- 主机原 CPK 探针加载 M01/1 并执行原 `func9001`：真实列表没有 `item200`，154 返回 false，之后真实脚本调用 210 一次并从同一 `SceneObjects` owner 查找缺失名称，随后脚本仍在更后续服务处失败。该探针证明原 CSB 能到达并执行 210 miss 路径，不代表 M01 中的对象字段被重载或函数完整执行；没有把查询强制为 true，也没有提交 M01 场景。
- `GameServices` 的 180 路由已接入 ChapterHost 会话持有的 `ArenaSkillState`。它初始化共用 `ArenaSkillManager`，并通过 `ScriptArenaSkillState::HideSkillObject` 调用共用 `ArenaSkillManager::CleanupPlayerData`；关闭时调用 `ClearActiveObjects` 后再 `Destroy`。
- 当前 owner 尚不能创建或接收真实技能 actor/clump/effect。清理前会检查 manager 三组记录：发现 actor 或非空 effect handle 时显式拒绝；保留对象需要 scene clump owner 时也拒绝。实际测试只覆盖空 manager 清理；这不是完整 service 180，也不能清理真实非空技能对象。
- SceneObjects 合同覆盖 Normal/GetItem 配置重载、Normal 使用次数恢复、缺失名称无操作、Action 显式拒绝，以及 AngelScript 210 路由；time-script 合同与 M01 原 CPK probe 验证真实 CSB 的 154→210 分支。主机未运行 ChapterHost 完整场景入口。Android x86_64/GLES ChapterHost 构建尚未复验本轮改动。
- M01 场景仍受非空怪物场景门禁拒绝。本次没有宣称完成 cave entry、完整技能对象清理或剧情推进。

## 2026-10-29 定时回调复核

- 原始定时回调缺失时不能静默成功：IDA 对应的 `PalScriptTimedProcessor` 在解释器缺失时调用 `ShowMissingScriptMessage()`；原运行适配将其作为错误。模块可加载但函数声明不存在时，原 `NativeScriptFrameRunner::Queue()` 因 `FunctionId < 0` 抛出 `PAL4 script function is unavailable`。便携 ChapterHost 现通过 `PushTimeScriptCallback` 对活动重复项去重，对缺失声明显式报错。
- `pal4_portable_time_script_tests` 新增缺失声明不改栈、有效声明可推入、活动重复项去重合同。主机上真实原包探针通过：从 `script.cpk` 读取原 M01 CSB，执行 `M01_1_init` 注册 180 秒 `func9001`，以 1/60 秒推进 10808 帧并执行回调；100 次可见物查询后回调模块卸载、栈深为 0，最后经 217 清除 timer。
- 独立 Android x86_64 CLI 交叉编译后在 `emulator-5558` 对同一原 CPK 探针通过，结果同主机。CLI 只读取模拟器已有的 `script.cpk`，没有安装 APK、启动应用或推进游戏；执行后删除了自己上传的 `/data/local/tmp` 文件。ChapterHost Android x86_64/GLES 编译通过；主机 time-script、script-stack、CSB 三项合同通过。
- 这组探针使用受控的可见物与场景就绪服务，确认真实原 CSB、string opcode、timer、脚本栈与上下文生命周期组合，不构成真实 M01 场景验收。`giArenaLoad(M01, 1)` 仍受非空怪物场景门禁拒绝，本轮未改门禁或部署。

## 本轮目标：M01 giTimeScript

M01 的第一章入口会启动原定时脚本服务。共享 `ReadCsb` 从原包读取 `M01.csb` 后，执行 `M01_1_init` 得到：

```text
call 216 story 10200 f:43340000 s:66756e6339303031
call 67 story 10200
final 10200
```

`43340000` 是 `180.0f`，字符串字节解码为 `func9001`。原包路径为 `H:/PAL4/Games/PAL4_original_steam/gamedata/script.cpk`，SHA-256 `1048EEE567938DD3CA29A5D6279E2E7436BDB347988554EC888F5ADF744AFE06`。这是原 CSB 入口调用记录，不代表本轮实际运行了 180 秒回调。

原服务签名为 `giTimeScript(float, string &in)`（216）和 `giTimeScriptTerminate()`（217），见 `fresh/native/angelscript/game_function_catalog.inc`。原函数证据见 `fresh/include/pal4/script_timed_state.h` 的 `0x5D8610/0x5D8640`，帧处理器 `0x600B90` 累加时间、到期清零并按 ActorControl/脚本执行器空闲门控加载回调。`fresh/native/package/timed_script.md` 记录回调模块名为合成的 `TimeScript[callback]`，字节码来自当前世界 CSB，已有 NativeTimedScriptStack 负责解释器及上下文生命周期。

便携运行时新增 `TimeScript` 单计时器状态，并在 `GameServices` 严格路由 216/217。ChapterHost 持有 timer，将 216/217 绑定到实际 owner，并在每帧脚本步进前推进一次。门控复用 ActorControl `enabled` 状态和当前脚本栈状态：到期先清零，再按 ActorControl 活跃或执行器空闲条件尝试分派。时间精度保持原 double 中间值比较、float 状态回写及丢弃超额时间；保持单活动计时器，不接受隐式空操作。

到期后从当前 Arena world 取得对应 CSB，在合成模块 `TimeScript[callback]` 中查找同名零参函数并嵌套压栈；使用现有 `ScriptStack` 与游戏服务上下文执行，不绕过 Arena 初始化或非空怪物场景门禁。重复活动回调按模块名去重。场景切换提交后释放旧 world 的原始 CSB 字节；返回此前已注册的 world 时按需重读 bytes，不重复编译模块。timer 回调完成、没有活跃模块帧后卸载合成模块。`StopGame` 终止 timer 并释放脚本字节缓存与回调模块。

## 验证

- `pal4_portable_time_script_contract`：MSVC Debug 构建并通过。
- `pal4_portable_script_stack_contract`：MSVC Debug 构建并通过；新增覆盖活动模块查询、活动中拒绝卸载和完成后卸载。
- `pal4_portable_script_module_cache_contract`：覆盖 A→B→A 回场重读、旧 world bytes 释放、读取失败不污染缓存。
- `pal4_portable_csb_contract`：MSVC Debug 构建并通过；新增 string `BC_ALLOC`/`BC_FREE` 正常析构、调用异常清理和 8/16 位有符号/无符号转换及引用读写合同。
- 真实原包组合探针：`pal4_portable_time_script_tests.exe --original-cpk H:/PAL4/Games/PAL4_original_steam/gamedata/script.cpk` 从 M01 CSB 执行 `M01_1_init`，经 `GameServices` 216 注册 180 秒 timer 和 67 就绪路由；按 1/60 秒推进 10808 帧后从原 M01 CSB 装载 `TimeScript[func9001]` 并完整执行。受控服务仅为 `giGetVisibleObject` 返回 true、`giArenaReady` 记录调用；结果为100次查询、剧情值保持10200、timer重复启用，回调合成模块卸载、栈深回到0；经217路由终止后 elapsed/delay/callback 清空。此探针验证 CSB 执行和服务组合，不装载 Arena、不验证真实场景 owner。
- Android x86_64 无资产 Debug APK 构建通过；本轮未部署到模拟器，因此不把既有安装包状态计作本改动的模拟器验证。
- 原 CPK `M01.csb / M01_1_init`：既有原包 CSB 探针观测 216(180.0, `func9001`) 后接 67。
- 没有安装/部署设备，也没有附带用户资源。

## 当前边界

原 `launch.exe` IDA 数据库 `I:/Games/PAL4_game/launch.exe` 中复核 `0x600B90`：计时达到阈值后先清 elapsed；ActorControl 首字非零或脚本执行器空闲时，从 GameArenaManager 当前 CSB 字节创建 `TimeScript[callback]`，检查活动栈重复项后加载并压栈。`ControllerState.enabled` 对应该首字，所选玩家身份不用于门控。

CSB 对象合同依据 AngelScript 2.2 `as_context.cpp` `BC_ALLOC/FREE/WRT1/RD1` 和 `scriptstring.cpp` 默认 string 构造、引用释放；-14 为该运行时 string 默认构造器，17/18/26/34-37 为已核对签名的 string 赋值、追加及连接行为。未知对象类型或构造器仍明确失败。切场时只有不再被任何活动脚本帧引用且不是当前 world 的 world 模块才卸载；仍活动的旧 world 调用帧保留，待其结束后再卸载，后续回场由原包 CSB 字节缓存按需重读并重建模块。栈合同验证活动中拒卸与完成后卸载，缓存合同验证 A→B→A。

受控原包探针实际推进并执行了 `func9001`，但不是完整 ChapterHost 场景验收：`giGetVisibleObject` 固定返回 true，`giArenaReady` 使用记录服务。当前 Q01 至洞口 `func1006` 的设备记录仍请求 `giArenaLoad(M01, 1)`，并被非空怪物场景玩法门禁拒绝；该门禁和怪物/战斗范围未绕过。完整洞穴剧情、真实物件状态分支和自然离峰仍未验收。
