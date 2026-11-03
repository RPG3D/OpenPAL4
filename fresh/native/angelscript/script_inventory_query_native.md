# 原生脚本库存查询与千佛塔烛台

## 真实分叉及依据

2026-10-30，两端从已验收的50402/M08/1近档继续。定位实际ev_M08_1_13附近(-756.46,1,1113.98)，相同短移动触发func2004，正常推进50600。请求2030892858和产物pair-m08-event2004-verified.json验证位置/朝向/摄像机float32位值、48全局、金钱6526、完整MT及恢复control1；这些只证明当前17ms配对路径。

随后同位定位(268,1,550)，共同Space交互真实ctrl01/func7001（请求429121413），原版正常返回，重建在回调-156退出。compare-rebuilt-20261030-184637.err.log保留首个错误“PAL4 game script callback -156 is not implemented”。没有写剧情、物品或随机结果，仍执行原CSB。

归属门对5D6CF0、467480、4C7310、4C7430返回OWNERSHIP_REVIEW_REQUIRED，随后现场IDA确认：467480只是登记；5D6CF0通过真实库存FindElementInGameDB(id,5)查找条目，缺少条目返回false，否则以有符号数量比较entry+8 >= required。4C7310按GameDB Property类型选择槽；4C7430无效槽仅记录日志并返回空，并不抛异常。IDA入口注释已更新。

## 组合与清理

复用共享ScriptInventoryQuery::HasAtLeast/Money和InventoryItemCollection::FindById，不复制查找/数量判断算法。NativeInventorySession::FindItemById提供实际拥有条目的借用视图，ItemCount也复用它。独立NativeScriptInventoryQuery适配库存读接口，绑定-156及既有-205；不缓存条目地址，不生成或补发物品。

NativeScriptMoneyTransaction只负责增减钱及提示/音效；移除旧的仅Money可用、FindElement必抛异常的查询接口。共享查询与交易分别组合到同一库存owner。原生库存无效槽适配从抛异常改为记录日志，让共享算法按原版返回空值。

实际SDK回归扩展既有native_script_money_transaction测试，检查3121从缺少→1→2→全部消耗、数量不足、零/负required、无效有符号物品ID，以及查询不产生提示、音效或钱变化。保留既有GetMoney/增减钱/失败清理回归。尚未验证点燃、门Machine任务和高楼层，不得将烛台入口当成机关完成。

## 最终验证与下一个分叉

- m08-inventory-query-build2最终构建成功；m08-inventory-query-sdk2实际SDK回归通过。首轮sdk1失败保留，它暴露未知ID的原生无效槽异常，修正日志/空值行为后重测通过。
- 完整m08-inventory-query-smoke1退出0，GUI13252/13252，DISPLAY1、静音观测544次。未产生截图。
- 从同一有效50402原版writer存档冷启动原版72964/native31724，限定启动定位(268,1,550)，未重跑住宿/渡口。pair-m08-candle-before请求1388350017；只读扫描五个实际物品槽，全部(id/category/sequence/count)相同，双方3121缺少，见m08-candle-before-inventory.json。
- 共同Space请求541988099后，native不再报-156，继续至缺少物品分支的giShowHint并以未实现-144退出。compare-rebuilt-20261030-190705.err.log保留证据。此轮不计烛台端到端通过；没有成功的after双端快照，不能将before当作after。下一步接共享ScriptHintDisplay，静态归属门和现场IDA5D5D80已完成，尚未修改该功能。
- 启动前原版遗留hintWindow和native不存在该临时窗口，before观察hints_equal=false；这里未主张所有临时UI生命周期一致，更未主张点燃/开门已验收。

后续-144已接线，缺少3121分支双端限定验证通过；当前新的分叉是随后宝箱未触发，详见[提示回调及宝箱边界](script_hint_display_native.md)。上面的-144退出是修复前历史记录。

## M10 容器数量查询接线

2026-10-31 pair75 的真实容器func7002已触发，但重建在SDK回调-190
`giGetPropertyNumb(int)`退出，证据为
`H:/PAL4/objs/compare-rebuilt-20261031-132418.err.log`。归属门对5D78A0返回
OWNERSHIP_REVIEW_REQUIRED；现场IDA确认该函数以category5查找实际库存条目，
存在则返回entry+8数量，否则返回0。

共享ScriptInventoryQuery::Count及原生inventory_query服务已有实现和装配。
本次仅在NativeGameScriptCallbacks的-190分支委托既有Count并转发SDK返回值，
不复制查找逻辑、不缓存条目、不生成物品。实际AngelScript SDK回归增加存在
条目的数量返回、缺失条目0且不读取空条目、数量变化后重新读取以及缺少服务时
明确异常。这个接线不等于func7002后续事件或整个容器流程已通过；原分叉仍需
主任务重新运行真实CSB验证。

`m10-inventory-count-sdk-build1.log`的Release SDK测试目标构建exit0；
`m10-inventory-count-sdk-test1.log`显示实际SDK回归1/1通过。此次未构建应用、
启动游戏或提交代码。

父任务随后完成应用build及真实资源resource1（exit0/noSKIP/GUI13257对应），
pair76从原版99近档双端冷读后真实func7002通过数量读取、删除99与后续奖励；
42556端点完整观测库存一致。后续42557的相机pitch一ULP分叉单独保留，
不计整段剧情通过。参见../renderware/script_object_timed_scale_native.md。
