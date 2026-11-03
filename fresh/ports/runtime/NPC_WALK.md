# NPC 脚本行走

公共 `NpcWalks` 由 ChapterHost 持有，Android/Switch 共用；平台只负责输入、文件和绘制。

## 原逻辑与所有权

- 269/270 复用 `ScriptPlayerWalk::WalkNpc/RunNpc`；结束等待复用 `EndMoveNpc/Update`。
- 已在 IDA 复核 `0x5E2A90`：按名找 NPC、目标投影至地面、跑速 135，缺 C03 时选 Z02；按最后 bool 决定是否挂起 CSB。
- 路径和静态碰撞使用既有 CollisionSurface/SurfaceMotion；变换提交复用 BaseObjectMotion。没有另写路径算法。
- SceneNpcs 独占动画时钟。NpcWalks 仅推进运动及原等待处理器，提交 typed 位置/朝向；隐藏 NPC 冻结运动。
- 单场景仅有一个脚本运动拥有者，换场先销毁 NpcWalks，再释放 SceneNpcs 和地面。数值句柄不保存宿主指针。

## 当前验证

APK `214EEC59…` 从原 CPK/CSB 新游戏通过 25 句、170 调用，停未绑定 81（giCameraSetMode）。MN001 使用原 Z02 缓存，以 135 跑速实际移动，随后被脚本隐藏；这不是到达终点的证据。

`verify-npc-walk.ps1` 采集原调用与逐帧记录，`verify_npc_walk.py` 用设备实际 delta/显隐在主机重放同一原处理器，核对位置、朝向、路径、速度和处理器状态。最终进程29285共2012帧、差0；错误目标、瞬移、错误速度、伪完成四类负例均拒绝。报告在输出目录 `emulator/npc-walk-comparison.json`；跨 ABI 一致不等于独立证明原 SDK 数值完全等价。

主机还覆盖非阻塞、隐藏冻结、动作回退、End 等待、任务替换、单一运动拥有者及非法参数。动态 NPC/机关碰撞、完整 AI、洞穴战斗仍未接入。
