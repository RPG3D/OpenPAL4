# Q02主线NPC朝向回调分发

## Q03揭榜新增 -278（2026-10-26，live151–152）

live150正常完成寿阳入城40842–40856及客栈前40857–40873，两次actual
Scene/control1、位姿/镜头float32 bits/完整MT624对应。通过正常System保存分别归档
global21800/arena20原版与native writer，各219503字节；native来源已两端冷加载
限定验证，归档objs/checkpoints/q03-inn-global21800-20261026-live150。

live151从该真档共同定位告示附近，正常Space调查MO005，40874/40875后native
退出“callback -278 is not implemented”，原版继续。日志live151-poster-driver1.log、
compare-rebuilt-20261026-210712.err.log；无剧情值/属性/RNG外写。原版正常WM_CLOSE退出0。

先classify 5E4050，再查当前IDA：NPC名称查找、明确Player索引、目标减源、角度计算、
0x38字节849570等待记录、C02/Z02和第三参数wait。现有共享
ScriptPlayerTurn::FaceNpcToPlayer已完整承担这些流程，原生NPC转向服务也已绑定。
仅补NativeGameScriptCallbacks -278分发，透传string/int/bool，未复制转向算法。
IDA注释同步标记真实触发。

SDK脚本新增NpcFacePlayer调用，缺失服务须准确报告“service -278 unavailable”，
避免静默成功或仍落入未实现分支。npc-faceplayer-20261026-build1/test1：
callback与默认应用2/2通过2.08秒。全func2003 direct gi静态dispatch无缺项，
但不代表服务与完整剧情正确。live152改用同归档原版writer冷读，继续原失败点。
尚未验收修复后的完整揭榜流程。

2026-10-26，live130–132。

## 触发与复用边界

用户指定的既有反编译脚本根目录为 I:/Games/original/gamepatch/gamedata/EditData/script。直接读取Q02.cs（GBK），不重复生成已有脚本，不将该文本替换进正在运行的CSB。func2002在global20300触发吃粽子/追鸡/后续战斗。实际EVF只读资源/记录反向引用确认func2002竖直触发面，位置范围x[-752.124,-698.890]、z[-1356.632,-771.902]。

从已双端冷读的Q02真实档开始，授权共同定位到(-725,0.5,-1100)。定位本身没有触发；随后共同输入跨过实际事件面，正常场景事件启动原脚本。live130在旧NPC朝向分发缺口退出；不改global或直接调用剧情函数绕过事件。无需NewGame/迷宫/开场复验，当前脚本锁定区内没有新的正常存档，因此修复后仍使用最近Q02档重进待测事件。

## -286 giNpcSetAng

live130场景事件执行至giNpcSetAng(4012,31)退出，错误“callback -286 is not implemented”；原版继续显示40458。IDA5E6980所有权门禁后核实：NPC manager/name查找，保留X/Z，按vtable+8写入yaw，并建立关闭的20字节849550等待记录。现有ScriptPlayerRotation::SetNpcAngle及NativeScriptPlayerRotationRuntime已经实现，已有NPC绑定；仅补NativeGameScriptCallbacks分发，复用服务、名称与float参数。

SDK回调测试增加真实脚本NpcAngle；缺失rotation服务时必须报告service -286 unavailable，不能走未实现分支或静默成功。npc-angle-20261026-build1.log/test1.log：构建成功、回调与默认应用2/2通过1.82秒。原共享旋转测试文件本来覆盖数值角度、X/Z保留、等待记录、缺失NPC/分配失败；本轮未重新配置或运行该独立测试目标，不能算新增执行证据。

live131正常事件到首句40458，原NPC4012与98个native related actors中唯一同位置对象朝向bits均(0,31,0)，证据live131-npc4012-angle-proof.json与配对图101022。随后共同51次输入之前的对白逐次相同至40489，证明跨过旧-286缺口；不证明整段动作/时序像素等价。

## -277 giNpcFaceToNpc

live131共同step50后到追鸡NPC互相朝向时，native退出“callback -277 is not implemented”，原版现场文本和错误日志保留。IDA5E3D60所有权门禁后核实两个NPC查找、目标减源位置、向量/角度计算、38字节849570等待及C02/Z02动作。现有ScriptPlayerTurn::FaceNpcToNpc已经实现这些分支，NPC turn服务也已用于其他实际交互；只补分发，传两个原名称和wait字节。

SDK测试增加NpcFaceNpc脚本和缺失turn服务必须报告service -277 unavailable的约束。npc-facing-20261026-build2.log/test2.log：两项2/2通过2.20秒。静态检查提供脚本func2002直接gi调用的catalog分发不再缺项（live132-func2002-dispatch-audit.json）；不证明内部脚本、服务和动态语义全部正确。

## 证据与范围

当前动态复验以objs/live132-dumpling-*和live132-work-state.json为准；最终状态另记录在模块.md。所有中间文件在objs，原版注入/native不注入，800×600并排于动态1600×900副屏、静音、不抢焦点。未修改原EXE、HP、时钟、RNG、剧情变量或动画完成标志。当前仍未验收整段剧情/战斗、全部NPC API及完整游戏。

## live132自然复验结果

live132从Q02真实近档，经授权共同定位与W12跨事件面，84次共同Space完成func2002对白40458–40516并进入王魁山2581战斗。补齐-286 giNpcSetAng/-277 giNpcFaceToNpc分发，复用既有旋转/转向服务；SDK回调与应用2/2通过2.20秒。普通攻击、第一次冰咒、止血草后完整MT45与角色数值对应。第二次冰咒后稳定分叉：王魁山双方803精、菱纱双方0精；天河原版279/native465，MP双方41，MT原62/native56。129011/129012两次观察均未汇合，停止输入并保留现场，不能继续将战斗结果视为等价。原版额外6次抽取返回地址均属5808E0事件9117；根因尚未确定。

MT观测前56次值和索引对应，原版后续增加6次9117抽取。ATB导出完整652/647条；原版task marks59条、native marks512条且dropped115，后者不是完整任务证据。保留本现场继续定位，未为制造一致写入HP/RNG/时间/剧情。
