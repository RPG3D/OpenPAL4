# Q03 庭院 func2010 实际对比（2026-10-27）

直接读取用户指定 `I:/Games/original/gamepatch/gamedata/EditData/script/Q03.cs`（GBK），运行保留原始CSB。live227从正常native31400近档冷读到原版注入端和不注入重编译端，位置/方向/全部镜头float32与完整MT对应，Scene/control1。

原版只读EVF扫描确认Q03X有两个func2010触发面，重新读取顶点验证位置。授权同位定位到(600,-0.3,-1000)后，共同S一步双方自然触发脚本。此前W16步方向背离触发面，两端control1保留输入记录，不算剧情或移动一致性通过。未直接调用脚本/写故事/HP/RNG/时钟/完成标志。

func2010共89次对话调用（编号41450–41537，附加编号47007按源序核对），双方文字hash/原编号匹配，各自自然打字/翻页边界后共同Space。整段89次完成，终点自然Scene/control1；位置/方向/眼点/目标/角度/实际及请求镜头距离全部float32 bits对应，MT624/index对应。完整逐帧/动画/时序/像素未验收。

通过正常ESC/F7/Space/Enter菜单，两writer均保存31500、header37、326968字节、当前仅云天河；归档H:/PAL4/objs/checkpoints/q03-garden-global31500-20261027-live227。新档冷读待验。两端正常WM_CLOSE，未强杀。近档可跳过已验89句。

沿用System呈现状态诊断build1和完整resource1通过（GUI12767/12767）。live224/225呈现失败没有在226/227出现，仍开放；不能称已修复。专题system_present_failure_native.md。现场验收H:/PAL4/objs/live227-garden-story-verdict.json、live227-work-state.json。全游戏/十二模块未完成。
