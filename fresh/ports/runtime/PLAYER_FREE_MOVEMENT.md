# 玩家自由移动提交

## 2026-10-31 Switch 行走退出定位

实机 0.2.15 在 Q01 教程后触发 `func2006`，服务257请求脚本行走，最终应用内错误为 `walk camera follow is not bound`。证据：`H:/PAL4/objs/walking-crash-20261030-235502/runtime.log`。该错误来自 ChapterHost 的脚本行走移动通知，并非已证实的原游戏规则缺陷。

移植层现将该通知的 auto_seek 分支接到既有 `PlacementServices::FollowPlayer`，复用当前控制角色校验及 `CameraFollow::FollowPlayer`，保留地面和小地图更新顺序。未关闭 auto_seek 或跳过脚本行走。主机118项共享回归、Android原生库和含FFmpeg/CEGUI的x86_64 APK构建通过；这些检查不覆盖完整实机故障路径，教程后连续行走、方向响应和Switch复验仍待完成。此前交付的0.2.16 NSP不包含此后续修复。

新原包复验：模拟器使用与 `I:/Games/original` 哈希一致的27,220,190字节Q01.cpk，从新游戏完成25次对白和教程，再只发送W/S键输入，越过初始区域边界自然触发 `Q01 func2006`。服务257完成后继续执行转向、相机模式及第26次“先回房拿弓”对白，未出现原先的未绑定错误；到达 `(-564.459778,57.9313126,-447.053497)`、朝向175，`auto_seek=true`，行走任务已完成且无遗留handler。未改位置/剧情变量。证据 `H:/PAL4/objs/walking-newq01-20261031/func2006-after-walk.json`、`runtime.log`、`func2006-frame.png`。这覆盖了安卓模拟器的原故障路径，不证明Switch、手机、所有行走方向和后续流程均已验收。

`PlayerFreeMovement`复用`actor_vector_movement.h`，把原自由输入输出连接到PlayerRoster、PlayerActions、SceneCollision与SceneMovement。它借用场景owner，场景切换前应销毁；平台输入、模态窗口和脚本事件由ChapterHost管理。

## 原顺序

相机访问 → 读取当前位置/模型前向 → dt乘原模式速度 → 准备双球体 → 选择C02/C03 → 三组碰撞修正 → 地面选择 → 检查并回写地面高度 → 位置提交/跟随通知 → 原近似反余弦求朝向 → 地面属性通知。

- 位移使用上一帧模型前向，接受地面后才朝向当前输入，保留原一帧顺序。
- 身高来自角色绑定球半径乘2；向上/下地面查询分别乘原0.35/0.53浮点常量。身体探测半径使用控制器+0x24的18，不混用角色身高。
- `SceneMovement::CheckAndAdjust`复用原活动子对象检查，成功才回写位置；失败时不提交玩家位置、朝向或地面属性。主/次地面切换均有测试。
- PlayerActions实际选择原动作和AMF；不创建替代剧情动作。原向量流程本身没有小地图调用，地图更新应由外层帧流程连接。

## 已验证

主机整合测试覆盖输入到落点、C02/C03、原速度、朝墙位移清零但可转身、随后沿墙行走、缺地面拒绝提交和锁定门控；原场景缓存独立探针在主机和安卓各执行76次有效输入/落点提交，N01/Q01均通过。源动作消息直接读取用户原palactor.cpk内101/C03.amf。

探针起点取第一块地面三角形中心，并使用固定世界轴方向，**不是剧情出生点或实际游戏流程**。相机/HUD外部服务只记录调用，该移动探针未绑定配置对象；SceneObjects的独立绑定已另外验证，见[物件碰撞](OBJECT_COLLISION.md)。它另从scenedata.cpk确认原floor/wall命名空间，并核实这两场景没有intri；发现intri则明确失败。不会据空缓存假定原包缺失。

