# CombatCamera 矩阵对照

## 采集方式

独立工具 `ports/tools/combat_camera_sdk_oracle` 在隐藏的 `800×600` D3D9 窗口中创建 RenderWare SDK `CombatCamera`，按已验证的 `NativeCameraObjectResources` 适配路径设置原版战斗相机姿态，附加一个内存中的 prelit 三角形 atomic 到 `RpWorld`，调用 `RwCameraBeginUpdate` 和 `RpWorldRender`。view/projection 从活动 `IDirect3DDevice9::GetTransform` 读取；另以 SDK frame LTM 逆矩阵和公开相机参数交叉核对。

对照统一了 D3D9 行向量矩阵内存与列主序数学矩阵、view-only 的 D3D 相机 X 反射、D3D 深度 `[0,1]` 到 OpenGL 深度 `[-1,1]` 的转换、viewport 和 NDC 屏幕方向。VP 比较全部16项，并用相机前方6个同一世界点比较屏幕坐标。

相机姿态和投影数据来自 SDK 相机对象及已重建的游戏配置初始化路径。world 中只有用于触发真实 RenderWare draw path 的单三角形 fixture，不读取原版 CPK，也不写入 SD/临时资源缓存。采集器不创建可见窗口、不激活窗口，不访问 Android 模拟器。

## 原版参数依据

- IDA `0x53F870`（`CombatCamera_SetupDefaults`）在 `CombatCamera` 上依次设置目标 `(13,20,-2)`、距离 `452`、角度 `(178,14,0)`，每次更新矩阵。IDA 直接反编译结果和三个游戏调用者均已复核。ownership 分类为 `OWNERSHIP_REVIEW_REQUIRED`，现有文档将它归类为游戏适配器；没有把它当作 RenderWare 私有库函数重写。
- `GameConfigInitializer` 初始化 near clip 为 `5.0`、far clip 为 `44700.0`，并将默认 view scale `0.5` 放入配置项5。WinMain 将前两项传给原生相机资源，将配置项5用于视窗调整。
- SDK 对 `800×600` viewport 回报 view window `(0.5,0.375)`、near `5`、far `44700`。这些是运行时相机 getter 的结果，不是 oracle 临时默认值。

## 结果

完整采集 JSON：`H:/PAL4/objs/combat-camera-recovery-20261030/combat-camera-d3d9-comparison.json`。

| 指标 | 结果 |
|---|---:|
| 视窗 / slot | `800×600` / portable slot `2` |
| SDK view window | `0.5, 0.375` |
| near / far | `5 / 44700` |
| eye 位置误差 | `0` |
| target 位置误差 | `7.629394531e-6` |
| frame basis 最大分量误差 | `0` |
| D3D view 统一 X 方向后，与 portable view 最大逐项误差 | `0` |
| SDK frame inverse view 与 portable view 最大逐项误差 | `1.172348857e-5` |
| SDK getter projection 与 D3D device projection 最大逐项误差 | `4.768371582e-7` |
| SDK device VP 与 portable VP 最大逐项绝对误差 | `6.103515625e-5` |
| VP 逐项误差 RMS | `1.525882001e-5` |
| 6点最大归一化屏幕坐标误差 | `0` |

SDK D3D device view matrix（直接从 `IDirect3DDevice9::GetTransform(D3DTS_VIEW)` 读回，列主序，16项）：

```text
0.9993908405, 0.008442941122, 0.03386278823, 0,
0, 0.9702957273, -0.2419219017, 0,
0.03489945084, -0.2417745292, -0.9697046876, 0,
-12.92228222, -19.99921417, 454.4588318, 1
```

SDK D3D device projection（直接从 `IDirect3DDevice9::GetTransform(D3DTS_PROJECTION)` 读回，列主序，16项）：

```text
2, 0, 0, 0,
0, 2.666666746, 0, 0,
0, 0, 1.000111818, 1,
0, 0, -5.000558853, 0
```

frame inverse view（列主序）在 view-only X 方向归一化后与 portable view 完全一致。SDK view eye、target 和 basis 分别独立记录，目标由 SDK frame 的 `pos + at * distance` 重建。

`RwD3D9GetTransform` 在本 SDK build 中仍返回单位阵；直接 COM `IDirect3DDevice9::GetTransform` 在实际 `RpWorldRender` 后返回有效非单位矩阵，报告将直接 device readback 标为有效。`RwCameraGetViewMatrix` 原始 getter 在这个 fixture 下与 frame inverse 最大元素差为 `273.8942261`，所以作为诊断单独保留；最终 view/VP 对照使用直接 D3D device 状态，屏幕误差也由 device matrices 投影得到。旧 `COMBAT_WORLD_PROGRESS.md` 中 near `1`、far `10000` 的说明已被这次配置路径证据取代。

## 工具与验证

工具源码为 `ports/tools/combat_camera_sdk_oracle`，RenderWare SDK 依赖 `refcode/renderwareGraphics/Graphics/rwsdk`。它复用 `NativeCameraObjectResources` 创建和更新 SDK camera/frame，调用 `RpWorldPluginAttach` 后创建 world、camera 和一个 prelit atomic。world 先移除 atomic/camera 并释放 fixture；随后原生相机资源 owner 使用 `NativeCameraObjectResources::Destroy` 清理 camera/frame/rasters。frame advance slot在此单次采集工具中不调用。

构建命令：

```powershell
cmake -S fresh/ports/tools/combat_camera_sdk_oracle `
  -B H:/PAL4/objs/combat-camera-recovery-20261030/build `
  -A Win32 `
  -DPAL4_RW_SDK=H:/PAL4/pal4_rewrite_android/refcode/renderwareGraphics/Graphics/rwsdk
cmake --build H:/PAL4/objs/combat-camera-recovery-20261030/build `
  --config Release --target combat_camera_sdk_oracle --parallel 8
```

构建和隐藏窗口采集成功，JSON 已用 PowerShell `ConvertFrom-Json` 校验。Android GLES CombatWorld 探针已同步为 `800×600`、near `5`、far `44700`、view scale `0.5`；本轮没有运行 Android 部署或模拟器画面验证。

共享 `GameCameraConfig` 的默认 near/far 已按本次原配置路径证据修正为 `5/44700`，供 Android/Switch 的实际章节宿主使用；显式传入配置的探针不受默认值变化影响。该修改仍需通过共享相机合同与两端构建，不能由本次 SDK 数值参考替代设备画面验收。
