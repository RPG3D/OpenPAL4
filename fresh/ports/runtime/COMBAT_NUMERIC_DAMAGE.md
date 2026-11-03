# 战斗数值伤害链

## 原函数与共享算法

- `0x57D520` 读取 stat7、`CombatFormulaConfig` 键5/6/7、攻击与防御元素、RNG `%1000`、伤害/减伤/反射因子及目标半伤标志，返回主伤和次级伤害浮点增量，不写 HP。共享实现：`CombatNormalDamageCalculator::CalculateSkill`。
- `0x5806D0` 执行 9116：检查主伤概率，计算并写目标 stat7；再按次级概率筛选相关目标，累计反射增量并写来源 stat7。共享实现：`CombatDamageActionHandler::Skill`。
- `0x585440` 更新 health state：current HP 不大于0写状态3，否则按最大 HP 的20%/40%阈值写状态2/1/0。共享实现：`CombatActorStatus::UpdateHealthStatus`。

Native owner 用注册的 Player/Monster 属性链、`NativeCombatFormulaCatalog`、SkillSystem RNG 和 `NativeCombatActorSkillRuntime::AddCurrentStat` 连接这些算法。portable 新增的 `CombatNumericDamageAction` 通过 typed `ICombatNumericActorAccess` 连接 actor 属性、HP 写入和 related-target 集合；它使用逻辑句柄，不把宿主指针转成32位地址。公式读取的 `Stat` 走 actor virtual `+48`，当前属性读取的 `CurrentStat` 走 actor virtual `+52`；后者不能用基础 stat7 替代。宿主的 `AddCurrentStat` 必须保留 actor84/property184 的 setter、health state 与属性通知顺序。

portable `CombatFormulaCatalog` 从 `Pal4db.db` 的 typed `CombatFormulaConfig` 行读取公式，并复用共享 `GameDataDatabaseHeader::LoadCombatFormulas` 的32-key校验和范围告警。`RandomDraws()` 返回 damage action 实例创建以来的累计 RNG 次数；单次调用的消耗量用调用前后的差计算。

## 数值合同

`pal4_portable_combat_numeric_damage_tests` 无参运行显式受控 Pal4db/actor fixture：验证主伤与反射 HP 写入、self-target，以及 related list 中 source、target 和重复目标的顺序；还验证目标由 critical state2 变为非正 HP state3。该模式随后报告跳过原包公式分支。

带参数时从真实 `Pal4db.db` 读公式；所有 actor、action 和 RNG 数值都必须显式输入。PC 与 Android x86_64 的原包运行输出一致：公式 `[5..7]=1.6/0.9/1.1`，单次3个随机数，目标/来源 stat7 增量 `-6/-2`，HP 从 `100→98` 和 `20→14`，health state 为0和2。

PC 命令：

```text
pal4_portable_combat_numeric_damage_tests.exe --original-gamedata I:/Games/original/gamedata --seed 17 --target-selector 0 --source-hp 100 --source-max-hp 100 --source-stat7 4 --source-element 0 --source-damage-factor 1 --target-hp 20 --target-max-hp 100 --target-stat7 3 --target-element 0 --target-reduction 0.1 --target-secondary-factor 0.2 --target-half 1 --element 4 --skill-multiplier 2 --primary-chance 1 --secondary-chance 0 --splash-multiplier 1
```

Android 命令将可执行文件路径替换为 `/data/local/tmp/pal4-numeric-20261030`，`--original-gamedata` 使用 `/sdcard/Android/data/org.pal4.port/files/PAL4/gamedata`，其余参数相同。输出只报告公式、RNG次数、stat7增量、HP及health state，不报告胜负。

这些数值 operand 是显式合同输入，不是原场景 actor roster。Android 尚未接入真实 Combat actor/property owners、9116 Skill record owner 和生产 AddCurrentStat adapter；自然输入、AI、回合流程及战斗结果仍未接入。该合同不代表完整战斗可玩。
