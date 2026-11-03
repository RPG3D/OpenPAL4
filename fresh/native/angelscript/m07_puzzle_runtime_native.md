# M07前厅门侧机关：脚本适配器已补，实际窗口/state8仍待接

2026-10-27 live255。从H:/PAL4/objs/checkpoints/m07-ghost-global40900-20261027-live252/original的真实40900档双端冷读。采用ScriptedKeyboard/MatchOriginalRandomSeed/TraceMtRandom，严格Scene/control1、全部位姿/镜头float32 bits/fullMT624通过；证据H:/PAL4/objs/live255-ghost-cold-verdict.json。native来源253同样限定通过。254启动未共seed，原20547751/native20550734，排除该轮严格验收；两端正常关闭，不修改实时RNG或存档绕过。

## 真实脚本与触发证据

直接读I:/Games/original/gamepatch/gamedata/EditData/script/M07.cs（GBK），运行仍原始CSB。func1001只有giGetVisibleObject("entrance2")为真才换M07/2；不是到达出口就必定换图。门侧func7001先锁角色/停追击/教程12，然后giStartPuzzleGame(0)。结果大于零才做huaRongDoor01_key动作B、原相机viewDoor01、giGOMTouch("huaRongDoor01")并显示entrance2。不能强改门可见/结果或故事变量跳过机关。

只读EVF证据H:/PAL4/objs/live250-m07-evf.json：func1001平面z约-1715.3；func7001实体八点x79.8..101.8、y16.9..47.3、z-1665.7..-1644.7。255在关闭北门前同定位后16次共同W/两次S未触发换场且X有差，保留输入，不计移动精度验收。随后System中授权同位定位(85,.897199988,-1620)，共同Space后native明确因callback -300未实现退出。原版稍后只读Scene/control1；未在其短时机关窗口期间取证，不能宣称双端机关界面对应。证据compare-rebuilt-20261027-171817.err.log、live255-door-key-input.json、live255-original-after-puzzle.json、live255-work-state.json（均在H:/PAL4/objs）。原版正常WM_CLOSE结束；无driver/pendingGPU。

## 当前IDA确认及复用边界

归属分类5E6F30为OWNERSHIP_REVIEW_REQUIRED，当前IDA实际反编译确认为PAL4游戏入口PuzzleGame_Start。它清UIFrameManager+0x222，构造临时XML Mode0/Type参数，进入state8，分配0x20字节模式1等待84960C入executor+0x54队列，然后销毁XML。复用fresh/src/script_puzzle_game.cpp的ScriptPuzzleGame；不调用原EXE地址。

当前IDA4A0BA0的构造体（查询4A0CD0返回同函数）调用UiPropertyGrid_Construct初始化frame+0x2C，并将真实layout的StaticBoard/StaticStep/StaticText和BtnClose接到共享输入/绘制。已有fresh/src/ui_property_grid.cpp包含首个4×6布局及后续布局、选择/移动/摆放/渲染逻辑；不是必须重写的库。原有名字UiPropertyGrid虽然泛化，明确包含机关棋盘，不能因搜索Puzzle类未命中而复制算法。

复用链：ScriptPuzzleGame → 原生NativeScriptPuzzleRuntime（已独立实现，实际UI端点待接）→ PuzzleUiState/state8 → 原生CEGUI窗口owner（待接）→ PuzzleWindow/UiPropertyGrid/PuzzleWindowInput/Counter/Result。原生owner借用已有CEGUI、NativeUiFrameUpdate、NativeGameTimerQueue，按生命周期先销毁窗口与回调再销毁借用服务。状态8退出应通过真实关闭回Scene，等待按IDA84960C真实条件结束；不能用教学窗口等待或强制成功代替。-131结果应复用ScriptPuzzleResult读取实际同一UI结果字段。原生脚本适配器已在下文补齐；真实窗口/state8/聚合等待器和应用上下文服务仍待绑定，不能算实际机关已完成。

## 验证

共享五项ScriptPuzzleGame/PuzzleUiState/UiPropertyGrid/PuzzleWindowInput/PuzzleWindowCounter的Release目标实际build1成功，focused2为5/5通过(.20秒)，日志H:/PAL4/objs/m07-puzzle-shared-20261027-{build1,focused2}.log。首次focused1因为这些独立EXE尚未生成而Not Run，随后实际编译再通过；不是native机关已可运行的证据。下一从40900近档直接同位同Space验证开局棋盘/同操作/计时/失败关闭/成功门动作，不重跑已通过23句。


