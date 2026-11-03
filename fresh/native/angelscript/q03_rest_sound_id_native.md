# 客房休息：按 ID 停止音效（2026-10-27）

直接读取用户指定的 GBK `I:/Games/original/gamepatch/gamedata/EditData/script/Q03.cs`，运行时仍使用原 CSB。live233 从31500近档共同进入XN03，在授权同床边位置后共同一次Space，双方自然进入func2013。41538–41543六句的编号、文字hash及自然打字/翻页边界对应，均有配对GPU截图。交互距离修复已实际越过原先首句阻塞。

第六句后native退出，错误明确为未实现脚本回调-135。Q03.cs此处在起床动作、镜头002之后调用`gi2DSoundStopID(local_3)`，停止此前负mode播放的WJ024。不是采集超时或已完成休息。

先运行所有权分类0x5D5A60（要求人工复核），再核对IDA：非负ID调用ScriptExecutor的sound-tree删除，负ID原样返回。此为PAL4脚本包装，实际音频资源管理委托已有Miles模块。共享`ScriptSound2d::StopId`及`NativeMilesSound2dRuntime::RemoveSoundId`已经实现；仅补-135分发，保留有符号ID，不增加替代音频实现。

真实AngelScript SDK回调测试新增StopID入口及缺服务异常检查，通过1/1（0.14秒）；共享StopId已有负最小值/零/正值及删除转发分支测试。应用build1及完整resource1通过，GUI12767/12767正常shutdown。证据：H:/PAL4/objs/live233-rest-dialogue-journal.json、live233-rest-run.log、compare-rebuilt-20261027-130617.err.log。

live233已正常关闭原版，native此前自行退出，没有继续输入。未取得40100存档。床边碰撞滑动差、System呈现间歇错误、完整时序/像素/存档/RNG持久化仍开放。

## live234 实际复验

九句41538–41546与源序、原版编号及文字hash逐句对应，按双方实际DLL打字/翻页边界共同Space，逐句配对GPU。原-135退出已修复。最终原版Scene/control1，native持续control0、wait17（combined+action），fade phase1/active1、elapsed=duration=1.5；60次自然观察没有解锁，driver停止且没有第十次Space。双方只读NPC均隐藏、C10/completed0；native动画时间停在.817秒。原版脚本executor+54已经无当前记录/无等待对象。仅能判定终点等待调度差，尚不能把根因断言为隐藏NPC、立即块或淡入模块之一。IDA5E3540与5E04F0本轮核对仍支持原有EndNpc/UpdateEnd实现；不改其已证实语义冒充完成。

当时双方40100近档尚未取得。live234已在live235前正常关闭，两端均无driver。证据H:/PAL4/objs/live234-rest-story-verdict.json和live234-work-state.json。

后续live235实际队列采样已定位已关闭位置/朝向记录的顺序差，修复与live236完整休息/40100保存、237/238两来源冷读验收见[q03_rest_completed_handlers_native.md](q03_rest_completed_handlers_native.md)。上文234未完成仅为历史证据，不是当前状态。
