# M03角色朝向另一个角色的脚本分发

2026-10-26。live141在M03/1Y同40675后，giPlayerFaceToPlayer(0,1,true)触发native callback -254 is not implemented并退出。原版正常继续40676，现场UI和输入保留，确认响应后正常WM_CLOSE。白天转夜景原EVF缺口已在同轮自然通过。

归属门禁5E1730/5DA0E0后核对当前IDA Player_FaceToPlayer5E1730：按两个索引查询Player，目标减源位置，既有向量处理/角度计算，原38字节849570转向等待记录、等待参数、C02/Z02与武器特效清理。原登记签名在script_dialog_late_specs.inc中已存在，SDK也已注册。

现有ScriptPlayerTurn::FacePlayer及NativeScriptPlayerTurnRuntime已经完整提供此服务，原本也有共享测试覆盖向量/调用顺序、缺源/缺目标。只补NativeGameScriptCallbacks的-254分发，传原两个索引和wait字节，不另写角度或任务算法，不改脚本源码/CSB。SDK回调测试新增真实脚本PlayerFacePlayer，未绑定服务时必须进入service -254 unavailable而不是未实现/静默成功。

player-facing-20261026-build1成功；SDK回调与应用test1共2/2通过2.06秒。共享转向目标重新构建，shared-test1 1/1通过0.04秒（CTest总0.22秒）。所有日志在H:/PAL4/objs。修复后自然40675分支待live142从global20800近档复验；不得将上述有界回归称作整段夜景或所有角色API验收。

live142已自然通过旧40675转向分支至40676并共同推进40685（截图live142-lake-step61-456110.png及完整journal）。之后未实现-182表情导致退出，是新缺口；不撤销-254局部自然通过，不称整个睡眠流程通过。
