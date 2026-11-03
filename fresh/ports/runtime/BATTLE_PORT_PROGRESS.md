# 战斗移植进度

## 战斗角色生命状态

`pal4::CombatHealthState` 位于公共头文件 `fresh/include/pal4/combat_health_state.h`，是原生重建与 portable runtime 共用的唯一生命状态规则。状态值沿用原逻辑：当前生命值不大于 0 时为 3；正生命值比例不大于 20% 时为 2，不大于 40% 时为 1，其余为 0。比例计算保留先转 `float` 再除以最大生命值的原始行为，包括最大值为零或负数时的结果。

原生 `CombatActorStatus::UpdateHealthStatus` 仍负责读取角色当前/最大生命值、写回角色状态，并保留非正生命值时不查询最大值的调用顺序；状态分类统一调用共享规则。portable 的 `portable::BattleHealthState` 仅作为该规则的轻量接口，不再维护第二份判断实现。

验证目标 `pal4_portable_battle_state_tests` 同时调用 portable 接口与原生状态更新入口，对各阈值及零、负最大生命值逐项比较，并检查原生查询与写回顺序。

## 当前范围

本次完成的是战斗角色生命状态分类及其原生/portable 共享接入。它不负责保存战斗数据、执行伤害、选择行动或回合、运行敌方 AI、结算奖励，也没有把洞窟遭遇接成完整战斗流程。上述部分仍需在现有战斗系统状态和运行时服务具备明确契约后继续接入。
