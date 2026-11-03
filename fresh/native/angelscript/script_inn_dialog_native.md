# 原生住宿选择与脚本等待（2026-10-30）

## 实际前沿

原版86296/native96532从50300继续，瞬移至Q04C入口(-1967,85.7,420)，共同W0.08正常func1005进入CN03。场景位置、48全局变量和完整MT一致。原版掌柜4087位置(-141.930191,13.724,-274.765625)；共同瞬移玩家至(-141.93,1.2,-235)并SPACE，原版显示SettleSelectWindow，native在回调-197显式失败。没有跳过住宿、修改剧情/金钱/结果或写入RNG。

证据位于 `H:/PAL4/objs`：`pair-q04-inn-baseline.json`、`pair-q04-inn-entry1.json`、`pair-q04-innkeeper-original.json`、`pair-q04-inn-original-ui.txt`、原启动164535错误日志。双方旧进程已正常关闭/退出。最近可复用50300档为 `checkpoints/q04c-reunited-global50300-20261030-machine-activity`，无需重跑80句剧情。

## 归属与实现

库归属门对5D7B10/5D7C60、47D3D0/47DAB0/47DB80/47DBB0/47DBD0/47DBF0及4BB300返回OWNERSHIP_REVIEW_REQUIRED。Live IDA确认：住宿回调发送show_settle_select事件{1,bool低字节}，构造SettleSelectWindow的frame XML后进入状态10；结果读取frame+70，住宿1、委托2、离开/Esc0。旧IDA加载画面误名已改为Script_ShowInnDialog/Script_GetInnDialogResult。

`NativeScriptInnRuntime`复用ScriptInnDialog/ScriptFrameStateDocument及实际arena XML，绑定实际SDK上下文，进入状态10后挂起，仅在Scene返回后解除等待。主、事件、并发和timed上下文分别拥有适配器，统一opening wait聚合其阻塞/取消。结果来自真实窗口字段。

`NativeInnSelectorWindow`复用TimerFrame构造、按钮/GameString、折叠/展开布局及选择回调；CEGUI DLL负责原始SettleSelectWindow.xml、文本转换、窗口、订阅、父子关系和模态按键。导航复用已有NativeShopSelectorInput，通过INativeSelectorNavigationLayout提供各自的受限图像大小与控件所有权；商店行为保持原接口语义。

IDA47DB40/4BB300未向全局activity插入住宿成员；原版使用CEGUI根窗口KeyDown与模态输入。首版错误地仿照商店插入activity，该项已移除，回归转而检查真实modal、Esc/Enter及鼠标选择，不能以数组预期错误声称原版缺陷。

## 验证状态

- 共享ScriptInnDialog及TimerFrame构造/按钮/布局/可见性/动作6/6通过。
- build1因Windows FindWindow宏污染接口未编译，修正包含顺序后build2/3完成。
- smoke1/2均在新增Inn回归的活动数组预期处失败，不计通过；smoke2确认collapsed导航目标数为2而活动数组为空。
- build4包含按IDA修订的模态输入接线。smoke3中的真实Inn回归通过，最后仅因旧shutdown固定4帧计数未包含新增8帧而失败；保留该失败日志，不计完整smoke通过。
- build5更新并独立核对Inn8帧与总Modal12帧计数；完整smoke4退出0、GUI12812/12812、DISPLAY1静音观测504次。实际Esc0、Enter住宿1、鼠标委托2、collapsed2/expanded3导航、冻结场景/ActorControl、实际状态10等待及清理均通过。
- 实际50300住宿扣费/夜景剧情的双端重测尚未验收。

## 实际双端复测与下一边界

2026-10-30 17:13:08 启动的原版95376/native88704从50300近档进入CN03，实际func5005成功显示住宿窗口。`pair-inn-native-modal-entry.json`（请求2014743886）确认双方场景暂停、位置、48全局变量及完整MT一致；native在真实脚本行7493挂起。共同Esc（请求513450582）关闭窗口后，native恢复脚本并在下一回调-205 `giGetMoney()`显式失败，见`compare-rebuilt-20261030-171308.err.log`。因此取消分支恢复控制尚未验收，也没有完成住宿扣费或夜景剧情。

上述是修复前保留的失败证据。后续已接通共享`ScriptInventoryQuery::Money`，inn-money-build1与完整smoke1通过。原版46532/native95544从50300近档实际重测：共同Esc后双方Scene/control1、事件结束、48全局变量及完整MT一致（pair-inn-money-cancel.json，请求577008502）。朝向存在约0.000122度残差，尚非逐位全等；详见script_money_transaction_native.md。

同进程重新SPACE并Enter住宿时曾在下一回调-317 giShowCommonDialogInSelectMode显式失败；此为修复前边界。后续已复用现有CommonDialog缓存与共享ScriptDialogs/ScriptCommonDialogSelection，并修复实际CEGUI UserData标签接线。

最终双端正常支付300（6826→6526）、CN03Y五句42048–42052及分页对应，正常到50400/Q04CY/Scene/control1。结束位置/方向/摄像机float32 bits、48全局及完整MT一致，两个正常writer均已归档。详见[CommonDialog脚本接线和验收边界](script_common_dialog_native.md)。冷恢复、默认实时及后续夜间渡口流程另验，不代表全游戏完成。
