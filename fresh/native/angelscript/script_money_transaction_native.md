# 脚本金钱回调：复用既有事务与背包算法

## 2026-10-30：实际金钱查询接线

归属门对5D7FC0返回OWNERSHIP_REVIEW_REQUIRED；现场IDA确认giGetMoney仅返回库存对象+0x134的DWORD。回调-205复用现有ScriptInventoryQuery::Money，NativeScriptMoneyTransaction借用同一NativeInventorySession提供该地址的受限只读访问；其他地址仍执行GameString记录校验，物品/category查询显式拒绝。各事件/并发上下文继承同一库存服务，不引入第二份金钱状态。

inn-money-build1成功；真实GameDB+AngelScript金钱回归加入giGetMoney，检查返回实际1000且不改钱、不新增提示/声音，inn-money-test1退出0。完整inn-money-smoke1退出0、GUI13252/13252、DISPLAY1静音观测515次。此轮未声称完整物品查询已接通。

原版46532/native95544从50300近档进入CN03，实际掌柜func5005共同Esc后双方Scene/control1、48全局变量/完整MT一致，事件SDK正常完成（pair-inn-money-cancel.json，请求577008502）。位置对应，镜头字段对应；人物yaw原版197.4800567626953/native197.479935，差约0.000122度，尚未逐位对齐。同进程重开并Enter住宿后native越过金钱查询，在下一回调-317显式停止，见compare-rebuilt-20261030-172453.err.log。尚未扣费/推进50400，不能据此声称住宿分支或完整流程通过。

2026-10-27。Q03func2008末尾正常调用giAddMoney(1000,true)。静态覆盖检查发现-178未分发；在自然走完func2007六句并正常保存global30200之后，关闭双方程序，再补齐回调与运行时。

## IDA与复用边界

先分类5D7530/5D7620，再核对当前IDA：Add先调用库存UpdateAndClampCounter，再查GameString C2并格式化；show_hint非零才创建提示；Add调用WA40音效。Pay按32位取负调用计数、使用GameString10B和可选提示，不播放获得音效。ASM5D7549读取完整DWORD金额，旧反编译char ArgList类型错误，已改正函数声明并补充IDA注释。

回调-178/-179直接调用已有ScriptMoneyTransaction::Add/Pay；NativeScriptMoneyTransaction只适配NativeInventorySession::UpdateCounter和既有NativeScriptInventoryChange的GameString、HintWindow、声音端点。没有复制金钱、上下限或提示算法。适配器拥有临时格式记录和文本，数据库字符串仅借用；正常/异常路径都通过共享事务清理文本。它与库存提示服务一起由应用持有，不缓存转场前的arena或库存地址。

共用GameTextFormat增加有类型校验的整数格式入口，复用原有CRT格式化实现；允许原版空串fallback，拒绝错误的变参格式。UI保留原版0.5/0.15/1.0定位与标志。

## 验证

script-money-20261027-build1构建成功。test1四项通过2.20秒：应用默认、真实GameDB+SDK金钱测试、SDK回调与frame runner。真实资源测试通过原始SDK giAddMoney/giPayMoney编译和执行、负数金额、获得/支付提示C2/10B、获得音效、show=false仍播放获得音效、缺提示不阻止金额增加、提示异常后保留先前金额变更且清理文本、库存上下限与32位有符号加法溢出、格式类型拒绝及int32最小值。

完整--cave-memory-smoke通过，GUI11114/11114，正常shutdown及exit0。新回调未绑定服务时返回准确的service -178/-179 unavailable，不静默成功。live173已完整完成Q03func2008的28句41180–41207，双方正常返回寿阳Q03/Q03S并解锁。新增AddMoney实际执行，前后UI金钱3387→4387对应；结束位姿/方向/镜头bits及完整MT624对应。正常UI写出global30201/arena20，两writer各255788字节，三人队伍头字段和柳梦璃等级11对应。教程19在本轮未形成可观察待关闭窗口，10次收尾UI观察均无TutorialWindow，无教程关闭输入，不宣称显示/关闭验收。

检查点 H:/PAL4/objs/checkpoints/q03-liufu-morning-global30200-20261027-live172 保存前厅入口，原版/native两writer均254503字节；冷读交叉检查另记provenance。原版对照继续显式使用DisableOriginalHeapTrim策略，默认分配器失败证据保留，见m04_original_load_corruption_native.md。


live174从native保存来源双端冷启动载入30201，实际Scene/control1和位姿/方向/镜头bits、完整MT624对应。相同F1打开队伍页，三人精/气/神及钱4387对应；同点物品新增页，没药15、丁香15对应；装备页依次切换到柳梦璃，四槽装备及派生数值对应。成对GPU证据live174-role-1500100.png、live174-items-1500200.png、live174-equipment-mengli-1500500.png；这里是实际UI视觉核验，不等同于完整物品/装备序列化逐字段比较。原版保存来源live175也已双端冷加载限定通过，并再核对队伍主数值与金钱4387；相同点击关闭UI后双方actual Scene/control1。可靠新档目录 H:/PAL4/objs/checkpoints/q03-hall-global30201-20261027-live173；验收q03-hall-20261027-verdict.json。