## 原生脚本层实现（2026-10-27）

新增fresh/include/pal4/native_script_puzzle_runtime.h与native/angelscript/native_script_puzzle_runtime.cpp，借用实际UI manager、结果clear/read、state8入口和当前state查询；每个SDK上下文独立拥有32字节等待与临时Mode/Type文档投影。原共享ScriptPuzzleGame负责原始调用顺序与异常清理，适配器只桥接服务/SDK Suspend，不复制开局或棋盘算法。-300/-301/-131已接NativeGameScriptCallbacks，缺服务仍明确失败；尚未给实际应用伪造窗口或结果端点。

84960C的12字节当前IDA内容为5E48D0/5E6EF0/5E8CE0，机关等待与交易共用5E6EF0。提取ScriptCurrentStateWait::Complete并使既有IScriptTradeWaitRuntime通过兼容方法接入，保留原先两次current-state读取与null短路。Puzzle Tick复用同一判定；仅真实Scene1/null恢复，result=1且state8不会恢复。当前IDA5E6EF0更名ScriptWait_CurrentStateUntilScene并更正注释，5D5890更名giGetPuzzleGameResult_ReadUiResult。归属审查两者均PAL4域胶水，CEGUI/CRT继续用既有库。

真实AngelScript SDK编译原giStartPuzzleGame/giStartJigsawGame/giGetPuzzleGameResult签名，验证Mode0/Type0、Mode1/Type3、旧结果清零、同上下文暂停/恢复、非Scene各状态保留等待、成功字节不提前结束、null不做vtable查询、独立上下文、foreign owner在清结果前拒绝、取消不关闭全局UI、异常文档回收/重试。puzzle/trade SDK及共享trade边界focused2为3/3通过(.18秒)，application-build1成功。证据H:/PAL4/objs/m07-puzzle-sdk-20261027-{build1,focused2,application-build1}.log。

资源gate首次resource1只给config未带情节模式，在既有M01 initializer门禁失败；不把它隐去或归因新机关代码。resource2显式--cave-story-smoke通过GUI2150/2150正常shutdown，但覆盖小于此前完整cave-memory gate。resource3用--cave-memory-smoke按同覆盖范围复验通过，GUI13239/13239且正常shutdown（H:/PAL4/objs/m07-puzzle-sdk-20261027-resource3.log）。当前没有40900双端重跑：真实机关UI/state8未绑定，重跑尚不能提供修复证据。


后续接线位置已查明：fresh/native/package/native_arena_load_runtime.cpp的LeaveState/EnterState、Scene往返分支（当前包括2/3/4/5/6/9/10/12，尚无8）、ReturnBink/系统/ModalFrame返回场景门禁；fresh/tests/win32_application_runtime_tests.cpp注册state8 owner并构建UI/backend、主/ContextExecution/Timed服务与OpeningWait、同input/render/update/retire。不能只在脚本回调里显示一个布局而跳过真实state8生命周期。现资源门禁已终止，无游戏窗口。


2026-10-27补充：原生棋盘后端NativePuzzleBoard已完成并通过实际18布局/原图/原font/DLL String/绘制队列/占用格/循环切换资源验证；完整resource3 GUI13047/13047正常shutdown。共享normal图错误、字体原文件名及启动atlas重复创建已按证据修复。外层PuzzleWindow/state8/计时结果/输入分发/聚合等待仍未绑定，40900双端尚未重跑。见fresh/native/ui/puzzle_board_native.md，不把后端通过混称开门通过。


2026-10-27窗口组件进展：NativePuzzleWindow的真实XML/DLL绘制、global Enter/Esc、自然倒计时失败、结果动画/四秒关闭及输入清理已通过；build6/resource5 GUI12609/12609正常shutdown。Logo旧空动画限制改为共享UI渲染，动画quad同帧提交/清理。该组件fixture的Scene-return端点是计数验证，实际state8/Scene往返及主/事件/定时SDK/OpeningWait仍未接，因此M07 func7001成功开门和双端配对仍未通过。见fresh/native/ui/puzzle_window_native.md。


2026-10-27状态接入已更新：真实Minigame8/Scene往返和各SDK上下文/OpeningWait接入完成，独立实际SDK在Esc与自然超时后同上下文续执行通过。build5/resource2完整--cave-memory-smoke GUI12612/12612正常shutdown；原组件计数fixture保留为组件覆盖。M07真实事件双端及成功开门仍未通过。当前进展以fresh/native/package/native_puzzle_state.md为准，前文未接入描述为当时边界。
