# M08/3 千佛塔四、五层烛台

## 当前验收

真实EVF ctrlA_1/func7007、ctrlA_2/func7008、ctrlB_1/func7010已正常共同Space通过，
消耗三份真实战斗掉落3121。第一组同时点亮后实际嵌套func7009，完成清理其他脚本
缓存、镜头、door01及wall01动作并恢复控制。上层第一处已亮；ctrlB_2/func7011
及其嵌套func7012/door02随后在pair27通过，详见下方最新验证。

范围来自用户指定GBK M08.cs和实际原CSB行为；EVF、GOB资源由原包只读提取。
不是直接调用剧情函数，也没有修改可见性、库存、HP、RNG或机关完成标志。
正常Save状态退出会按原算法恢复HP/MP，该接线修复见
[存档退出](../package/save_ui_exit_recovery_native.md)。

## 资源与路线

EVF名称含ctrlA_1/A_2/B_1/B_2，旧仅匹配ctrl加数字的临时提取器会漏掉它们。
精确资源见H:/PAL4/objs/m08-3-routes.json，四个触发体下底面中心约为：

| 触发体 | 原函数 | 中心x/y/z | 控制对象 |
| --- | --- | --- | --- |
| ctrlA_1 | 7007 | 36.7063 / 2.05384 / 985.9041 | candlelight02_ctrl |
| ctrlA_2 | 7008 | -713.4951 / 2.05390 / 590.3790 | candlelight03_ctrl |
| ctrlB_1 | 7010 | 36.9322 / 243.68398 / 985.1504 | candlelight04_ctrl |
| ctrlB_2 | 7011 | -713.3167 / 243.68405 / 590.2384 | candlelight06_ctrl |

本次只定位到中心上方0.01并正常Space，每次先检查实际Scene/party/全局/钱/完整MT。
不是行走/完整平台运动验证。已解析本场景所有item记录，无3121奖励；详见
m08-3-items-config.json。已验出口1005由M08/2进入本场景；1006正常返回M08/2，
随后1004回一层save2。func1007往下一场景必须等第二组开门；pair27在完成后实际进入
M08/4，但入口相机实际距离/eye的严格位值检查失败，不能算入口全字段一致。

## 数值与检查点

22逐事件终点1207026201、1743368894、1824216707，完整背包数量3→2→1→0，
四槽party、48全局、钱及完整MT对应；七组位姿/镜头float32位值逐项对应。
六GOB已查类型、位置、旋转、可见、次数、Machine任务ready/count/running和根LTM
均对应；三火亮、第四火暗，door01 uses0/door02 uses1，wall01 inactive/wall02 active。
这些是稳定终点字段，未逐帧检查alpha、所有运动、声音或画面。

正常两个writer归档：
H:/PAL4/objs/checkpoints/m08-floor3-three-candles-global50600-20261031。
两份均477868字节、global50600，实际存档位置为一层save2；缓存保留本场景三处火。
native来源23重入1928846326、original来源24重入951659805，均由两游戏冷读、
经真实出口重入本场景，确认上述火/门/墙持久化与所查位姿镜头/party/全局/钱/MT。
冷读阶段完整库存和提示文字也对应。整档hash不同，完整字段及RNG持久化未证明。

上述三火档是历史分叉点，已被下方四火档接替。随机开场导致的ATB条件差异独立
保留，20b/21b均在首指令前停止，未用容差或时钟/RNG写入掩盖差异。

## 四火完成与最新检查点（pair27–29）

25/26/27是依次正常存档、冷读后新增的三场2522战斗，未重放已验战斗。
终点942809046/182711270/1308162329分别金8221/8456/8665、MT106/111/157；
各稳定指令的actor数值/技能、持久party、48全局、钱、完整MT和终点七组float32
对应，完整背包对应。前两场未掉燧石，第三场实际掉两份，含梦璃倒下/还魂香复活/
补满HP的正常路径。汇总H:/PAL4/objs/m08-farm25-27-verdict.json；不据此清除CRT/
ATB时序的既有失败。未写HP、库存、剧情或RNG。

正常Space触发ctrlB_2/7011，实际消耗3121一份，嵌套7012完成viewDoor04、door02、
wall02及相机回寻和解锁。终点589546071：四火全亮，两门uses0且无运行任务，
wall01/02均inactive；所查GOB字段/根LTM、七组位姿镜头float32、party、48全局、
钱、完整背包与完整MT对应。证据m08-floor3-last-candle27-verdict.json及journal。

随后真实1007进入M08/4，2116579046及1749498376均因eye/currentDistance严格位值
不同而失败，保留为独立相机分叉，未继续该层机关；见
[M08相机残差](../renderware/m08_camera_collision_residual_native.md)。通过真实1008、
1006、1004退回一层save2，再分别正常保存两个writer。

最新可靠近档：H:/PAL4/objs/checkpoints/m08-floor3-four-candles-global50600-20261031。
两份均483065字节，实际位于一层save2，缓存保留二层两火和三层四火，燧石剩1份。
native来源28重入1126222638、original来源29重入72168736，均由两游戏冷读、经真实
出口进入M08/3；上述火/门/墙及所查位姿镜头/party/全局/钱/MT对应，冷读完整背包与
场景名文字也对应。provenance记录两来源hash和具体检查范围，整档全字段/RNG持久化
未证明。两进程已正常关闭，共享slot0保留此original来源；后续从此档直接接近1007，
不再取材、重打三场战斗或重放四个机关。
