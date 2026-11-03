# 剧情镜头

公共 `CameraTracks` 接入287 `giCameraPrepare`、288 `giCameraRunSingle`。原始路径为 `用户scenedata.cpk → Q01\N01\MC001.cam → ReadSceneCameras → 原任务容器/镜头算法 → GameCameraSystem → 世界与玩家共用渲染矩阵`，不依赖JNI、libnx或GPU对象。

## 复用与所有权

本轮在IDA复核0x5E4660、0x5E46C0、0x592680，复用已有 `ScriptCameraSequences`、`CameraTaskObject`、`GomTaskContainer/GomTaskBase`、参数设置和插值代码。镜头记录、BinaryXml、任务、任务名称和等待状态均由模块拥有；原32位地址槽只存检查过的数值句柄。

任务基类使用真实C++对象；原算法通过受限字节接口读取其对象表示，不把字节数组或64位指针强转成原对象。主机原生逃跑诊断通过 `PAL4_PORTABLE_RUNTIME` 排除，其余任务容器逻辑共用同一实现。

287先清理当前任务，再从原包读取指定镜头文件。288选择已准备记录、建立原32字节等待状态、加入任务并开始等待。脚本令牌只在原等待处理器关闭后完成；非阻塞调用仍推进真实任务。替换镜头后，旧等待同样观察新容器，等新任务结束；文件或记录不存在返回0，不伪造播放成功。

## 时钟与渲染

MC001原文件628字节、2条记录。001使用mode1：原Enter直接设置目标、角度和距离，将任务时间设为记录的2秒并标记完成。随后仍经过原容器的进入、运行结束和清理三个阶段；这个时间字段不表示额外等待2秒。

002为mode0、时长4.5秒，使用原方向处理、插值和角度差算法。目标锚定与眼睛锚定分别更新目标和眼睛；后续帧矩阵保持相应锚点。主机夹具覆盖立即、插值、眼睛锚定、替换等待、非阻塞、无记录、非法输入，以及脚本栈挂起/恢复和重复完成拒绝。

原音频监听器的坐标转换被保存在公共状态中；空间音频后端尚未接入。摄像机默认仍为MainCamera；其他相机选择、循环镜头、完整镜头与控制器联动尚未完成。默认投影配置和原场景绘制边界见[游戏相机](GAME_CAMERA.md)。

## 数值对照的未解决项

向量归一化和矩阵转四元数使用固定版本HandmadeMath公开接口；方向查表归一化、四元数转欧拉角等游戏代码仍复用原重建实现。来源与授权见[第三方来源](../../third_party/handmade-math/PROVENANCE.md)。

独立Win32 RenderWare SDK null后端比较同一适配器的6组方向：最大角差0.00444793701°，超过预设0.001°，报告 `H:/PAL4/objs/pal4-android/camera-geometry-sdk-oracle/comparison.json` 的passed为false。保留此失败，不调宽阈值；插值姿态的原SDK数值等价尚未证明。首条001立即镜头不经过该四元数分支。

可重复构建SDK对照工具：

```powershell
cmake -S fresh/ports/tools/camera_geometry_sdk_oracle -B H:/PAL4/objs/pal4-android/camera-geometry-sdk-oracle -A Win32 -DPAL4_RW_SDK=H:/PAL4/pal4_rewrite/refcode/renderwareGraphics/Graphics/rwsdk
cmake --build H:/PAL4/objs/pal4-android/camera-geometry-sdk-oracle --config Release
H:/PAL4/objs/pal4-android/camera-geometry-sdk-oracle/Release/camera_geometry_sdk_oracle.exe H:/PAL4/objs/pal4-android/camera-geometry-sdk-oracle/comparison.json
```

## 设备验证

`verify-media.ps1`比较设备与原包全部镜头字节、原首记录目标/角度/距离、三个任务阶段、等待恢复顺序、实际世界/玩家渲染矩阵及EGL恢复。`camera-frames.jsonl`保存001三阶段及002插值期间的脚本与绘制状态，`camera-track-probe.json`保存最终快照。当前APK和通过范围以[STATUS.md](../android/STATUS.md)及报告为准。

原133音效、292淡入和239等待已接通，002插值完成后进入首条中文240。淡入后的木屋实际截图已检查；NPC/GOB绘制、富文本对白与战斗仍未接入，第一章尚未完成。

2026-10-27 APK `B55C4B092AF8BED4674320BB1F6D6EFA94F7F9FE1B9B2860FB5DC01BDEFAEA2B`，模拟器5706完整自然播放后29次调用，在240显示等待；冷重启7685。001目标(-74.1172028,12,-119.752701)、yaw242°/pitch16°/roll0°、距离145.600006；002用4.5秒推进，记录120个合计任务帧，实际距离和矩阵变化、脚本等待及两次完成均通过。主机/设备最终矩阵最大误差0，EGL恢复通过。该一致性不能证明原SDK数值等价；上述SDK角差失败仍开放。

历史AE23包的001单镜头报告已归档为 `emulator/media-verification-through-camera001.json`。

扩展流程已实际调用MC003、MC005与MC004；第十四句后请求290 giCameraWait并真实等待MC005/002完成，再继续至第二十句与室外资源装载后的97边界。附加镜头不纳入MC001两条记录的数值等价结论；SDK差异仍按前述失败保留。

服务290为 `void giCameraWait()`：复用原 `CreateWaitHandler(0x5E4B20)` 与 `UpdateWaitHandler(0x5E4BB0)`，建立独立等待并在相机完成后恢复令牌，不清除任务或重置镜头。空闲相机也挂起至下一次更新。公共测试覆盖空闲等待、非阻塞镜头继续推进及脚本恢复；设备验证状态以最新报告为准。
