# CEGUI 共享对白绑定

`ChapterDialogueUi` 供 Android 与 Switch 共用，借用 `ChapterHost` 读取原资源。它加载原始对白页，用原字体计算节点与分页结果并提交 `DialogServices::ConfigureRich`，只根据服务视图绘制；`Draw` 的时间参数仅推进提示动画。逐字、确认和脚本完成仍由共享服务负责。

## 已验证

- 安卓模拟器从原 `ui.cpk` 读取 `101G` 头像并显示，背景上方有 11766 个头像像素；左右布局的头像区域反射后 RGB 完全一致。这是镜像自洽测试，尚不等于 PC 原版头像镜像验收。
- 绘制多次、传入较大绘制时间不会改变正文游标；八次服务更新的节点/文字状态正确。错误代际和对白 ID 的确认被拒绝。
- 九行样本自动分为三页，页尾码点为 8、16、17；补全文字、翻页不会提前释放脚本等待，关闭后仅返回一次完成令牌。
- 连续带头像、无头像对白切换与关闭后，窗口、默认字体和图集均释放。
- 安卓 CEGUI APK 的应用独立样本通过实际帧循环及 JNI 绘制；资源列表为空，Java 字体/图集排版路径未运行。按虚拟 A 后 confirmations=1、completed=1、waiting=false，运行时记录单次 completion。

应用独立样本由启动参数 `--es scene cegui_dialogue_probe` 进入，使用自造短文本及用户原包头像，不启动新游戏。`PAL4_CEGUI_BOUND_DIALOGUE_RGBA_PREFIX` 可输出独立 GLES 绑定测试截图，仅为诊断证据。

## 平台接线与限制

Switch 0.2.13 的正式对白已替换旧 SDL_ttf 排版与逐帧贴图，使用原布局、正文、头像、镜像和提示。完整 NRO 构建及 FTP 读回核验通过，尚无本版实机画面验收。教程、开始界面及 FPS 保留原实现。

Android 的 CEGUI 构建使用 FreeType 2.3.1；Java `DialogueView` 只保留透明输入层，不再把字体或图集复制到 Java。帧循环统一配置、绘制，EGL 丢失时放弃旧 GPU 对象并重建。新 APK 已在模拟器验证，未覆盖用户当前手机试玩会话。未启用 CEGUI 的旧构建仍使用 Canvas，不能混用两种排版结果。

Android 双 ABI CEGUI APK 位于 `H:/PAL4/objs/pal4-android/cegui-game-runtime-20261029/app/outputs/apk/debug/app-debug.apk`，包内无游戏资产。Switch 运行时包为 `H:/PAL4/objs/pal4-switch-test/PAL4_Switch_Test_0.2.13_runtime.zip`。Switch 当前 portlibs FreeType 与 2.3.1 不同，像素一致性尚未证明；真实新游戏、后续页面、完整富文本语义及整章战斗仍未完成。
