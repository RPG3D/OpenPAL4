# CombatWorld 原包资源进度

更新时间：2026-10-29。此模块只负责把原 CombatWorld CPK 中的静态 BSP、16 字节 lightmap config 和材质纹理转换为 portable `Scene`，供现有 Scene/GLES renderer 消费；它不等同于完整 PAL4 `NativeCombatWorldResources` 或可玩的 CombatSystem。

## 原路径与复用解析

native `NativeCombatWorldResources::Load(root, region, scene)` 使用 `root/gamedata/PALWorld/{region}/{scene}.cpk`。Combat Start 把 region 设为 `CombatWorld`，area 来自本次请求；归档读取 `{scene}/{scene}.bsp` 与 `{scene}/{scene}_ltMap.cfg`。IDA 文档区分 Combat mapper 0x6706E0 与普通 PALWorld mapper 0x6704C0。普通场景 `ArenaSource::OriginalDirectory` 只能定位 `PALWorld/{world}.cpk`，不能映射此嵌套 CombatWorld 路径。

新增 `portable::CombatWorldResources` 使用 `PackageReader` 直接打开 `<game_data>/PALWorld/CombatWorld/<area>.cpk`，按 native 成员名读取 BSP/config，再复用 `ReadBspScene` 与 `DecodeOriginalTexture`。它不固定 M01；安全 area 名成为实际 CPK 路径和 member prefix。配置小于 16 字节、CPK/BSP/config/required diffuse 纹理缺失、损坏或超出已有 parser 限额时抛错，候选状态只在所有解析完成后提交。`Lease` 保持已解析数据存活，并阻止 reset 或 area replacement；reset 后可加载另一 area。

## 验证与边界

H 盘独立 host 构建目录：`H:/PAL4/objs/pal4-combat-runtime-build`。`pal4_portable_combat_world_resources_contract` 通过；M01 原包探针以 `I:/Games/original/gamedata` 加载真实 `PALWorld/CombatWorld/M01.cpk` 成功，覆盖 area/property、非空 Scene、同区重载、借用期 reset/replacement 拒绝、release/reload，以及不存在 `M02.cpk` 时保持空状态。该 M01 CPK SHA-256 为 `7CA3800E6C8D5646F14029259502C6BB73745B7D666BFFC56BC6F7970925B18B`。

独立 Android EGL pbuffer probe 已在 `emulator-5558` 运行，不安装 APK：`CombatWorldResources` 加载真 M01 CPK，`GlesRenderer` 上传 3,041 个 BSP triangles、136 draws、33 textures，并完成 1280×720 GLES readback。早先仅将 native 数值覆盖到 `OrbitCamera` 的捕获被拒绝：姿态算法并不相同，不能作为原相机近似。

修正后的探针在 `pal4_portable_game_camera` 上链接共享 `GameCameraSystem`，向 camera slot `2` 传入同一组 `CombatCameraAccess::Setup` 参数，使用 `GameCameraSystem::Matrix` 输出相机矩阵。`emulator-5558` 上 Android `Google SwiftShader` 运行通过，readback 无 GL 错误；M01 仍为 3,041 triangles、136 draws、33 textures，JSON `visible_pixels=921600/921600`（该计数只是与固定背景色比较，不是几何覆盖率）。画面显示地面和石笋。投影采用共享 camera API 默认 near=`1`、far=`10000`、view_scale=`0.5`；生产 SDK 相机投影设置尚未核实，也没有同姿态 PC/SDK reference capture，因此不声称与原版像素或投影等价。RGBA/PNG/JSON 输出：`H:/PAL4/objs/combat-world-gpu-probe-android-x86_64/M01-shared-camera.*`。独立进程返回成功后，临时设备目录 `/data/local/tmp/pal4-combat-world-gpu-probe-camera2-20261029` 已删除。

当前 portable Scene 解析不重建 native `NativeWorldAsset` 的 RenderWare clumps、UV dictionaries、SDK world/camera attachment 或 `MatFxWorldSectorPipeline`。可加载的 CPU BSP 不表示这些对象已接入 ChapterHost Combat Start；本模块尚未接 Entry、CombatWorld frame/UI/events、actor roster 或 M01 战斗路线。也未解除普通 Arena 的非空怪物/路径门禁，不应计入本轮 Switch 可玩包。

下一步由 ChapterHost Combat Start owner 在验证完整依赖后持有 `CombatWorldResources`，用 Start config 中的实际 `CombatArea` 请求加载，向 Scene renderer 提供 lease，并在结束/失败清理时先释放 lease 再 Reset。若要匹配 native完整 world 行为，还需补独立且经原 CPK 验证的 clump/UV/照明与 camera/event owners。
