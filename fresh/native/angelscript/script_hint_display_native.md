# giShowHint原生接线与千佛塔重测

## 真实分叉及实现依据

M08/1真实ctrl01/func7001在库存查询修复后，缺少3121的分支到giShowHint(-144)退出。失败保留compare-rebuilt-20261030-190705.err.log。继续使用有效50402近档，不重跑住宿/渡口，不直接调用剧情或修改库存/随机/剧情结果。

归属门5D5D80返回OWNERSHIP_REVIEW_REQUIRED。现场IDA确认真正运行回调读取脚本文字payload+4，空指针使用空串，通过ConvertToUTF8及CEGUI::String编码，随后HintWindow_Show(text,{x,y},2.5f,0,0)。入口注释已同步。

新增NativeScriptHintDisplay借用既有NativeCeguiUiSession HintWindow及游戏计时器。-144适配SDK字符串到局部payload视图，执行共享ScriptHintDisplay::Show；文本转换复用ConvertGameTextToUtf8，编码大小/字符串构造/length/ptr直接调用已安装CEGUIBase.dll导出，未重写库的UTF-8算法。显示复用HintWindow::Show和NativeHintRuntime，避免再次转换已编码文字，不创建额外提示owner。

## 验证

- 最终m08-script-hint-build2构建成功。
- native_script_hint_probe通过真实AngelScript编译/执行-144，读取实际DLL提示窗口文字；覆盖空串、超过32码点的GBK及标记文本、连续替换提示、flag0、取消节点正常延迟退休和2.5秒真实计时器关闭。测试嵌入现有完整smoke，未改变生产窗口流程。
- 完整m08-script-hint-smoke1退出0，GUI12620/12620，DISPLAY1，静音观测525次，无截图。此计数是本次渲染路径的实际与期望对应，不将跨次帧总数差异当成等价证明。
- 从相同50402原版writer档冷加载原版10564/native99252，仅定位到(268,1,550)。共同Space2045874834，结束快照1852046887，m08-script-hint-verdict.json通过：位置/朝向/摄像机各float32位值、control1、48全局、金钱、完整MT、全部物品槽相同。双方3121缺少，实际缺物品提示文字相同；只对提示窗口名称前缀作大小写归一。采样时根已脱离sheet，不宣称该顺序采样证明两端同帧可见/关闭时刻。
- 已直接核对JSON中的UTF-32文字码点，双方均为“熄灭的蜡烛”。此前PowerShell输出经终端编码显示为乱码，不能据此判断游戏原始文案损坏；已纠正文案乱码的旧结论。

## 宝箱后续分叉（修复前记录）

从原始M08/1/GameObjs.gob唯一命名记录及编辑器ParsePal4GobSummary/ParseXndNode字段确认：item001给3081；item101的Machine任务依次OM07、3001×1、3121×1。产物m08-1-items-config.json保留完整配置，解析范围不含其他记录及尾块。

同位定位宝箱附近(845.38,150.093,-623.39)，共同Space193964596，快照587384584后原版3001从27→28并新增3121×1(sequence117/category4)，native均未获得。不是提示回调异常，两端仍存活。只读native item101对象地址667876048，uses+198仍1、running+27C为0、两任务容器ready，尚未触发。没有补发物品绕过。

读取原版实际ActorControl1678465080的words20/21/22均0。native扫描得到与当前player337548096及ActorControl布局一致的候选337559624，其words22仍686312484（前次烛台EVF选择），详见m08-chest-interaction-latches.json；另一候选是外围Services布局，已区分。保留此为输入释放/交互目标清理假设，尚未用正式导出的owner地址及IDA完成归因，不直接修改候选内存。不计宝箱、物品消耗、点燃、开门或高楼层通过。

当前仍可复用pair-m08-script-hint-identities.json检查存活进程，但两端库存已分叉；下一修复后从50402近档重放烛台→宝箱，不能在分叉状态上补数据并宣称验收。

上述进程已正常关闭；后台分发门遗漏GOB/EVF目标已由IDA及正式快照确认并修复。新的双端近档路线已通过烛台→宝箱，实际3121奖励、库存及各项限定数值对应。当前边界转为有物品点燃时Machine类型1尚未组合。最新证据、进程身份与验收范围见[交互目标释放记录](../input/interaction_release_native.md)，不要复用本节旧进程身份。

2026-10-31补充：上述Machine类型1缺口为历史阶段，后续第一处烛台已完成并建立
双writer/双来源冷读与真实重入检查点，见comparison_save_checkpoints_native.md。
加载场景名也改为真实HintWindow，不再使用只记录文字的SceneHintEvents；
实现、flag1/1秒专项、失败夹具修正及实际双来源加载文字证据见
[场景名提示](../ui/arena_ready_hints_native.md)。脚本giShowHint的flag0/2.5秒语义不变。
