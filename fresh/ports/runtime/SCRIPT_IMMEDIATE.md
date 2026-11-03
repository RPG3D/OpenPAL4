# 原CSB步进与立即模式

游戏使用 `ScriptStack::Tick(..., ScriptRunMode::original_steps)`。原CPK/CSB装载路径保持不变，暂不实现CS到CSB编译。默认 `until_wait` 保留给开发记录及既有源码对照，不作为游戏原调度规则。

## 原代码规则

- 0x5D2FB0调用原ScriptImmediateMode::Begin，保存当前模式到previous，再设current=1；0x7DF440对应这一写入规则。
- 0x5D2FC0调用原End，把previous恢复到current并清previous。只有一个保存位；连续两次Begin后，一次End仍为1，第二次End才回0。
- 0x7DF540根据帧入口模式决定执行一次ExecuteStep(0)，还是重复执行。普通帧中遇到Begin后仍结束当前步；下一帧才重复。End恢复0后结束重复。
- AS2.2的ExecuteStep在原SUSPEND语句标记及系统调用返回处让出，包括字符串工厂。旧CSB的108和97据此推进PC后暂停，避免重复执行上一条回调。

`ScriptImmediateState`复用原Begin/End处理器，持有两项32位值。每个ScriptStack帧独立拥有状态，不存宿主指针到原DWORD字段。55/56在上下文层处理，仍经过OnCall记录，不能交给平台层的空回调。

## 等待与限制

普通步进让出使用 `ScriptPause::step`，不创建等待令牌；真实服务等待继续使用原delay/external令牌。立即模式不解除影片、对白、动作、镜头或计时等待。取消、弹栈和新上下文不继承别的帧的模式；调用者被子入口覆盖时保留自己的模式。

原源码中的立即循环没有本模块的指令预算保护。公共执行器保留预算上限，防止循环锁住渲染线程；预算暂停保留PC、栈和模式。这是明确的宿主保护边界，不宣称无限立即块的逐帧等价。正式运行的默认预算为10000条指令。

`original_steps`仅支持原CSB。2.38源码后端显式拒绝此选项，继续用于独立开发对照；两者不互相回退。尚未支持的指令仍明确失败。

## 验证

主机合成CSB测试覆盖单保存位、普通步进、Begin首帧、立即块跨语句、End、真正计时/外部等待、嵌套上下文、取消、预算和异常优先级。合成夹具仅写测试线格式，不是CS编译器。

`verify_csb_steps.py`直接提取用户原CSB，与32位官方AS2.2的ExecuteStep(0)逐帧比较服务调用、模式、previous、剧情值和状态：

| 原入口 | 记录帧 | 服务调用 |
| --- | --- | --- |
| NewGame | 24 | 8 |
| Q01_Q01_init | 18 | 4 |
| Q01_N01_init | 450 | 165 |

三组均逐行一致。这里的服务是记录夹具，查询133/287/288返回明确的1；等待不执行真实游戏逻辑，不以450帧等同实际影片/对白时长。设备真实等待由verify-media/verify-dialogue另行检查。

可重复命令：

```powershell
python fresh/ports/tools/verify_csb_steps.py H:/PAL4/Games/PAL4_original_steam/gamedata/script.cpk H:/PAL4/objs/pal4-android/host-tests-msvc/Debug/pal4_portable_package_tests.exe H:/PAL4/objs/pal4-android/script-oracle/Release/pal4_script_oracle.exe H:/PAL4/objs/pal4-android/host-tests-msvc/Debug/pal4_portable_csb_tests.exe H:/PAL4/objs/pal4-android/emulator
```

报告 `csb-original-steps-comparison.json`包含原包、输入CSB、两个执行器和逐帧trace的SHA256。原CSB和trace仅存本地诊断输出目录，不打包发布。verify-media会重新运行此检查并核对当前原包来源；设备进展见[安卓状态](../android/STATUS.md)。

设备验证按实际帧边界检查：镜头任务清理帧仍停288，后续首对白状态单独证明脚本恢复；250位置通知在提交快照中计数，253朝向通知在其提交快照中核对名称、阶段2、关闭和模式0。已关闭的位置通知会在跨帧Process时清理，不要求两次通知共存。音效仍要求实际播放样本、原音频来源和自然完成，结束位置与解码时长相差不超过50毫秒；不把EGL恢复期间UI漏采样当作提前结束。
