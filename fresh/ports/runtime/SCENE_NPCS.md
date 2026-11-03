# 场景NPC与脚本显隐

`SceneNpcs`拥有当前场景NPC的可变状态和动画，借用`ArenaResources`中的模型。`npcInfo.npc`直接来自用户原scenedata.cpk；骨架/网格使用已核对的用户缓存，AMF/AMD仍从原palactor.cpk读取。模块不依赖JNI、GL、SDL或libnx，不增加发布资产。

## 原处理链

- IDA 5D3BC0 / `ScriptNpcVisibility::SetVisible`：缺名无副作用；隐藏只Leave；显示先Leave，再SetStateFlag(1)，最后Enter。重复显示仍执行这条顺序。
- 初始场景登记复用51BB00 / `GameArenaNpcItemCallbacks::EnterNpc`；脚本进出复用51CA00/51CAA0 / `GameArenaScriptCharacterCallbacks`。初始进场和脚本进场的运动属性参数不同，分别保留。
- 活动状态由`BaseObjectActivity::SetStateFlag`写入；默认动作取原NPC行为文档，经现有`SceneNpc::DefaultAction`解析。`ActorPlayback`处理原动画和AMF。隐藏对象不推进动画，重新显示设置原restart标志，不自行归零动画时间。
- Leave移除世界、运动、相机及绘制准备登记，并用原`CollisionActor::ClearBinding`清除碰撞属性。Enter把同一状态交给安卓绘制消费。

游戏旧DWORD字段只存本集合句柄和原数值，不保存64位宿主指针。场景切换先销毁借用NPC的动作任务，再销毁NPC绘制资源/状态，再替换被借用模型。EGL恢复重传GPU，保留CPU状态及动画时间。

## 碰撞属性

原4441A0 / `BaseObjectPropertyCallbacks::Initialize`在初始及脚本Enter中绑定碰撞属性，即使NPC当前隐藏也可能执行。原规则先Clear，再写四个回调标识，按零向量选面/投影；这里不把这次初始化的零向量误写到NPC实际位置。

`CollisionSurface`复用原`CollisionTreeStorage::SelectFace`、`D3dTriangleFace`、`D3dFacePointProjection`和高度处理器。面句柄为索引加1，零表示未选中；不通过宿主地址模拟x86内存。

重要的缓存约定：P4CL按DFF流记录顶点，原RpGeometryStreamRead在788A09..788A23交换前两个halfword，再形成内存RpTriangle。适配层恢复这个顺序后建面。未交换时，真实Q01从原零向量初始化会选不到面；修正后选到句柄2472。SDK读入函数保持库覆盖，只参考其边界行为。Q01缓存3490个地面三角形无重复顶点；其他场景的退化面过滤和完整邻接构建尚未专项验收。

## 绘制与未完成边界

安卓逐帧从公共集合筛选实际可绘制NPC，采样骨骼，按原位置/旋转绘制，与世界/玩家共享相机和深度。隐藏模型即使已上传GPU也不提交绘制。

绘制准备登记不是原RW材质/几何插件的完整等价实现。影子、附着武器/特效、自动行为、运动任务、邻接寻路、动态碰撞响应及NPC交互仍待接入；报告明确给出`motion_bound=false`。当前未创建武器/特效拥有者，遇到活动拥有者的清理请求会明确失败。碰撞回调标识和属性初始化不等于运动回调已经执行。

## 验证

主机测试覆盖初始显示、缺名、隐藏清理、隐藏动画冻结、重复显示的Leave/Enter顺序、原位置保留，以及多层地面选择和边界外投影。真实Q01原包/用户缓存另验证四个NPC初始隐藏，显示MN001后Z01推进，原408模型采样2382顶点。原场景接入基线主机42/42；当前连同动作任务47/47，Android双ABI和Switch公共arena交叉编译通过。

N01的201J包围球缺口已解决，当前使用P4AR v3。整套N01四模型和世界/碰撞现从原包重生成，40份原输入核对通过，报告`prepared-data/atomic-distance-runtime/N01-source-verification.json`；早期仅五份NPC输入的证明不再代表当前整套范围。`verify-media`仍在影片前预检实际入场，不估算半径。

`scene-npcs-probe.json`保存实际登记、碰撞面、动画时刻与姿势校验值；`status.json`的npc_draws/npc_triangles记录实际GPU提交。`verify-dialogue.ps1`用同一原包资源在主机采样设备记录的动画时刻，比较变换、选面和姿势。当前APK及设备验证结论以[安卓状态](../android/STATUS.md)为准，不能将主机测试当作完整章节验收。

NPC接入基线（2026-10-27）APK 40778F05…连续验证进程18009：N01原201J实际提交1522三角形，画面小山猪已目视；二十句对白后执行76，Q01原408提交794三角形。两者同时间主机姿势误差均为0，变换及原选面一致。室外NPC后台冻结，EGL上下文6→7后状态/显隐计数保持、动画继续，GL错误0。累计116调用停250的grid阶段：(-575,61,-576)已提交且地面属性完成，有层地图尚未更新；不把这个部分失败算作250完成。设置冷重启20309、源码对照20379通过。

本轮N01独立快照与连续验证属于同进程/同APK，报告`emulator/N01-initial-npc-verification.json`，核对这两项身份后并入media报告。后续`verify-media`已包含同样的自动核对。首次缺包围球失败保留于`emulator/npc-media-verification.log`，最终连续记录为`npc-media-final-verification.log`。

当前APK与设备身份见[安卓状态](../android/STATUS.md)。21句后136调用完成272原NPC动作，停97对Jeffect001的类型8特效显隐链。MN001的J01实际播放92帧，末帧2.66666675秒后恢复Z01；同时间主机姿势误差0，动作中/恢复后截图均已目视，GPU794三角形、GL0。N01原201J仍提交1522三角形；EGL6→7保留CPU材质与显隐。见[NPC动作](NPC_ACTION.md)；自动行为、NPC运动及完整室外玩法仍未完成。