主机和安卓结束位置差小于0.001，9项定向CTest及Android双ABI通过。精确开发APK、CLI、输入SHA256和报告：`H:/PAL4/objs/pal4-android/emulator/baselines/free-movement-b6718e09`。探针不运行CSB，game_calls始终0；本轮未替换已安装应用。

## 调用工具

主机既有runtime构建目标：`pal4_portable_free_movement_probe`。Android独立项目：`fresh/ports/tools/free_movement_probe`，复用同一实现。参数顺序：

```
free_movement_probe data_dir actors_dir JDumy.p4a collision.p4c palactor.cpk Q01 N01 output.json
```

palactor.cpk旁需有用户scenedata.cpk；角色/碰撞等缓存由既有转换器从用户原文件生成，不进入公开运行时包。

## 尚缺

2026-10-29 模拟器真实剧情关闭教程后已执行 75 和 67，但 Q01 的原场景类型为 0，`free_movement_bound=false`。当前创建条件仅覆盖类型 1，控制器对类型 0 选择事件输入；已有 `ProcessInput` 将其派发至 `HandleInputEvents`，便携宿主尚未连接该分支。需恢复并验证事件输入、原离散位移及碰撞，不应把类型 0 强制改为类型 1。

共享控制器提供事件输入命令分派，详见 [控制器](CONTROLLER.md)。

2026-10-29 已将类型 0 事件命令接至原离散位移，复用场景碰撞、地面和地图更新；行走门控改检查实际活动状态，避免历史请求永久阻塞。主机测试覆盖前进、转向、无地面拒绝及行走结束。模拟器真实新游戏中，关闭教程后虚拟摇杆触发 `Q01 func1001`，从 Q01 进入 N01；N01 的 66 场景就绪随后报 `arena ready hints or layered minimap are not bound`，完整流程尚未通过。类型 3/5 无距离回调的 GOB 使用原插件默认允许碰撞，未删除其碰撞形状。移动数值、墙体及手机/Switch 仍需实机和 PC 对照。

ChapterHost已在完整碰撞缓存可用的类型1场景创建此owner，并在每帧处理平台方向状态；对白、教程、媒体等待、装载、淡入淡出、镜头轨迹和脚本行走时门控。空闲复用原424750尾部：忙时清计时、空闲C01、六秒C12、完成后恢复。Android方向键与Switch左摇杆共用；当前仅Android双ABI构建和入口启动已验证，实际教程关闭、75解锁及移动仍需手动验收。类型1场景事件已接入碰撞回调及CSB压栈，机关交互与实际路线仍待验收，见SCENE_EVENTS.md。小地图、相机/HUD的真实更新和动态缩放仍待接线/验证；石沉溪洞剧情与战斗、离峰均未验收。


## 章节开发版验证

原控制器帧合同已加入便携主机测试，连同玩家、输入和碰撞共7项通过。Android缓存优先读取用户PAL4/cache，开发资产仅作后备；角色101增加原C12动画，18份输入与用户palactor.cpk核对一致。模拟器已更新完整N01/Q01碰撞缓存与该角色缓存，逐文件SHA256核对通过；应用停在未开始新游戏的入口，game_calls=0、GL0。未向实际剧情发送输入。

当前键盘/方向按钮将状态映射为原离散输入，摇杆有0.25死区，未实现模拟量速度。完整原控制器的相机交互、空闲交互提示及EVF事件队列尚未接入；地图更新仍待外层原调用链核对，不在向量函数里擅自增加调用。
# 离散移动诊断（2026-10-29）

共享报告现记录离散移动的角色高度、地面查询范围、累计实际位移，以及最近一次碰撞前后位移；转向时清空位移字段。该改动不改变移动或碰撞结果。主机测试覆盖前进、墙体将位移清零和转向；Android x86_64 无资产 APK 构建通过。未安装到当前模拟器会话，西侧阻挡原因和 Switch 实机仍待验证。
