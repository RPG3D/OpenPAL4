# 游戏相机

`GameCameraSystem` 是独立于窗口和 GPU 的 CPU 模块。它复用重建的 `CameraData`、矩阵、视锥、参数设置、环境探测及障碍距离恢复代码；安卓将其输出矩阵用于世界和玩家绘制。调试预览仍使用 `OrbitCamera`。

## 原版依据

- `CameraSystemInitializer` 初始化 Main、Script、Combat、UI 四个相机，目标为原点，距离350，pitch30°、yaw225°。
- IDA `0x5D4DD0`（103，`giCameraSetCollide`）查找 MainCamera 和 ScriptCamera，将各自 `CameraInternal + 0x1A8` 写为 `enabled == 1`；Combat/UI 不受此调用影响。
- 公共模块保留原 CameraData 的固定宽度数值布局。存在标识使用数值句柄，不把宿主指针缩成32位。

## 平台适配与边界

旋转适配实现原调用所需的 REPLACE/POSTCONCAT。渲染直接读取计算后的眼睛和基向量，将 RenderWare 朝向正Z的投影转换为 OpenGL 深度范围。没有创建原生 RenderWare frame，frame 更新接口由输出矩阵替代。

目前仅更新 MainCamera。静态探测遍历世界三角形，复用原四条偏移探测线及距离恢复；缓存点范围检查使用 AABB，尚未证明与完整 ClumpSpatialBounds 等价。长度使用标准平方根，未复现原数学查表近似。静态探测尚无空间加速。

near=1、far=10000、view_scale=0.5 是显式临时配置，尚未从原配置文件读取。其他相机切换、完整尾随输入、震动随机数、动态物体碰撞和完整出生点相机联动尚未接入；震动回调明确失败。81/104和当前玩家目标刷新已接入，见下节。

## 验证

`game_camera_tests.cpp` 检查零角与90°旋转、屏幕方向、近远平面、静态墙遮挡、关闭碰撞、移动目标后的距离恢复及无效参数。偏移探测线碰墙时应保留原几何距离，不能直接将距离写成墙面深度。

安卓 `verify-media.ps1` 检查原CSB调用顺序、103对四相机碰撞开关的影响，以及相机绑定、世界/玩家和脚本状态在EGL重建后保持。`game-camera-probe.json` 是103执行时的状态快照；`status.json` 每秒另外采样实际相机状态，恢复检查使用这份实时状态核对开关。完整镜头姿态和画面仍需后续剧情验证。

模块已通过主机测试和 Switch AArch64 交叉编译。交叉编译不代替 Switch 实机画面验收。

剧情镜头通过受限的数值读取和参数接口更新MainCamera，绘制继续使用同一矩阵。任务选择眼睛锚定时，后续帧保持眼睛坐标；SetParameters/SetTarget恢复目标锚定。`camera_tracks_bound`在实际应用镜头参数后才置true；103快照保留此前的false。原包、等待与SDK数值差异见[剧情镜头](CAMERA_TRACKS.md)。

## 81/104与左右方向（2026-10-27）

公共CameraFollow复用ScriptCameraModeHandler、ScriptCameraCallbacks、CameraSystemInitializer和CameraModeConfiguration；六项模式参数直接取原初始化表。0/5刷新角色目标并执行共享FlushActorCameraTailYaw，其他模式按原bool处理目标；无效模式保留原模式。

角色目标复用BaseObjectSceneParameter，读取角色位置和原绑定球半径，目标Y为位置Y+radius*2*0.9。尾随变换通过公共HandmadeMath提供逆矩阵、归一化、乘法和旋转查询适配，不重写私有SDK数学代码。原SDK九组姿态对照的最大朝向差0.003204°，未达到0.001°阈值；保留失败报告camera-follow-sdk-comparison.json，不宣称SDK完全等价。

104刷新目标后按原规则转换Z，保存监听器位置/朝向/up；当前2D输出未绑定空间音频，报告spatial_output_bound=false。原初始化写入的x86动态碰撞回调地址未映射为宿主函数，现仍只启用静态world探测。

Android D330版本实际通过25句/175原CSB调用，跨过81/104，停309教程窗口。相机yaw225→174，半径39.2483101，目标(-579,126.646957,-421)；同进程EGL3→4保留相机/监听器状态。

用户指出两端3D画面左右相反后，通过原SDK公开RwCameraBeginUpdate/GetViewMatrix验证：frame为单位矩阵时，中心屏幕X=0.5，世界+X投到0.4，-X投到0.6。公共OpenGL投影原来漏掉X方向转换，现仅在GameCameraSystem::Matrix将X投影设为负；保留世界坐标、相机姿态、UV与2D界面。

camera_projection_sdk_oracle的四点对照误差0，主机增加三种yaw/宽高比的方向检查。Android A83C版本再次通过175调用；578200个静态世界像素与修复前图像水平反射后平均差0.02954，未反射时20.94095，截图已目视。证据camera-projection-sdk-comparison.json、camera-mirror-pixel-verification.json。Switch0.2.5已nxlink启动，0.2.6加入FPS及媒体内存/流读取；实机完整章节仍未验收。

## 91独立尾随刷新（2026-10-28）

IDA `0x5D4050`转发到`0x423FF0`：有当前角色/MainCamera时，按原有符号修正水平朝向，并以keep_look_at=1更新。CameraFollow新增独立入口，复用模式0/5已有的FlushActorCameraTailYaw与PalActorControlTransform；GameServices服务91与ChapterHost均已接线。

主机及安卓原生camera_follow_contract验证五种水平角、原脚本声明路由、无角色、未绑定和多参数负例。独立调用保持目标、俯仰、滚转、请求距离、相机模式和玩家auto_seek；重复调用的174°用例出现约0.000015°浮点差，矩阵差0.00012207，按角0.002°/矩阵0.0002容差检查，未修改原算法。此前SDK姿态对照未达阈值的限制仍保留。

Android双ABI和Switch交叉编译通过。模拟器安装基线`tail-yaw-7306c4f3`，APK读回核验一致；未操作实际取弓或洞穴剧情。
